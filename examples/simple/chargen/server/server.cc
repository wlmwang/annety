// By: wlmwang
// Date: Aug 04 2019

#include "server.h"
#include "Logging.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

ChargenServer::ChargenServer(annety::EventLoop* loop, const annety::EndPoint& addr, bool print)
{
	server_ = make_tcp_server(loop, addr, "ChargenServer", false, true);

	server_->set_connect_callback(
		std::bind(&ChargenServer::on_connect, this, _1));
	server_->set_close_callback(
		std::bind(&ChargenServer::on_close, this, _1));
	server_->set_message_callback(
		std::bind(&ChargenServer::on_message, this, _1, _2, _3));
	server_->set_write_complete_callback(
		std::bind(&ChargenServer::on_write_complete, this, _1));

	if (print) {
		loop->run_every(3.0, std::bind(&ChargenServer::print_throughput, this));
	}

	std::string line;
	for (int i = 33; i < 127; ++i) {
		// Printable characters
		line.push_back(char(i));
	}
	line += line;

	// See RFC 864. --- 94 lines, 72 characters. --- 94*72=6768 ~ 6.6k
	for (size_t i = 0; i < 127-33; ++i) {
		message_ += line.substr(i, 72) + '\n';
	}
}

void ChargenServer::listen()
{
	server_->listen();
}

void ChargenServer::on_connect(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "ChargenServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "UP";

	conn->send(message_);
}

void ChargenServer::on_close(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "ChargenServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< "DOWN";
}

void ChargenServer::on_message(const annety::TcpConnectionPtr& conn,
		annety::NetBuffer* buf, annety::TimeStamp receive)

{
	std::string message(buf->taken_as_string());
	
	LOG(INFO) << conn->name() << " chargen " << static_cast<int>(message.size()) << " bytes, "
		<< "data received at " << receive;
}

void ChargenServer::on_write_complete(const annety::TcpConnectionPtr& conn)
{
	transferred_ += message_.size();
	conn->send(message_);
}

void ChargenServer::print_throughput()
{
	annety::TimeStamp end_time = annety::TimeStamp::now();
	
	double seconds = (end_time - start_time_).in_seconds_f();
	printf("%4.3f MiB/s\n", static_cast<double>(transferred_.exchange(0))/seconds/1024/1024);

	start_time_ = end_time;
}
