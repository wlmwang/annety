###############################
# simple makefile for test
###############################

CC		:= g++
CCFLAGS	:= -g -Wall -std=c++11 -rdynamic

# 第三方库
LIB_INC		:= -I./
DIR_LIB		:= -L./
LIBFLAGS	:= ${DIR_LIB} -lpthread

# 源文件主目录
DIR_SRC		:= ./

# Annety
ANT_INC		:= ../../../annety/include
ANT_DIR		:= ../../

# 头文件
INCFLAGS	:= ${LIB_INC} -I${DIR_SRC} -I${ANT_INC} -I${ANT_DIR}

# 源文件
CC_SRC	:=	$(wildcard ${DIR_SRC}/*.cc)
CC_ANT	:=	Logging.cc LogStream.cc TimeStamp.cc ByteBuffer.cc \
			StringPiece.cc SafeStrerror.cc StringSplit.cc StringPrintf.cc StringUtil.cc \
			MutexLock.cc ConditionVariable.cc CountDownLatch.cc \
			PlatformThread.cc Thread.cc \
			SocketsUtil.cc \
			SelectableFD.cc EventFD.cc TimerFD.cc SignalFD.cc \
			Channel.cc Timer.cc \
			SignalServer.cc \
			TimerPool.cc Poller.cc PollPoller.cc EPollPoller.cc \
			EventLoop.cc

# 编译文件
OBJ	:= $(patsubst %.cc, ${DIR_SRC}/%.o, $(notdir ${CC_SRC})) $(patsubst %.cc, ${DIR_SRC}/%.o, $(notdir ${CC_ANT}))

TARGET	:= testing

.PHONY:all clean

all: ${TARGET}

${TARGET}: ${OBJ}
	${CC} ${CCFLAGS} $^ -o $@ ${LIBFLAGS}

${DIR_SRC}/%.o:${DIR_SRC}%.cc
	${CC} ${CCFLAGS} ${INCFLAGS} -c $< -o $@ 

${DIR_SRC}/%.o:${ANT_DIR}%.cc
	${CC} ${CCFLAGS} ${INCFLAGS} -c $< -o $@ 

clean:
	-rm -f ./${TARGET} ${DIR_SRC}*.o
