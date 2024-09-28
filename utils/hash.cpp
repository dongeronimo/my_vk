#include "hash.h"
#include <functional>
std::hash<std::string> hash_str_fn;
hash_t utils::Hash(const char* s) {
#ifdef HASHED_KEYS
    std::string str(s);
    return utils::Hash(str);
#else
    return std::string(s);
#endif
}
hash_t utils::Hash(const std::string& s)
{
#ifdef HASHED_KEYS
    return hash_str_fn(s);
#else
    return s;
#endif
}
