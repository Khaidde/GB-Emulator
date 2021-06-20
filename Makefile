BUILD_TYPE ?= debug

BIN_DIR := build/bin
OBJ_DIR := build/obj

SDL2_INCLUDE := C:\SDL2\include
SDL2_LIB := C:\SDL2\lib\x64


SOURCES := $(shell ls src/*.cpp)
OBJECTS := $(patsubst src/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
DEPS := $(patsubst src/%.cpp, $(OBJ_DIR)/%.d, $(SOURCES))
-include ${DEPS}

EXEC := gbemu.exe

PLATFORM := $(shell uname -s)
ifneq ($(PLATFORM), windows32)
${warning Build has not been test on ${PLATFORM}}
endif

ifeq ($(PLATFORM), windows32)
MKDIR := $(shell which mkdir)
RM := $(shell which rm)
else
MKDIR := mkdir
RM := rm
endif

ifeq ($(shell which clang++),)
$(error Could not find path to clang)
else
CC := clang
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

TESTER_DIR := tester/build

all: build

build: ${BIN_DIR}/${EXEC} ${BIN_DIR}/SDL2.dll

${BIN_DIR}/${EXEC}: ${OBJECTS}
	@${MKDIR} -p ${dir $@} ||:
	${CC} ${CFLAGS} -L${SDL2_LIB} ${LDFLAGS} -Wl,/subsystem:console $^ -o $@

${OBJ_DIR}/%.o: src/%.cpp
	@${MKDIR} -p ${dir $@} ||:
	${CC} ${CFLAGS} -I${SDL2_INCLUDE} -MMD -MF $(@:.o=.d) -c $< -o $@

${BIN_DIR}/SDL2.dll:
	cp ${SDL2_LIB}/SDL2.dll $@

clean:
	${RM} -f ${EXEC} ${OBJECTS} ${DEPS} ${BIN_DIR}/*
