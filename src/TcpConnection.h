// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_TCP_CONNECTION_H_
#define ANT_TCP_CONNECTION_H_

#include "Macros.h"
#include "EndPoint.h"
#include "Time.h"
#include "CallbackForward.h"

#include <string>
#include <memory>

namespace annety
{
class Channel;
class EventLoop;
class SocketFD;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
	TcpConnection(EventLoop* loop,
				const std::string& name,
				SelectableFDPtr sockfd,
				const EndPoint& localaddr,
				const EndPoint& peeraddr);
	~TcpConnection();

	EventLoop* get_owner_loop() const { return owner_loop_; }
	const std::string& name() const { return name_; }

	

	void set_close_callback(const CloseCallback& cb)
	{
		close_cb_ = cb;
	}

	void connect_established();

	void connect_destroyed();

private:
	enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting};
	void handle_read(Time receive_tm);
	void handle_close();
	void handle_error();

	const char* state_to_string() const;

	EventLoop* owner_loop_;
	const std::string name_;
	StateE state_;

	SelectableFDPtr conn_socket_;
	std::unique_ptr<Channel> conn_channel_;

	CloseCallback close_cb_;

	DISALLOW_COPY_AND_ASSIGN(TcpConnection);
};

}	// namespace annety

#endif	// ANT_TCP_CONNECTION_H_
