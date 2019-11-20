#ifndef ANT_PROTORPC_PROTORPC_CODEC_H
#define ANT_PROTORPC_PROTORPC_CODEC_H

#include "build/BuildConfig.h"
#include "protobuf/ProtobufCodec.h"
#include "protobuf/ProtobufDispatch.h"

#include "ProtorpcMessage.pb.h"

namespace annety
{
class ProtorpcMessage;
using ProtorpcMessagePtr = std::shared_ptr<ProtorpcMessage>;

}	// namespace annety

#endif	// ANT_PROTORPC_PROTORPC_CODEC_H
