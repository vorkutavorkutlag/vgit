#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vgit/constants.hpp>

namespace vgit {
class Environment {
   public:
    static std::string get_active_branch();
    [[nodiscard]] static bool set_active_branch(std::string_view branch);

    static std::string get_head_hash();
    [[nodiscard]] static bool set_head_hash(std::string_view hash);

    static bool valid_file_scope(const fs::path& fp);
    static bool valid_branch_scope(const fs::path& fp);
    [[nodiscard]] static bool update_history(std::string_view new_commit_hash);

    static nlohmann::json get_commit_history();

   private:
    static nlohmann::json get_global_info();
    [[nodiscard]] static bool set_global_info(const nlohmann::json& info);

    [[nodiscard]] static bool ensure_json_list(const fs::path& fp);
    [[nodiscard]] static bool ensure_json_dict(const fs::path& fp);

    [[nodiscard]] static bool remove_commit_unsafe(
        std::string_view commit_hash);

    static nlohmann::json get_branch_info();
    [[nodiscard]] static bool set_branch_info(const nlohmann::json& info);
};
}  // namespace vgit