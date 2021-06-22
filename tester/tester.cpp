#include <iostream>

#ifdef _WIN32
#include <tchar.h>
#include <windows.h>
#endif

#include "../src/game_boy.hpp"

namespace {

void test_rom(GameBoy& gameboy, std::string path, std::string passOut, std::string failOut) {
    gameboy.load(path.c_str());
    size_t pIndex = 0;
    size_t fIndex = 0;
    bool running = true;
    while (running) {
        gameboy.emulate_frame();
        if (gameboy.get_serial_out() == passOut[pIndex]) {
            if (++pIndex == passOut.size()) {
                running = false;
                std::cout << "x PASSED: " << path << std::endl;
            }
        } else {
            pIndex = 0;
        }
        if (gameboy.get_serial_out() == failOut[fIndex]) {
            if (++fIndex == failOut.size()) {
                running = false;
                std::cout << "  FAILED: " << path << std::endl;
            }
        } else {
            fIndex = 0;
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage gbtester [test directory]" << std::endl;
        return -1;
    }

#ifndef _WIN32
    std::cerr << "Tester only supported on windows" << std::endl;
    return -1;
#else
    std::string path(argv[1]);
    char cPath[MAX_PATH];
    WIN32_FIND_DATA ffd;
    if (_tcslen(path.c_str()) > MAX_PATH - 4) {
        std::cerr << "Directory path too long." << std::endl;
        return -1;
    }
    _tcsncpy(cPath, path.c_str(), MAX_PATH);
    if (cPath[_tcslen(cPath) - 1] != '\\') {
        _tcscat(cPath, "\\");
    }
    _tcscat(cPath, "*.*");

    HANDLE hFind = FindFirstFile(cPath, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Invalid handle value." << GetLastError() << std::endl;
        return -1;
    }

    GameBoy gameboy;
    Debugger debugger;
    gameboy.set_debugger(debugger);
    try {
        bool running = true;
        while (running) {
            if (_tcscmp(ffd.cFileName, ".") != 0 && _tcscmp(ffd.cFileName, "..") != 0) {
                if (FileManagement::is_path_extension(ffd.cFileName, "gb")) {
                    test_rom(gameboy, (path + "\\" + ffd.cFileName).c_str(), "\"", "B");
                } else if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    // std::cout << "dir:" << ffd.cFileName << std::endl;
                }
            }
            running = FindNextFile(hFind, &ffd);
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
#endif
}
