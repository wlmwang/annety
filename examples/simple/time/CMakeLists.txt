ADD_EXECUTABLE(timeserver server/times.cc server/main.cc)
TARGET_LINK_LIBRARIES(timeserver annety)

ADD_EXECUTABLE(timeclient client/times.cc client/main.cc)
TARGET_LINK_LIBRARIES(timeclient annety)
