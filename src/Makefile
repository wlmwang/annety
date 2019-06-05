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
CC_SRC		:= main.cc StringPiece.cc StringPrintf.cc SafeStrerror.cc Time.cc \
			   LogStream.cc Logging.cc MutexLock.cc ConditionVariable.cc \
			   CountDownLatch.cc PlatformThread.cc Thread.cc Exception.cc \
			   ThreadPool.cc File.cc AtExit.cc SafeSprintf.cc StringUtil.cc \
			   StringSplit.cc FilePath.cc FileEnumerator.cc

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
	-rm -f ${TARGET} ${DIR_SRC}/*.o
	-rm -rf ./bin ./log
	
install:
	-rm -rf ./bin ./log
	-mkdir ./bin ./log
	-mv ${TARGET} ./bin