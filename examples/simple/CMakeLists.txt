ADD_EXECUTABLE(echoserver echo/echo.cc echo/main.cc)
TARGET_LINK_LIBRARIES(echoserver annety)

ADD_EXECUTABLE(daytimeserver daytime/daytime.cc daytime/main.cc)
TARGET_LINK_LIBRARIES(daytimeserver annety)

ADD_EXECUTABLE(discardserver discard/discard.cc discard/main.cc)
TARGET_LINK_LIBRARIES(discardserver annety)

ADD_SUBDIRECTORY(time)
ADD_SUBDIRECTORY(chargen)
