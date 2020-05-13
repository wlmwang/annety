ADD_EXECUTABLE(netty_echoserver echo/server/server.cc echo/server/main.cc)
TARGET_LINK_LIBRARIES(netty_echoserver annety)

ADD_EXECUTABLE(netty_echoclient echo/client/client.cc echo/client/main.cc)
TARGET_LINK_LIBRARIES(netty_echoclient annety)


ADD_EXECUTABLE(netty_discardserver discard/server/server.cc discard/server/main.cc)
TARGET_LINK_LIBRARIES(netty_discardserver annety)

ADD_EXECUTABLE(netty_discardclient discard/client/client.cc discard/client/main.cc)
TARGET_LINK_LIBRARIES(netty_discardclient annety)
