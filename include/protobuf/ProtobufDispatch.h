// By: wlmwang
// Date: Oct 12 2019

#ifndef ANT_PROTOBUF_PROTOBUF_DISPATCH_H
#define ANT_PROTOBUF_PROTOBUF_DISPATCH_H

#include "Macros.h"
#include "Logging.h"
#include "TimeStamp.h"
#include "TcpConnection.h"
#include "CallbackForward.h"

#include <map>
#include <utility>
#include <functional>
#include <type_traits>

#include <google/protobuf/message.h>

namespace annety
{
using MessagePtr = std::shared_ptr<google::protobuf::Message>;
using ProtobufMessageCallback = 
		std::function<void(const TcpConnectionPtr&, const MessagePtr&, TimeStamp)>;

// Dispather Callback
class Callback
{
public:
	virtual ~Callback() = default;
	virtual void operator()(const TcpConnectionPtr&, const MessagePtr&, TimeStamp) const = 0;
};

template <typename T>
class CallbackT : public Callback
{
// compile-time assertion checking
static_assert(std::is_base_of<google::protobuf::Message, T>::value,
		"T must be derived from google::protobuf::Message.");

public:
	using ProtobufMessageTCallback = 
			std::function<void(const TcpConnectionPtr&, const std::shared_ptr<T>&, TimeStamp)>;

	CallbackT(const ProtobufMessageTCallback& cb) : cb_(cb) {}

	void operator()(const TcpConnectionPtr& conn, const MessagePtr& mesg, TimeStamp receive) const override
	{
		std::shared_ptr<T> concrete = std::dynamic_pointer_cast<T>(mesg);
		CHECK(concrete);
		
		cb_(conn, concrete, receive);
	}

private:
	ProtobufMessageTCallback cb_;
};

// A simple dispatcher of protobuf message.
class ProtobufDispatch
{
public:
	ProtobufDispatch(ProtobufMessageCallback cb = ProtobufDispatch::unknown) 
		: default_cb_(std::move(cb)) {}

	// Register protobuf message callbacks.
	template <typename T>
	void add(const typename CallbackT<T>::ProtobufMessageTCallback& cb)
	{
		std::shared_ptr<CallbackT<T>> pd(new CallbackT<T>(cb));
		cbs_[T::descriptor()] = pd;
	}

	void dispatch(const TcpConnectionPtr&, const MessagePtr&, TimeStamp) const;

private:
	static void unknown(const TcpConnectionPtr&, const MessagePtr&, TimeStamp);

private:
	using CallbackMap = 
			std::map<const google::protobuf::Descriptor*, std::shared_ptr<Callback>>;

	// Protobuf callback lists.
	// [
	//		T::descriptor() => callback,
	// ]
	CallbackMap cbs_;

	ProtobufMessageCallback default_cb_;

	DISALLOW_COPY_AND_ASSIGN(ProtobufDispatch);
};

void ProtobufDispatch::dispatch(const TcpConnectionPtr& conn, const MessagePtr& mesg, TimeStamp receive) const
{
	CallbackMap::const_iterator it = cbs_.find(mesg->GetDescriptor());
	if (it != cbs_.end()) {
		(*it->second)(conn, mesg, receive);
	} else {
		// Unknown protobuf message.
		if (default_cb_) {
			default_cb_(conn, mesg, receive);
		} else {
			LOG(ERROR) << "ProtobufDispatch::dispatch Invalid message, TypeName=" 
				<< mesg->GetTypeName();
		}
	}
}

void ProtobufDispatch::unknown(const TcpConnectionPtr&, const MessagePtr& mesg, TimeStamp)
{
	LOG(WARNING) << "ProtobufDispatch::unknown - " << mesg->GetTypeName();
}

}	// namespace annety

#endif  // ANT_PROTOBUF_PROTOBUF_DISPATCH_H
