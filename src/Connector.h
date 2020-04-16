// By: wlmwang
// Date: Jun 30 2019

#ifndef ANT_CONNECTOR_H_
#define ANT_CONNECTOR_H_

#include "Macros.h"
#include "EndPoint.h"
#include "TimerId.h"
#include "CallbackForward.h"

#include <string>
#include <atomic>
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
	
	// *Thread safe*
	void retry();

	// *Thread safe*
	void restart();

	// *Not thread safe*, but usually be called before start().
	void set_new_connect_callback(NewConnectCallback cb)
	{
		new_connect_cb_ = std::move(cb);
	}
	// *Not thread safe*, but usually be called before start().
	void set_error_connect_callback(ErrorCallback cb)
	{
		error_connect_cb_ = std::move(cb);
	}

	const EndPoint& server_addr() const
	{
		return server_addr_;
	}

private:
	enum States {kDisconnected, kConnecting, kConnected};

	// *Not thread safe*, but run in own loop thread.
	void do_connect();
	void in_connect();

	// *Not thread safe*, but run in own loop thread.
	void start_in_own_loop();
	void stop_in_own_loop();
	void retry_in_own_loop();

	// *Not thread safe*, but run in own loop thread.
	void handle_write();
	void handle_error();

	// *Not thread safe*, but run in own loop thread.
	void reset_channel();
	void remove_and_reset_channel();

private:
	EventLoop* owner_loop_{nullptr};
	EndPoint server_addr_;

	std::atomic<bool> connect_{false};
	std::atomic<States> state_{kDisconnected};

	// For retry()
	int retry_delay_ms_;
	TimerId time_id_;

	// connect socket/channel
	SelectableFDPtr connect_socket_;
	std::unique_ptr<Channel> connect_channel_;

	NewConnectCallback new_connect_cb_;
	ErrorCallback error_connect_cb_;

	DISALLOW_COPY_AND_ASSIGN(Connector);
};

}	// namespace annety

#endif	// ANT_CONNECTOR_H_
