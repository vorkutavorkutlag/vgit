#pragma once

#include <filesystem>
namespace fs = std::filesystem;

namespace vgit::Consts {
constexpr size_t commit_hash_length{40uz};

constexpr std::string_view BOOM{
    "    ____  ____  ____  __  ___   __\n"
    "   / __ )/ __ \\/ __ \\/  |/  /  / /\n"
    "  / __  / / / / / / / /|_/ /  / / \n"
    " / /_/ / /_/ / /_/ / /  / /  /_/  \n"
    "/_____/\\____/\\____/_/  /_/  (_)   \n"};

constexpr std::string_view empty_json_list{"[]"};
constexpr std::string_view empty_json_dict{"{}"};
constexpr std::string_view hex_digits{"0123456789abcdef"};

constexpr std::string_view s_active_branch{"active_branch"};
constexpr std::string_view s_head_hash{"head_hash"};
constexpr std::string_view delta_extension{".vdelta"};
constexpr std::string_view tmp_extension{".tmp"};

/* constant full paths */

const fs::path CWD{fs::current_path()};
const fs::path VGIT_ROOT{CWD / ".vgit"};
const fs::path BRANCHES_PATH{VGIT_ROOT / "branches"};
const fs::path GLOBAL_INFO_PATH{VGIT_ROOT / "global_info.json"};

/* partial paths, must be concatenated with relevant branch */

const fs::path p_stage_path{"active_stage"};
const fs::path p_commit_history_path{"commit_history.json"};
const fs::path p_branch_info_path{"branch_info.json"};
const fs::path p_commit_message_path{".commit_message.txt"};

constexpr __mode_t VGIT_PERMS{0777U};
}  // namespace vgit::Consts