BUILD_TYPE ?= debug

BIN_DIR ?= build/bin
OBJ_DIR ?= build/obj

SDL2_INCLUDE ?= C:/SDL2/include
SDL2_LIB ?= C:/SDL2/lib/x64

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
CFLAGS += -O3 -DDEBUG=false
endif

TEST_BIN_DIR := tester/build/bin
TEST_OBJ_DIR := tester/build/obj

TEST_OBJECTS := $(patsubst src/%.cpp, $(TEST_OBJ_DIR)/%.o, $(SOURCES))
TEST_OBJECTS += ${TEST_OBJ_DIR}/tester.o
TEST_DEPS := $(patsubst src/%.cpp, $(TEST_OBJ_DIR)/%.d, $(SOURCES))
TEST_DEPS += ${TEST_OBJ_DIR}/tester.d
-include ${DEPS}

define create_exe
	@${MKDIR} -p ${dir $@}
	${CC} ${CFLAGS} -L${SDL2_LIB} $^ -o $@ ${LDFLAGS} -Wl,--subsystem,console
endef

define build_objs
	@${MKDIR} -p ${dir $@}
	${CC} ${CFLAGS} -I${SDL2_INCLUDE} $< -o $@ -MMD -MF $(@:.o=.d) -c
endef

all: build test

# Build
build: ${BIN_DIR}/${EXEC}
	cp ${SDL2_LIB}/SDL2.dll ${BIN_DIR}/SDL2.dll

${BIN_DIR}/${EXEC}: ${OBJ_DIR}/icon.o ${OBJECTS}
	$(call create_exe)

${OBJ_DIR}/icon.o: resources/icon.rc resources/gb-icon.ico
	windres $< -o $@

${OBJ_DIR}/%.o: src/%.cpp
	$(call build_objs)

# Testing
test: ${TEST_BIN_DIR}/gbemu-tester.exe
	cp ${SDL2_LIB}/SDL2.dll ${TEST_BIN_DIR}/SDL2.dll
	${TEST_BIN_DIR}/gbemu-tester.exe

${TEST_BIN_DIR}/gbemu-tester.exe: ${TEST_OBJECTS}
	$(call create_exe)

${TEST_OBJ_DIR}/tester.o: tester/tester.cpp
	$(eval CFLAGS += -DLOG=false -I./src)
	$(call build_objs)

${TEST_OBJ_DIR}/%.o: src/%.cpp
	$(eval CFLAGS += -DLOG=false)
	$(call build_objs)

# Clean
clean:
	${RM} -f ${OBJ_DIR}/* ${BIN_DIR}/*
	${RM} -f ${TEST_OBJ_DIR}/* ${TEST_BIN_DIR}/*
