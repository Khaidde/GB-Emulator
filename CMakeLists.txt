cmake_minimum_required(VERSION 3.0.0)
set (CMAKE_CXX_STANDARD 17)

project(
    GBEmulator
    VERSION 1.0
    LANGUAGES CXX)

set(CMAKE_PREFIX_PATH
    ${CMAKE_PREFIX_PATH}
    C:/CDev/SDL2_MinGW_32Bit
)

find_package(SDL2 REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src)

set(SOURCES
    src/cartridge.cpp
    src/cpu.cpp
    src/game_boy.cpp
    src/main.cpp
    src/memory.cpp
    src/ppu.cpp
)
add_executable(gbemu ${SOURCES})

target_include_directories(gbemu PUBLIC ${SDL2_INCLUDE_DIRS})
target_link_libraries(gbemu ${SDL2_LIBRARIES})
