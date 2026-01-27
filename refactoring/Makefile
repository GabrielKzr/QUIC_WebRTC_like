CC=gcc

INC_DIR=inc
SRC_DIR=src
OBJ_DIR=obj

CFLAGS=-Wall -Wextra -std=c11 -I${INC_DIR} -Wno-unused-parameter

SOURCES=$(shell find ${SRC_DIR} -name '*.c')
OBJECTS=${patsubst ${SRC_DIR}/%.c, ${OBJ_DIR}/%.o, ${SOURCES}}
#$(info ${SOURCES})

TARGET=main

all: ${TARGET}

${TARGET}: ${OBJECTS}
	${CC} ${CFLAGS} $^ -o $@

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c
	mkdir -p $(dir $@)
	${CC} ${CFLAGS} -c $< -o $@

.PHONY:
	all
	run
	clean

clean:
	rm -rf ${OBJ_DIR} ${TARGET}

run:
	@./${TARGET}