CC=gcc

APP ?= 

TEST_DIR=test_examples/WebRTC

ifneq ($(APP),)
    CFLAGS += -DAPP_$(APP)
    APP_SRC=$(TEST_DIR)/test_$(shell echo $(APP) | tr A-Z a-z).c
    SOURCES += $(APP_SRC)
endif

INC_DIR=inc
SRC_DIR=src
OBJ_DIR=${BUILD_DIR}/obj
BUILD_DIR=build

CFLAGS+=-Wall -Wextra -std=c11 -I${INC_DIR} -I/usr/local/include -Wno-unused-parameter
LDFLAGS=-L/usr/local/lib64
LDLIBS=-lssl -lcrypto

SOURCES=$(shell find ${SRC_DIR} -name '*.c')
SOURCES += $(APP_SRC)

OBJECTS=$(addprefix ${OBJ_DIR}/, $(SOURCES:.c=.o))
#$(info ${SOURCES})

TARGET=${BUILD_DIR}/main

all: build ${TARGET}

build:
	mkdir -p ${BUILD_DIR}

${TARGET}: ${OBJECTS}
	${CC} ${CFLAGS} $^ -o $@ ${LDFLAGS} ${LDLIBS}

${OBJ_DIR}/%.o: %.c
	mkdir -p $(dir $@)
	${CC} ${CFLAGS} -c $< -o $@

.PHONY:
	all
	run
	clean
	build

clean:
	rm -rf ${BUILD_DIR}

run:
	@./${TARGET}