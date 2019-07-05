// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_CALLBACK_FORWARD_H_
#define ANT_CALLBACK_FORWARD_H_

#include "Time.h"

#include <functional>
#include <memory>
#include <stddef.h>

namespace annety
{
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

// declare all user callback function prototype
class NetBuffer;
class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, NetBuffer*, Time)>;

// todo.
class Timer;
struct TimerId
{
	TimerId(Timer *timer, int64_t seq) 
		: timer_(timer), sequence_(seq) {}

	Timer* timer_;
	int64_t sequence_;
};
using TimerCallback = std::function<void()>;

// internal ------------------------------------------------------------------
// default callback handler
void default_connect_callback(const TcpConnectionPtr&);
void default_message_callback(const TcpConnectionPtr&, NetBuffer*, Time);

class SelectableFD;
using SelectableFDPtr = std::unique_ptr<SelectableFD>;

}	// namespace annety

#endif	// ANT_CALLBACK_FORWARD_H_
