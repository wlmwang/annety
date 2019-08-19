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
		: owner_(owner)
	{
		client_ = make_tcp_client(loop, addr, name);

		client_->set_connect_callback(
			std::bind(&Session::on_connect, this, _1));
		client_->set_message_callback(
			std::bind(&Session::on_message, this, _1, _2, _3));
	}

	void start()
	{
		client_->connect();
	}

	void stop()
	{
		client_->stop();
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
	TcpClientPtr client_;
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
	    , thread_pool_(loop, "PingPongClient")
	    , session_count_(session_count)
	    , timeout_(timeout)
	{
		// pingpong timeout
		loop_->run_after(timeout, std::bind(&Client::handle_timeout, this));
		
		// set message block size
		for (int i = 0; i < block_size; ++i) {
			message_.push_back(static_cast<char>(i % 128));
		}

		// start EventLoop thread pool
		if (threads > 1) {
			thread_pool_.set_thread_num(threads);
		}
		thread_pool_.start();

		// new session add to a loop
		for (int i = 0; i < session_count; ++i) {
			std::string name = string_printf("SESSION-%05d", i);
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
		}
	}

private:
	void handle_timeout()
	{
		// called in main thread(main EventLoop timer)
		LOG(WARNING) << "stop all sessions";
		for (auto& session : sessions_) {
			session->stop();
		}

		loop_->quit();
	}

private:
	EventLoop* loop_;
	EventLoopPool thread_pool_;

	std::atomic<int32_t> num_connected_{0};
	int session_count_{0};
	int timeout_{0};
	std::string message_;
	std::vector<std::unique_ptr<Session>> sessions_;
};

void Session::on_connect(const TcpConnectionPtr& conn)
{
	if (conn->connected()) {
		LOG(WARNING) << conn->name() << " has connected";

		conn->set_tcp_nodelay(true);
		conn->send(owner_->message());
		owner_->on_connect();
	} else {
		LOG(WARNING) << conn->name() << " has disconnected";
		
		owner_->on_disconnect(conn);
	}
}

int main(int argc, char* argv[])
{
	if (argc < 5) {
		fprintf(stderr, "Usage: client <threads> <sessions>");
		fprintf(stderr, " <blocksize> <time>\n");
	} else {
		int threads = atoi(argv[1]);
		int session_count = atoi(argv[2]);
		int block_size = atoi(argv[3]);
		int timeout = atoi(argv[4]);
		
		CHECK(threads <= session_count) 
			<< "threads should be less than or equal to session_count";

		LOG(INFO) << "PingPong client start."
			<< "threads=" << threads 
			<< ",session_count=" << session_count
			<< ",blocksize=" << block_size
			<< ",timeout=" << timeout;

		// control timeout callback(main EventLoop)
		EventLoop loop;
		EndPoint saddr(1669);

		set_min_log_severity(LOG_DEBUG);

		Client client(&loop, saddr, block_size, session_count, timeout, threads);
		loop.loop();

		LOG(WARNING) << "PingPong client finish";
	}
}
