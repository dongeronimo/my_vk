#include "hash.h"
#include <functional>
std::hash<std::string> hash_str_fn;
size_t utils::Hash(const char* s) {
    std::string str(s);
    return utils::Hash(str);
}
size_t utils::Hash(const std::string& s)
{
    return hash_str_fn(s);
}
