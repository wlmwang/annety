ADD_EXECUTABLE(netty_echoserver echo/server/server.cc echo/server/main.cc)
TARGET_LINK_LIBRARIES(netty_echoserver annety)

ADD_EXECUTABLE(netty_echoclient echo/client/client.cc echo/client/main.cc)
TARGET_LINK_LIBRARIES(netty_echoclient annety)
