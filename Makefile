###############################
# makefile
###############################

#
# 环境要求
# C++11
# Linux2.6+ 或 Linux2.4+附epoll补丁
#

CC		:= g++ -g
AR		:= ar
LD		:= g++
ARFLAGS := -fpic -pipe -fno-ident  # -fpic 便于其他*.so库可以静态链接 ${LIBNAME}.a 库
LDFLAGS := -fpic -pipe -fno-ident
CCFLAGS	:= -Wall -std=c++11 #-DNDEBUG -O3

# 第三方库
LIB_INC		:= -I./
DIR_LIB		:= -L./
LIBFLAGS	:= ${DIR_LIB} -lpthread

# 源文件主目录
SRC_INC		:= ./annety/include
DIR_SRC		:= ./src

# 头文件
INCFLAGS	:= ${LIB_INC} -I${SRC_INC} -I${DIR_SRC}

# 源文件
CC_SRC	:= $(wildcard ${DIR_SRC}/*.cc)

# 编译目标文件暂存目录
DIR_AR	:= ${DIR_SRC}/ar
DIR_LD	:= ${DIR_SRC}/ld

# 目标文件
LIBNAME	:= libannety
AR_LIB	:= ${LIBNAME}.a
LN_LIB	:= ${LIBNAME}.so
LD_LIB	:= ${LIBNAME}.so.0.0.1
SN_LIB	:= ${LIBNAME}.so.0

# 编译文件
OBJ	:= $(patsubst %.cc, ${DIR_SRC}/%.o, $(notdir ${CC_SRC}))
#AR_OBJ	:= $(patsubst %.cc, ${DIR_SRC}/%.o, $(notdir ${CC_SRC}))
#LD_OBJ	:= $(patsubst %.cc, ${DIR_SRC}/%.o, $(notdir ${CC_SRC}))

#$(warning  $(AR_OBJ))

# 安装目录
INS_LIB	:= /usr/local/lib
INS_INC	:= /usr/local/include/annety

.PHONY:all clean install

all: ${AR_LIB} ${LD_LIB}

# 编译目标
${AR_LIB}:${OBJ}
	@echo "Compiling $@"
	${AR} -rcs $@ $^
	
${LD_LIB}:$(OBJ)
	@echo "Compiling $@"
	${LD} -shared -Wl,-soname,${SN_LIB} -o $@ $^ ${CCFLAGS} ${LIBFLAGS}

# 编译ar
${DIR_SRC}/%.o:${DIR_SRC}/%.cc
	@echo "Compiling $@"
	${CC} ${CCFLAGS} ${INCFLAGS} -c $< -o $@ ${ARFLAGS}

# 编译ld
${DIR_SRC}/%.o:${DIR_SRC}/%.cc
	@echo "Compiling $@"
	${CC} ${CCFLAGS} ${INCFLAGS} -g -c $< -o $@ ${LDFLAGS}

clean:
	@echo "Clean and Rebuild"
	-rm -rf ${DIR_SRC}/*.o
	-rm -rf ${AR_LIB} ${LN_LIB} ${LD_LIB} ${SN_LIB}
	
install:
	@echo "Install LIB and HEADER"
	-cp -f ${LIBNAME}.*	${INS_LIB}
	-ln -fs ${LD_LIB} ${INS_LIB}/${LN_LIB}
