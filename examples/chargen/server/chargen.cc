// Refactoring: Anny Wang
// Date: Aug 04 2019

#include "chargen.h"
#include "Logging.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

ChargenServer::ChargenServer(annety::EventLoop* loop, const annety::EndPoint& addr, bool print)
{
	server_ = make_tcp_server(loop, addr, "ChargenServer");

	server_->set_connect_callback(
		std::bind(&ChargenServer::on_connection, this, _1));
	server_->set_message_callback(
		std::bind(&ChargenServer::on_message, this, _1, _2, _3));
	server_->set_write_complete_callback(
		std::bind(&ChargenServer::on_write_complete, this, _1));

	if (print) {
		loop->run_every(3.0, std::bind(&ChargenServer::print_throughput, this));
	}

	std::string line;
	for (int i = 33; i < 127; ++i) {	// 可打印字符
		line.push_back(char(i));
	}
	line += line;

	// see RFC 864. 94行，每行72个字符. 94*72=6768 ～ 6.6k
	for (size_t i = 0; i < 127-33; ++i) {
		message_ += line.substr(i, 72) + '\n';
	}
}

void ChargenServer::start()
{
	server_->start();
}

void ChargenServer::on_connection(const annety::TcpConnectionPtr& conn)
{
	LOG(INFO) << "ChargenServer - " << conn->local_addr().to_ip_port() << " <- "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< (conn->connected() ? "UP" : "DOWN");
	
	if (conn->connected()) {
		conn->set_tcp_nodelay(true);
		conn->send(message_);
	}
}

void ChargenServer::on_message(const annety::TcpConnectionPtr& conn,
		annety::NetBuffer* buf, annety::TimeStamp time)

{
	std::string message(buf->taken_as_string());
	
	LOG(INFO) << conn->name() << " chargen " << (int)message.size() << " bytes, "
		<< "data received at " << time;
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
	printf("%4.3f MiB/s\n", static_cast<double>(transferred_)/seconds/1024/1024);

	transferred_ = 0;
	start_time_ = end_time;
}
