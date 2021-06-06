#include "general.hpp"

bool FileManagement::is_path_extension(const char* romPath, const char* extension) {
    size_t len = std::strlen(romPath);
    size_t extLen = std::strlen(extension);
    for (size_t i = 1; i <= extLen; i++) {
        if (romPath[len - i] != extension[extLen - i]) {
            return false;
        }
    }
    return true;
}