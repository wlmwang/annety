ADD_SUBDIRECTORY(simple)
ADD_SUBDIRECTORY(asio)
ADD_SUBDIRECTORY(netty)
ADD_SUBDIRECTORY(pingpong)

# https://cmake.org/cmake/help/v3.0/module/FindProtobuf.html
FIND_PACKAGE(Protobuf)

IF (PROTOBUF_FOUND)
	ADD_SUBDIRECTORY(protobuf)
ENDIF()
