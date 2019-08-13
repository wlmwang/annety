// Refactoring: Anny Wang
// Date: Jun 30 2019

#ifndef ANT_CONNECTOR_H_
#define ANT_CONNECTOR_H_

#include "Macros.h"
#include "EndPoint.h"
#include "CallbackForward.h"

#include <string>
#include <memory>
#include <functional>
#include <utility>

namespace annety
{
class EventLoop;
class EndPoint;
class SelectableFD;
class Channel;

class Connector : public std::enable_shared_from_this<Connector>
{
public:
	using NewConnectCallback = std::function<void(SelectableFDPtr, const EndPoint&)>;

	Connector(EventLoop* loop, const EndPoint& addr);
	~Connector();

	// *Thread safe*
	void start();

	// *Thread safe*
	void stop();
	
	// FIXME: *Not thread safe*
	// you must be call this in loop thread
	void restart();  

	void set_new_connect_callback(NewConnectCallback cb)
	{
		new_connect_cb_ = std::move(cb);
	}

	const EndPoint& server_addr() const
	{
		return server_addr_;
	}

private:
	enum States {kDisconnected, kConnecting, kConnected};

	void start_in_own_loop();
	void stop_in_own_loop();
	void connect();
	void connecting();
	void retry();

	void handle_write();
	void handle_error();

	void remove_and_reset_channel();
	void reset_channel();

private:
	EventLoop* owner_loop_{nullptr};
	EndPoint server_addr_;

	bool connect_{false}; // atomic
	States state_{kDisconnected};  // FIXME: use atomic variable
	int retry_delay_ms_;

	SelectableFDPtr connect_socket_;
	std::unique_ptr<Channel> connect_channel_;

	NewConnectCallback new_connect_cb_;

	DISALLOW_COPY_AND_ASSIGN(Connector);
};

}	// namespace annety

#endif	// ANT_CONNECTOR_H_
