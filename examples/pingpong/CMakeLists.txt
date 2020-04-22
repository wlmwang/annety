ADD_EXECUTABLE(pingpongserver server/server.cc)
TARGET_LINK_LIBRARIES(pingpongserver annety)

ADD_EXECUTABLE(pingpongclient client/client.cc)
TARGET_LINK_LIBRARIES(pingpongclient annety)
