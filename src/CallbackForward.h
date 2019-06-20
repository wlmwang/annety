// Refactoring: Anny Wang
// Date: Jun 17 2019

#ifndef ANT_CALLBACK_FORWARD_H_
#define ANT_CALLBACK_FORWARD_H_

#include "Time.h"

#include <functional>
#include <memory>

namespace annety
{
// All client visible callbacks go here.

class Buffer;
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

// user callback
typedef std::function<void()> TimerCallback;
typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

// the data has been read to (buf, len)
typedef std::function<void (const TcpConnectionPtr&, Buffer*, Time)> MessageCallback;

// default handler
void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer, Time receiveTime);

}	// namespace annety

#endif	// ANT_CALLBACK_FORWARD_H_
