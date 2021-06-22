BUILD_TYPE ?= debug

BIN_DIR ?= build/bin
OBJ_DIR ?= build/obj

SDL2_INCLUDE ?= C:\SDL2\include
SDL2_LIB ?= C:\SDL2\lib\x64

SOURCES := $(shell ls src/*.cpp)
OBJECTS := $(patsubst src/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
DEPS := $(patsubst src/%.cpp, $(OBJ_DIR)/%.d, $(SOURCES))
-include ${DEPS}

EXEC := gbemu.exe

PLATFORM := $(shell uname -s)
ifneq ($(findstring MSYS,$(PLATFORM)),)
PLATFORM := windows32
endif

ifneq ($(PLATFORM),windows32)
${warn Build not test on ${PLATFORM}}
MKDIR := mkdir
RM := rm
else
MKDIR := $(shell which mkdir)
RM := $(shell which rm)
endif

ifeq ($(shell which clang++),)
$(error Could not find path to clang++)
else
CC := clang++
CFLAGS += -std=c++17 -Werror -Wpedantic -Wsign-conversion
CFLAGS += -Wno-gnu-anonymous-struct -Wno-language-extension-token
LDFLAGS := -lshell32 -lSDL2main -lSDL2
endif

ifeq (${BUILD_TYPE}, debug)
CFLAGS += -g
endif
ifeq (${BUILD_TYPE}, release)
CFLAGS += -O3
endif

TESTER_DIR := tester
TESTER_BIN_DIR := ${TESTER_DIR}/build/bin
TESTER_OBJ_DIR := ${TESTER_DIR}/build/obj

all: build test

build: ${BIN_DIR}/${EXEC} ${BIN_DIR}/SDL2.dll

${BIN_DIR}/${EXEC}: ${OBJECTS}
	@${MKDIR} -p ${dir $@}
	${CC} ${CFLAGS} $^ -o $@ -L${SDL2_LIB} ${LDFLAGS} -Wl,--subsystem,console

${OBJ_DIR}/%.o: src/%.cpp
	@${MKDIR} -p ${dir $@}
	${CC} ${CFLAGS} $< -o $@ -I${SDL2_INCLUDE} -MMD -MF $(@:.o=.d) -c

${BIN_DIR}/SDL2.dll:
	cp ${SDL2_LIB}/SDL2.dll $@

test: ${TESTER_BIN_DIR}/gbemu-tester.exe ${TESTER_BIN_DIR}/SDL2.dll

${TESTER_BIN_DIR}/gbemu-tester.exe: ${TESTER_OBJ_DIR}/tester.o ${OBJECTS}
	@${MKDIR} -p ${dir $@}
	${CC} ${CFLAGS} $^ -o $@ -L${SDL2_LIB} ${LDFLAGS} -Wl,--subsystem,console

${TESTER_OBJ_DIR}/tester.o: ${TESTER_DIR}/tester.cpp
	@${MKDIR} -p ${dir $@}
	${CC} ${CFLAGS} $< -o $@ -I${SDL2_INCLUDE} -c

${TESTER_BIN_DIR}/SDL2.dll:
	cp ${SDL2_LIB}/SDL2.dll $@

clean:
	${RM} -f ${OBJECTS} ${DEPS} ${BIN_DIR}/*
