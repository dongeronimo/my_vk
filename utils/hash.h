#pragma once
#include <string>
namespace utils {
    size_t Hash(const std::string& s);
    size_t Hash(const char* s);
}