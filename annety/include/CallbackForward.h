// By: wlmwang
// Date: Jun 17 2019

#ifndef ANT_CALLBACK_FORWARD_H_
#define ANT_CALLBACK_FORWARD_H_

#include "TimeStamp.h"

#include <string>
#include <functional>
#include <memory>
#include <stddef.h>

namespace annety
{
// Declare all user callback function prototype.
class NetBuffer;
class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using FinishCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, NetBuffer*, TimeStamp)>;
using ErrorCallback = std::function<void()>;
using TimerCallback = std::function<void()>;
using SignalCallback = std::function<void()>;

// Declare the client/server/connection object smart pointer.
class EventLoop;
class EndPoint;
class TcpClient;
class TcpServer;
class SelectableFD;
using TcpClientPtr = std::shared_ptr<TcpClient>;
using TcpServerPtr = std::shared_ptr<TcpServer>;
using SelectableFDPtr = std::unique_ptr<SelectableFD>;

TcpClientPtr make_tcp_client(EventLoop*, const EndPoint&, const std::string&);
TcpServerPtr make_tcp_server(EventLoop*, const EndPoint&, const std::string&, bool reuse_port = false);
TcpConnectionPtr make_tcp_connection(EventLoop*, const std::string&, SelectableFDPtr&&, const EndPoint&, const EndPoint&);

// internal ------------------------------------------------------------------
// Default callback handler
void default_connect_callback(const TcpConnectionPtr&);
void default_close_callback(const TcpConnectionPtr&);
void default_message_callback(const TcpConnectionPtr&, NetBuffer*, TimeStamp);

}	// namespace annety

#endif	// ANT_CALLBACK_FORWARD_H_
