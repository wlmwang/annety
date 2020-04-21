ADD_EXECUTABLE(chargenserver server/chargen.cc server/main.cc)
TARGET_LINK_LIBRARIES(chargenserver annety)

ADD_EXECUTABLE(chargenclient client/chargen.cc client/main.cc)
TARGET_LINK_LIBRARIES(chargenclient annety)
