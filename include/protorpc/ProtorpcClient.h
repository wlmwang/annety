// By: wlmwang
// Date: Oct 18 2019

#ifndef ANT_PROTORPC_PROTORPC_CLIENT_H
#define ANT_PROTORPC_PROTORPC_CLIENT_H

#include "TcpClient.h"
#include "EventLoop.h"
#include "Logging.h"
#include "protorpc/ProtorpcChannel.h"

#include <functional>

namespace annety
{
// Wrapper server with protobuf rpc
template <typename REQ, typename RES, typename SRV>
class ProtorpcClient
{
using Stub = typename SRV::Stub;
using DoneCallback = std::function<void(RES*)>;
using DoingCallback = std::function<void(Stub*, REQ*, RES*, ::google::protobuf::Closure*)>;

public:
	ProtorpcClient(EventLoop* loop, const EndPoint& addr)
		: loop_(loop)
		, chan_(new ProtorpcChannel(loop))
	{
		client_ = make_tcp_client(loop, addr, "ProtorpcClient");

		client_->set_connect_callback(
			std::bind(&ProtorpcClient::new_connection, this, _1));
		client_->set_message_callback(
			std::bind(&ProtorpcChannel::recv, chan_.get(), _1, _2, _3));
	}

	void call(DoingCallback doing, DoneCallback done)
	{
		doing_cb_ = doing;
		done_cb_ = done;

		client_->connect();
	}

	void finish(RES* resp);

private:
	void new_connection(const TcpConnectionPtr& conn);

private:
	EventLoop* loop_;
	TcpClientPtr client_;
	ProtorpcChannelPtr chan_;

	DoneCallback done_cb_;
	DoingCallback doing_cb_;
};

template <typename REQ, typename RES, typename SRV>
void ProtorpcClient<REQ, RES, SRV>::finish(RES* res)
{
	{
		if (done_cb_) {
			done_cb_(res);
		} else {
			LOG(ERROR) << "ProtorpcClient::finish was not bind callback";
		}
		done_cb_ = DoneCallback();
	}
	client_->disconnect();
}

template <typename REQ, typename RES, typename SRV>
void ProtorpcClient<REQ, RES, SRV>::new_connection(const TcpConnectionPtr& conn)
{
	LOG(INFO) << "ProtorpcClient - " << conn->local_addr().to_ip_port() << " -> "
			<< conn->peer_addr().to_ip_port() << " s is "
			<< (conn->connected() ? "UP" : "DOWN");

	if (conn->connected()) {
		chan_->attach_connection(conn);
		{
			if (doing_cb_) {
				Stub stub{chan_.get()};
				REQ req;
				RES* res = new RES();
				doing_cb_(&stub, &req, res, NewCallback(this, &ProtorpcClient::finish, res));
			} else {
				LOG(ERROR) << "ProtorpcClient::new_connection was not bind callback";
			}
			doing_cb_ = DoingCallback();
		}
	} else {
		chan_->detach_connection();
		loop_->quit();
	}
}

}	// namespace annety

#endif	// ANT_PROTORPC_PROTORPC_CLIENT_H
