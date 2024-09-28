#pragma once
#include <string>
#ifdef HASHED_KEYS
typedef size_t hash_t;
#else
typedef std::string hash_t;
#endif
namespace utils {
    hash_t Hash(const std::string& s);
    hash_t Hash(const char* s);
}