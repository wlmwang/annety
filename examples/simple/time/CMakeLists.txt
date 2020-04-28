ADD_EXECUTABLE(timeserver server/server.cc server/main.cc)
TARGET_LINK_LIBRARIES(timeserver annety)

ADD_EXECUTABLE(timeclient client/client.cc client/main.cc)
TARGET_LINK_LIBRARIES(timeclient annety)
