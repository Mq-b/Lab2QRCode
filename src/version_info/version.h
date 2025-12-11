#ifndef VERSION_H
#define VERSION_H

#include <string_view>

/**
 * @namespace version
 * @brief 提供应用程序的版本信息，包括 Git 提交哈希、标签、分支和构建时间、系统版本、架构信息
 * @note 这些仅仅是声明，实际定义由构建系统执行 version_info.ps1 脚本生成 version.cpp
 */
namespace version {
extern const std::string_view git_hash;
extern const std::string_view git_tag;
extern const std::string_view git_branch;
extern const std::string_view git_commit_time;
extern const std::string_view build_time;
extern const std::string_view system_version;
extern const std::string_view kernel_version;
extern const std::string_view architecture;
}; // namespace version

#endif // VERSION_H
