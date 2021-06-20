#include "general.hpp"

bool FileManagement::is_path_extension(const char* path, const char* extension) {
    size_t len = std::strlen(path);
    size_t extLen = std::strlen(extension);
    for (size_t i = 1; i <= extLen; i++) {
        if (path[len - i] != extension[extLen - i]) {
            return false;
        }
    }
    return true;
}

std::string FileManagement::change_extension(const char* path, const char* extension) {
    size_t len = std::strlen(path);
    for (size_t i = len - 1; i >= 0; i--) {
        if (path[i] == '.') {
            std::string res(path, i);
            res += extension;
            return res;
        }
    }
    fatal("Could not change extension of string: %s", path);
}
