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
ADD_TEST(AnnetyTests ${PROJECT_BINARY_DIR}/bin/StringPiece_unittest)
