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

class SelectableFD;
using SelectableFDPtr = std::unique_ptr<SelectableFD>;

// declare all user callback function prototype
class NetByteBuffer;
class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TimerCallback = std::function<void()>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, NetByteBuffer*, Time)>;

// default callback handler
void default_connection_callback(const TcpConnectionPtr&);
void default_message_callback(const TcpConnectionPtr&, NetByteBuffer*, Time);

}	// namespace annety

#endif	// ANT_CALLBACK_FORWARD_H_
