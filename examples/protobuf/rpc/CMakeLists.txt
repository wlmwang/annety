IF (Protobuf_VERSION)
	MESSAGE(STATUS "Using Protocol Buffers ${Protobuf_VERSION}")
ENDIF()

INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

# Compile the proto.
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS Query.proto ${PROJECT_SOURCE_DIR}/include/protorpc/ProtorpcMessage.proto)

# Compile the object.
ADD_EXECUTABLE(protobuf_rpcserver server/server.cc ${PROTO_SRCS} ${PROTO_HDRS})
SET_TARGET_PROPERTIES(protobuf_rpcserver PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
TARGET_LINK_LIBRARIES(protobuf_rpcserver annety ${PROTOBUF_LIBRARIES})

ADD_EXECUTABLE(protobuf_rpcclient client/client.cc ${PROTO_SRCS} ${PROTO_HDRS})
SET_TARGET_PROPERTIES(protobuf_rpcclient PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
TARGET_LINK_LIBRARIES(protobuf_rpcclient annety ${PROTOBUF_LIBRARIES})
