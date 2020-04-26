// By: wlmwang
// Date: Oct 18 2019

#ifndef ANT_PROTORPC_PROTORPC_CHANNEL_H
#define ANT_PROTORPC_PROTORPC_CHANNEL_H

#include "EventLoop.h"
#include "Logging.h"
#include "synchronization/MutexLock.h"
#include "protobuf/ProtobufCodec.h"
#include "protobuf/ProtobufDispatch.h"

// Define the protorpc bytes protocol.
#include "ProtorpcMessage.pb.h"

#include <map>
#include <string>
#include <atomic>
#include <memory>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

namespace annety
{
class ProtorpcMessage;
using ProtorpcMessagePtr = std::shared_ptr<ProtorpcMessage>;

class ProtorpcChannel;
using ProtorpcChannelPtr = std::shared_ptr<ProtorpcChannel>;

class ProtorpcChannel : public ::google::protobuf::RpcChannel
{
public:
	explicit ProtorpcChannel(EventLoop* loop)
		: loop_(loop)
		, codec_(loop, 
			std::bind(
				&ProtobufDispatch::dispatch, 
				&dispatch_, 
				std::placeholders::_1, 
				std::placeholders::_2, 
				std::placeholders::_3
			)
		)
	{
		CHECK(loop);

		LOG(DEBUG) << "ProtorpcChannel::ProtorpcChannel - " << this;

		using std::placeholders::_1;
		using std::placeholders::_2;
		using std::placeholders::_3;

		// Register protorpc protocol callbacks.
		dispatch_.listen<ProtorpcMessage>(
			std::bind(&ProtorpcChannel::dispatch, this, _1, _2, _3));
	}

	virtual ~ProtorpcChannel() override
	{
		LOG(DEBUG) << "ProtorpcChannel::~ProtorpcChannel - " << this;
		
		for (const auto& outstanding : outstandings_) {
			OutstandingCall out = outstanding.second;
			delete out.resp;
		}
	}

	// Register protorpc service impl.
	void listen(::google::protobuf::Service* service)
	{
		const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
		services_[desc->full_name()] = service;
	}

	void attach_connection(const TcpConnectionPtr& conn)
	{
		CHECK(conn);
		conn_ = conn;

		using std::placeholders::_1;
		using std::placeholders::_2;
		using std::placeholders::_3;

		conn_->set_message_callback(
			std::bind(&ProtorpcChannel::recv, this, _1, _2, _3));
	}
	
	void detach_connection()
	{
		CHECK(conn_);
		conn_.reset();
	}
	
	void recv(const TcpConnectionPtr& conn, NetBuffer* buff, TimeStamp receive)
	{
		CHECK(conn == conn_);
		codec_.recv(conn_, buff, receive);
	}

	// Call the given method of the remote service.  The signature of this
	// procedure looks the same as Service::CallMethod(), but the requirements
	// are less strict in one important way:  the request and response objects
	// need not be of any specific class as long as their descriptors are
	// method->input_type() and method->output_type().
	virtual void CallMethod(const ::google::protobuf::MethodDescriptor* method,
							::google::protobuf::RpcController* controller,
							const ::google::protobuf::Message* request,
							::google::protobuf::Message* response,
							::google::protobuf::Closure* done) override
	{
		CHECK(conn_);

		LOG(DEBUG) << "ProtorpcChannel::CallMethod req - " << request->DebugString();
		
		int64_t id = ++id_;

		ProtorpcMessage mesg;
		mesg.set_id(id);
		mesg.set_service(method->service()->full_name());
		mesg.set_method(method->name());
		mesg.set_request(request->SerializeAsString());	// FIXME: error check
		mesg.set_type(ProtorpcMessage::REQUEST);

		// Save request context.
		OutstandingCall out = { response, done};
		{
			AutoLock locked(lock_);
			outstandings_[id] = out;
		}

		codec_.send(conn_, mesg);
	}

private:
	void dispatch(const TcpConnectionPtr&, const ProtorpcMessagePtr&, TimeStamp);
	void request(const ProtorpcMessage&);
	void response(const ProtorpcMessage&);
	void error();

	void finish(::google::protobuf::Message*, int64_t);

	// for protorpc request.
	struct OutstandingCall
	{
		::google::protobuf::Message* resp;
		::google::protobuf::Closure* done;
	};

private:
	EventLoop* loop_{nullptr};
	TcpConnectionPtr conn_;

	// Protobuf bytes codec.
	ProtobufDispatch dispatch_;
	ProtobufCodec codec_;

	std::atomic<int64_t> id_{0};

	// Protorpc client lists.
	MutexLock lock_;
	std::map<int64_t, OutstandingCall> outstandings_;

	// Protorpc service lists.
	// [
	//		'service name' => service*,
	// ]
	std::map<std::string, ::google::protobuf::Service*> services_;
};

void ProtorpcChannel::dispatch(const TcpConnectionPtr& conn, const ProtorpcMessagePtr& mesg, TimeStamp)
{
	CHECK(conn == conn_);

	LOG(INFO) << "ProtorpcChannel::dispatch - " << mesg->DebugString();

	switch (static_cast<int>(mesg->type())) {
		case ProtorpcMessage::REQUEST:
			request(*mesg);
			break;

		case ProtorpcMessage::RESPONSE:
			response(*mesg);
			break;

		case ProtorpcMessage::ERROR:
			// TODO
			break;
	}
}

void ProtorpcChannel::response(const ProtorpcMessage& mesg)
{
	CHECK(conn_);

	LOG(DEBUG) << "ProtorpcChannel::response res - " << mesg.DebugString();

	OutstandingCall out = { nullptr, nullptr};
	{
		AutoLock locked(lock_);
		std::map<int64_t, OutstandingCall>::iterator it = outstandings_.find(mesg.id());
		if (it != outstandings_.end()) {
			out = it->second;
			outstandings_.erase(it);
		}
	}

	if (out.resp) {
		// RAII
		std::unique_ptr<google::protobuf::Message> l(out.resp);

		out.resp->ParseFromString(mesg.response());
		if (out.done) {
			out.done->Run();
		}
	}
}

void ProtorpcChannel::request(const ProtorpcMessage& mesg)
{
	CHECK(conn_);

	LOG(DEBUG) << "ProtorpcChannel::request req - " << mesg.DebugString();

	auto failed = [] (ProtorpcMessage::ERROR_CODE err, const ProtorpcMessage& mesg) {
		ProtorpcMessage resp;
		resp.set_id(mesg.id());
		resp.set_error(err);
		resp.set_type(ProtorpcMessage::RESPONSE);

		codec_.send(conn_, resp);
	};

	// Init error code.
	ProtorpcMessage::ERROR_CODE err = ProtorpcMessage::WRONG_PROTO;

	// Lookup listen impl services.
	std::map<std::string, google::protobuf::Service*>::const_iterator it = services_.find(mesg.service());
	if (it == services_.end()) {
		err = ProtorpcMessage::NO_SERVICE;
		failed(err, mesg);
		return;
	}

	// Service descriptor by impl.
	google::protobuf::Service* impl = it->second;
	const google::protobuf::ServiceDescriptor* desc = impl->GetDescriptor();
	CHECK(desc);

	// Method descriptor by method name.
	const google::protobuf::MethodDescriptor* method = impl->GetDescriptor()->FindMethodByName(mesg.method());
	if (!method) {
		err = ProtorpcMessage::NO_METHOD;
		failed(err, mesg);
		return;
	}

	// RAII
	std::unique_ptr<google::protobuf::Message> req(impl->GetRequestPrototype(method).New());
	
	// Parse request.
	if (!req->ParseFromString(mesg.request())) {
		err = ProtorpcMessage::INVALID_REQUEST;
		failed(err, mesg);
		return;
	}

	// res is deleted in finish().
	google::protobuf::Message* res = impl->GetResponsePrototype(method).New();

	// Tips:
	// Service::CallMethod is not Channel::CallMethod
	impl->CallMethod(method, nullptr, req.get(), res, 
				NewCallback(this, &ProtorpcChannel::finish, res, mesg.id()));
}

void ProtorpcChannel::finish(::google::protobuf::Message* res, int64_t id)
{
	CHECK(conn_);

	LOG(DEBUG) << "ProtorpcChannel::finish res - " << res->DebugString();

	// RAII to release the res*.
	std::unique_ptr<google::protobuf::Message> l(res);

	ProtorpcMessage mesg;
	mesg.set_id(id);
	mesg.set_response(res->SerializeAsString()); // FIXME: error check
	mesg.set_type(ProtorpcMessage::RESPONSE);

	codec_.send(conn_, mesg);
}

}	// namespace annety

#endif	// ANT_PROTORPC_PROTORPC_CHANNEL_H
