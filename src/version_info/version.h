#ifndef VERSION_H
#define VERSION_H

#include <string_view>

namespace version {
    extern const std::string_view git_hash;
    extern const std::string_view git_tag;
    extern const std::string_view git_branch;
    extern const std::string_view git_commit_time;
    extern const std::string_view build_time;
}; // namespace version

#endif // VERSION_H
