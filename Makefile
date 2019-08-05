###############################
# Makefile target libannety.a
###############################

#
# 环境要求:
# POSIX
# C++11
#

CC		:= g++
AR		:= ar
LD		:= g++
ARFLAGS := -pipe -fpic #-fpic 便于其他*.so库可以静态链接 ${LIBNAME}.a 库
LDFLAGS := -pipe -fpic
CCFLAGS	:= -g -Wall -std=c++11 #-DNDEBUG -O3

# install directory
INS_LIB	:= /usr/local/lib
INS_INC	:= /usr/local/include/annety

# linking library
LIB_INC		:= -I./
DIR_LIB		:= -L./
LIBFLAGS	:= ${DIR_LIB} -lpthread

# 源文件主目录
SRC_INC		:= ./annety/include
DIR_SRC		:= ./src

# 头文件 -I flags
INCFLAGS	:= ${LIB_INC} -I${SRC_INC} -I${DIR_SRC}

# 源文件 *.cc
CC_SRC	:= $(wildcard ${DIR_SRC}/*.cc)

# 目标文件
LIBNAME	:= libannety
AR_LIB	:= ${LIBNAME}.a
LN_LIB	:= ${LIBNAME}.so
LD_LIB	:= ${LIBNAME}.so.0.0.1
SN_LIB	:= ${LIBNAME}.so.0

# 编译目标文件暂存目录
DIR_AR	:= ${DIR_SRC}/ar
DIR_LD	:= ${DIR_SRC}/ld

# 编译文件
AR_OBJ	:= $(patsubst %.cc, ${DIR_AR}/%.o, $(notdir ${CC_SRC}))
LD_OBJ	:= $(patsubst %.cc, ${DIR_LD}/%.o, $(notdir ${CC_SRC}))

# $(info $(AR_OBJ))

.PHONY:all clean install

#all: ${AR_LIB} ${LD_LIB}
all: ${AR_LIB}

# 编译目标
${AR_LIB}:${AR_OBJ}
	@echo "Compiling $@"
	${AR} -rcs $@ $^
	
${LD_LIB}:$(LD_OBJ)
	@echo "Compiling $@"
	${LD} -shared -Wl,-soname,${SN_LIB} ${CCFLAGS} -o $@ $^ ${LIBFLAGS}

# 编译ar
${DIR_AR}/%.o:${DIR_SRC}/%.cc
	@echo "Compiling $@"
	${CC} ${CCFLAGS} ${INCFLAGS} ${ARFLAGS} -c $< -o $@

# 编译ld
${DIR_LD}/%.o:${DIR_SRC}/%.cc
	@echo "Compiling $@"
	${CC} ${CCFLAGS} ${INCFLAGS} ${LDFLAGS} -c $< -o $@

clean:
	@echo "Clean and Rebuild"
	-rm -rf ${DIR_SRC}/*.o ${DIR_AR}/*.o ${DIR_LD}/*.o
	-rm -rf ${AR_LIB} ${LD_LIB} ${LN_LIB} ${SN_LIB}
	
install:
	@echo "Install LIB and HEADER"
#	-cp -f ${LIBNAME}.*	${INS_LIB}
#	-ln -fs ${LD_LIB} ${INS_LIB}/${LN_LIB}
