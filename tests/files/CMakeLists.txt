SET(DIR ${PROJECT_SOURCE_DIR}/src)

SET(HNET_SRCS
	${DIR}/StringPiece.cc ${DIR}/SafeStrerror.cc ${DIR}/StringSplit.cc ${DIR}/StringPrintf.cc ${DIR}/StringUtil.cc
	${DIR}/Logging.cc ${DIR}/LogStream.cc ${DIR}/TimeStamp.cc ${DIR}/ByteBuffer.cc
	${DIR}/MutexLock.cc ${DIR}/ConditionVariable.cc ${DIR}/CountDownLatch.cc
	${DIR}/PlatformThread.cc ${DIR}/Thread.cc
	${DIR}/File.cc ${DIR}/FilePath.cc ${DIR}/FileEnumerator.cc ${DIR}/FileUtil.cc ${DIR}/FileUtilPosix.cc
)

# FilePath
ADD_EXECUTABLE(FilePath_unittest FilePath_unittest.cc ${HNET_SRCS})
TARGET_LINK_LIBRARIES(FilePath_unittest ${GTEST_BOTH_LIBRARIES} pthread)
ADD_TEST(AnnetyUnitTests-files ${PROJECT_BINARY_DIR}/bin/FilePath_unittest)
