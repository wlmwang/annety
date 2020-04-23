ADD_EXECUTABLE(chargenserver server/server.cc server/main.cc)
TARGET_LINK_LIBRARIES(chargenserver annety)

ADD_EXECUTABLE(chargenclient client/client.cc client/main.cc)
TARGET_LINK_LIBRARIES(chargenclient annety)
