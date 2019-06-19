###############################
# simple makefile for test
###############################

CC		:= g++ -g
CCFLAGS	:= -Wall -std=c++11 -rdynamic
ARFLAGS	:= -Wl,-dn #-Wl,-Bstatic
LDFLAGS	:= -Wl,-dy #-Wl,-Bdynamic

# 第三方库
DIR_INC		:= -I./
DIR_LIB		:= -L./
LIBFLAGS	:= ${DIR_LIB} -lpthread

# 源文件主目录
DIR_SRC		:= ./

# 头文件
INCFLAGS	:= ${DIR_INC} -I${DIR_SRC}

# 源文件
CC_SRC		:= main.cc \
			   LogStream.cc Logging.cc Time.cc AtExit.cc Exception.cc SafeStrerror.cc  \
			   MutexLock.cc ConditionVariable.cc CountDownLatch.cc \
			   PlatformThread.cc Thread.cc ThreadPool.cc \
			   StringPiece.cc StringPrintf.cc StringSplit.cc StringUtil.cc \
			   File.cc FilePath.cc FileEnumerator.cc FilesUtil.cc FilesUtilPosix.cc \
			   EndPoint.cc SocketsUtil.cc \
			   SelectableFD.cc SocketFD.cc \
			   EventLoop.cc \
			   Channel.cc Poller.cc Channel.cc EPollPoller.cc

# 编译文件
OBJ		:= $(patsubst %.cc, ${DIR_SRC}/%.o, $(notdir ${CC_SRC}))

TARGET	:= annety-test

.PHONY:all clean install

all: ${TARGET}

${TARGET}: ${OBJ}
	${CC} ${CCFLAGS} $^ -o $@ ${LIBFLAGS} 

${DIR_SRC}/%.o:${DIR_SRC}/%.cc
	${CC} ${CCFLAGS} -c $< -o $@ 

clean:
	-rm -f ${TARGET} ${DIR_SRC}*.o
	-rm -rf ./bin ./log
	
install:
	-rm -rf ./bin ./log
	-mkdir ./bin ./log
	-mv ${TARGET} ./bin