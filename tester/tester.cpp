#include <cstring>
#include <filesystem>
#include <iostream>
#include <vector>

#include "game_boy.hpp"

namespace {
GameBoy gameboy;
Debugger debugger;

size_t numPassed = 0;
size_t numTests;
const std::vector<std::string> excludeExtensions = {
    "A", "cgb0", "dmg0", "S", "mgb", "sgb", "sgb2",
};

void test_dir(const std::filesystem::path& file, const std::vector<std::string>& excludeList) {
    std::vector<std::string> files;
    for (const auto& file : std::filesystem::directory_iterator(file)) {
        bool doExclude = false;
        for (const auto& excludeFile : excludeList) {
            if (!file.path().filename().compare(excludeFile)) {
                doExclude = true;
                break;
            }
        }
        if (doExclude) {
            continue;
        }
        if (file.is_directory()) {
            test_dir(file.path(), excludeList);
        } else {
            std::string extension(file.path().extension().string());
            if (extension == ".gb" || extension == ".gbc") {
                std::string filePathStr(file.path().string());
                size_t i = 0;
                for (; i < excludeExtensions.size(); i++) {
                    if (filePathStr.substr(filePathStr.find_last_of("-") + 1,
                                           excludeExtensions[i].length()) == excludeExtensions[i]) {
                        break;
                    }
                }
                if (i == excludeExtensions.size()) {
                    files.push_back(file.path().string());
                }
            }
        }
    }
    if (files.size() > 0) {
        std::cout << " --- " << file.relative_path() << " --- " << std::endl;
        for (size_t i = 0; i < files.size(); i++) {
            std::cout << "[";
            if (i + 1 < 10) {
                std::cout << "0";
            }
            std::cout << i + 1 << "/" << files.size() << "] ";

            gameboy.load(files[i].c_str());
            bool running = true;
            while (running) {
                gameboy.emulate_frame();
                if (debugger.is_paused()) {
                    if (debugger.check_fib_in_registers()) {
                        std::cout << "x PASSED: " << files[i].c_str() << std::endl;
                        numPassed++;
                    } else {
                        std::cout << "  FAILED: " << files[i].c_str() << std::endl;
                    }
                    running = false;
                    debugger.continue_exec();
                }
            }
        }
        numTests += files.size();
    }
}
}  // namespace

int main(int argc, char** argv) {
    gameboy.set_debugger(debugger);
    try {
        if (argc == 1) {
            const std::vector<std::string> testRomPaths = {
                // "SameSuite",
                "mooneye-gb_hwtests\\acceptance",
                // "mooneye-gb_hwtests\\acceptance\\ppu",
                // "mooneye-gb_hwtests\\emulator-only",
                "mooneye-gb_hwtests\\misc",
                // "wilbertpol",
            };
            const std::vector<std::string> excludeList = {
                "multicart_rom_8Mb.gb",
            };

            for (const auto& path : testRomPaths) {
                test_dir(std::filesystem::path("tester\\roms\\" + path), excludeList);
            }
        } else if (argc == 2) {
            test_dir(std::filesystem::path(argv[1]), std::vector<std::string>());
        } else {
            std::cout << "Usage: gbemu-tester [optional test directory]\tRuns all tests when no "
                         "args provided"
                      << std::endl;
            return -1;
        }
        std::cout << "PASSED: [" << numPassed << "/" << numTests << "] tests" << std::endl;
        if (numPassed < numTests) {
            return -1;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}
