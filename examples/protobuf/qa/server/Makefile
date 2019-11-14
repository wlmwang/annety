###############################
# simple makefile for test
###############################

CC		:= g++
CCFLAGS	:= -g -Wall -std=c++11 -rdynamic

# 第三方库
LIB_INC		:= -I../../../../annety/include -I/usr/local/proto3/include
DIR_LIB		:= -L../../../../ -L/usr/local/proto3/lib
LIBFLAGS	:= ${DIR_LIB} -lannety -lprotobuf -lpthread

# 源文件主目录
SRC_INC		:= ./../
DIR_SRC		:= ./

# 头文件
INCFLAGS	:= ${LIB_INC} -I${DIR_SRC} -I${SRC_INC}

# 源文件
CC_SRC	:= server.cc ../Query.pb.cc

# 编译文件
OBJ		:= $(patsubst %.cc, ${DIR_SRC}/%.o, $(notdir ${CC_SRC}))

TARGET	:= server

.PHONY:all clean

all: ${TARGET}

${TARGET}: ${OBJ}
	${CC} ${CCFLAGS} $^ -o $@ ${LIBFLAGS}

${DIR_SRC}/%.o:${DIR_SRC}/%.cc
	${CC} ${CCFLAGS} ${INCFLAGS} -c $< -o $@ 

${DIR_SRC}/%.o:${SRC_INC}/%.cc
	${CC} ${CCFLAGS} ${INCFLAGS} -c $< -o $@ 

clean:
	-rm -f ${TARGET} ${DIR_SRC}*.o
