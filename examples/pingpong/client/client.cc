// Refactoring: Anny Wang
// Date: Aug 04 2019

#include "TcpClient.h"
#include "Logging.h"
#include "EventLoop.h"
#include "EventLoopPool.h"
#include "strings/StringPrintf.h"

#include <atomic>
#include <memory>
#include <stdio.h>

using namespace annety;

class Client;

class Session
{
public:
	Session(EventLoop* loop,
			const EndPoint& addr,
			const std::string& name,
			Client* owner)
		: client_(loop, addr, name)
		, owner_(owner)
	{
		client_.set_connect_callback(
			std::bind(&Session::on_connect, this, _1));
		client_.set_message_callback(
			std::bind(&Session::on_message, this, _1, _2, _3));
	}

	void start()
	{
		client_.connect();
	}

	void stop()
	{
		client_.disconnect();
	}

	int64_t bytes_read() const
	{
		return bytes_read_;
	}

	int64_t messages_read() const
	{
		return messages_read_;
	}

private:
	void on_connect(const TcpConnectionPtr& conn);

	void on_message(const TcpConnectionPtr& conn, NetBuffer* buf, TimeStamp)
	{
		++messages_read_;
		bytes_read_ += buf->readable_bytes();
		bytes_written_ += buf->readable_bytes();

		// echo back
		conn->send(buf);
	}

private:
	TcpClient client_;
	Client* owner_;

	int64_t bytes_read_{0};
	int64_t bytes_written_{0};
	int64_t messages_read_{0};
};

class Client
{
public:
	Client(EventLoop* loop,
			const EndPoint& addr,
			int block_size,
			int session_count,
			int timeout,
			int threads)
	    : loop_(loop)
	    , thread_pool_(loop, "pingpong-client")
	    , session_count_(session_count)
	    , timeout_(timeout)
	{
		loop->run_after(timeout, std::bind(&Client::handle_timeout, this));
		
		// start thread pool
		if (threads > 1) {
			thread_pool_.set_thread_num(threads);
		}
		thread_pool_.start();

		// set message block size
		for (int i = 0; i < block_size; ++i) {
			message_.push_back(static_cast<char>(i % 128));
		}

		for (int i = 0; i < session_count; ++i) {
			std::string name = string_printf("C%05d", i);
			Session* session = new Session(thread_pool_.get_next_loop(), addr, name, this);
			session->start();
			sessions_.emplace_back(session);
		}
	}

	const std::string& message() const
	{
		return message_;
	}

	void on_connect()
	{
		if (++num_connected_ == session_count_) {
			LOG(WARNING) << "all connected";
		}
	}

	void on_disconnect(const TcpConnectionPtr& conn)
	{
		if (--num_connected_ == 0) {
			LOG(WARNING) << "all disconnected";

			int64_t total_bytes_read = 0;
			int64_t total_messages_read = 0;
			for (const auto& session : sessions_) {
				total_bytes_read += session->bytes_read();
				total_messages_read += session->messages_read();
			}
			LOG(WARNING) << total_bytes_read << " total bytes read";
			LOG(WARNING) << total_messages_read << " total messages read";
			
			// statistics
			LOG(WARNING) << static_cast<double>(total_bytes_read) / total_messages_read
				<< " average message size";
			LOG(WARNING) << static_cast<double>(total_bytes_read) / (timeout_ * 1024 * 1024)
				<< " MiB/s throughput";

			conn->get_owner_loop()->queue_in_own_loop(std::bind(&Client::quit, this));
		}
	}

private:
	void quit()
	{
		loop_->queue_in_own_loop(std::bind(&EventLoop::quit, loop_));
	}

	void handle_timeout()
	{
		LOG(WARNING) << "stop";
		for (auto& session : sessions_) {
			session->stop();
		}
	}

private:
	EventLoop* loop_;
	EventLoopPool thread_pool_;

	int session_count_;
	int timeout_;
	std::string message_;
	std::vector<std::unique_ptr<Session>> sessions_;

	std::atomic<int32_t> num_connected_;
};

void Session::on_connect(const TcpConnectionPtr& conn)
{
	if (conn->connected()) {
		conn->set_tcp_nodelay(true);
		conn->send(owner_->message());
		owner_->on_connect();
	} else {
		owner_->on_disconnect(conn);
	}
}

int main(int argc, char* argv[])
{
	if (argc < 5) {
		fprintf(stderr, "Usage: client <threads> <blocksize>");
		fprintf(stderr, " <sessions> <time>\n");
	} else {
		int threads = atoi(argv[1]);
		int block_size = atoi(argv[2]);
		int session_count = atoi(argv[3]);
		int timeout = atoi(argv[4]);
		
		LOG(INFO) << "PingPong client start."
			<< "threads=" << threads 
			<< ",blocksize=" << block_size
			<< ",session_count=" << session_count
			<< ",timeout=" << timeout;

		set_min_log_severity(LOG_WARNING);

		EventLoop loop;
		EndPoint saddr(1669);

		Client client(&loop, saddr, block_size, session_count, timeout, threads);
		loop.loop();
	}
}
