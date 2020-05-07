SET(DIR ${PROJECT_SOURCE_DIR}/src)

SET(HNET_SRCS
	${DIR}/StringPiece.cc ${DIR}/SafeStrerror.cc ${DIR}/StringSplit.cc ${DIR}/StringPrintf.cc ${DIR}/StringUtil.cc
	${DIR}/Logging.cc ${DIR}/LogStream.cc ${DIR}/TimeStamp.cc ${DIR}/ByteBuffer.cc
	${DIR}/MutexLock.cc ${DIR}/ConditionVariable.cc ${DIR}/CountDownLatch.cc
	${DIR}/PlatformThread.cc ${DIR}/Thread.cc
)

# StringPiece
ADD_EXECUTABLE(StringPiece_unittest StringPiece_unittest.cc ${HNET_SRCS})
TARGET_LINK_LIBRARIES(StringPiece_unittest ${GTEST_BOTH_LIBRARIES} pthread)
ADD_TEST(StringPiece ${PROJECT_BINARY_DIR}/bin/StringPiece_unittest)

# StringSplit
ADD_EXECUTABLE(StringSplit_unittest StringSplit_unittest.cc ${HNET_SRCS})
TARGET_LINK_LIBRARIES(StringSplit_unittest ${GTEST_BOTH_LIBRARIES} pthread)
ADD_TEST(StringSplit ${PROJECT_BINARY_DIR}/bin/StringSplit_unittest)

# StringPrintf
ADD_EXECUTABLE(StringPrintf_unittest StringPrintf_unittest.cc ${HNET_SRCS})
TARGET_LINK_LIBRARIES(StringPrintf_unittest ${GTEST_BOTH_LIBRARIES} pthread)
ADD_TEST(StringPrintf ${PROJECT_BINARY_DIR}/bin/StringPrintf_unittest)
