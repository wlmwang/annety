IF (Protobuf_VERSION)
	MESSAGE(STATUS "Using Protocol Buffers ${Protobuf_VERSION}")
ENDIF()

INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

# Compile the proto.
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS Query.proto)

# Compile the object.
ADD_EXECUTABLE(protobuf_codecserver server/server.cc ${PROTO_SRCS} ${PROTO_HDRS})
SET_TARGET_PROPERTIES(protobuf_codecserver PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
TARGET_LINK_LIBRARIES(protobuf_codecserver annety ${PROTOBUF_LIBRARIES})

ADD_EXECUTABLE(protobuf_codecclient client/client.cc ${PROTO_SRCS} ${PROTO_HDRS})
SET_TARGET_PROPERTIES(protobuf_codecclient PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
TARGET_LINK_LIBRARIES(protobuf_codecclient annety ${PROTOBUF_LIBRARIES})
