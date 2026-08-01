#include <bits/stdc++.h>
#include <sys/stat.h>

#include <nlohmann/json.hpp>

#include "CLI11.hpp"

namespace fs = std::filesystem;

namespace vconsts {
constexpr const char* empty_json_list{"[]"};
constexpr const char* empty_json_dict{"{}"};

constexpr const char* s_active_branch{"active_branch"};

const fs::path CWD{fs::current_path()};
const fs::path VGIT_ROOT{CWD / ".vgit"};
const fs::path BRANCHES_PATH{VGIT_ROOT / "branches"};
const fs::path STAGE_PATH_P{"active_stage"};
const fs::path INFO_PATH{VGIT_ROOT / "active_info.json"};
constexpr __mode_t VGIT_PERMS{0777U};
}  // namespace vconsts

enum class Status { Success, Warning, Error };

struct Result {
    Status status;
    std::string_view message;
};

[[noreturn]] inline void finally(Result res) {
    std::cout << res.message << std::endl;
    exit(static_cast<int>(res.status));
}

nlohmann::json get_active_info() {
    if (!fs::exists(vconsts::INFO_PATH)) {
        std::ofstream out{vconsts::INFO_PATH};
        out << vconsts::empty_json_dict;
        out.close();
    }

    std::ifstream in{vconsts::INFO_PATH};
    nlohmann::json loaded;
    in >> loaded;
    in.close();

    return loaded;
}

void set_active_info(const nlohmann::json& json) {
    std::ofstream out{vconsts::INFO_PATH, std::ios::trunc};
    out << json;
    out.close();
}

/* Assumes file exists */
bool valid_file_scope(const fs::path& fpath) {
    if (fpath.is_absolute()) {
        // if it is not within CWD, return false
        fs::path itpath{fpath};
        while (itpath.has_parent_path()) {
            itpath = itpath.parent_path();
            // disallow adding .vgit files to vgit stage
            if (itpath == vconsts::VGIT_ROOT) return false;
            if (itpath == vconsts::CWD) return true;
        }
        return false;
    }

    // path is relative
    return fpath.root_directory() != vconsts::VGIT_ROOT;
}

/* ------------------------------ */
inline bool is_inited() { return fs::is_directory(vconsts::VGIT_ROOT); }

std::string get_active_branch() {
    auto json = get_active_info();
    auto it = json.find(vconsts::s_active_branch);
    if (it == json.end()) return std::string{};
    return fs::path(*it).filename();
}

Result set_active_branch(const std::string& b_name) {
    if (!fs::is_directory(vconsts::BRANCHES_PATH / b_name))
        return {Status::Error, "Branch does not exist."};
    auto json = get_active_info();
    json[vconsts::s_active_branch] = b_name;
    set_active_info(json);
    return {Status::Success, "Successfully switched branch."};
}

inline auto get_branches() {
    return fs::directory_iterator(vconsts::BRANCHES_PATH);
}

Result handle_init() {
    if (!is_inited() &&
        !mkdir(vconsts::VGIT_ROOT.c_str(), vconsts::VGIT_PERMS) &&
        !mkdir(vconsts::BRANCHES_PATH.c_str(), vconsts::VGIT_PERMS))
        return {Status::Success, "Successfully initialized empty repository."};
    return {Status::Error, "Could not initialize new empty repository here."};
}

Result handle_branch(const std::string& b_name) {
    fs::path would_be{vconsts::BRANCHES_PATH / b_name};
    if (fs::is_directory(would_be))
        return {Status::Error, "Branch by same name already exists."};
    if (mkdir(would_be.c_str(), vconsts::VGIT_PERMS))
        return {Status::Error, "Could not create new branch."};
    set_active_branch(b_name);
    mkdir((would_be / vconsts::STAGE_PATH_P).c_str(), vconsts::VGIT_PERMS);
    return {Status::Success, "Created new branch & switched."};
}

Result handle_branch() {
    static std::string all_branches;
    const std::string active_branch = get_active_branch();

    if (active_branch.empty()) return {Status::Success, all_branches};

    for (const auto& b : get_branches()) {
        const std::string b_name = b.path().filename().generic_string();
        if (active_branch == b_name) all_branches += " + ";
        (all_branches += b_name) += "\n";
    }

    all_branches.pop_back();

    return {Status::Success, all_branches};
}

Result handle_dbranch(const std::string& b_name) {
    if (get_active_branch() == b_name)
        return {Status::Error, "Switch to different branch before deleting."};
    if (!fs::remove_all(vconsts::BRANCHES_PATH / b_name))
        return {Status::Error, "Failed to remove the specified branch."};
    return {Status::Success, "Branch successfully deleted."};
}

Result handle_switch(const std::string& b_name) {
    return set_active_branch(b_name);
}

Result handle_nuke() {
    std::cout << "Are you sure you want to delete the current "
                 "repository?\n[y/N]\n";
    std::string input;
    std::getline(std::cin, input);
    if (input != "y" && input != "Y") return {Status::Success, "Aborted nuke."};

    if (!fs::remove_all(vconsts::VGIT_ROOT))
        return {Status::Error, "Failed to remove the repository."};
    return {Status::Success, "BOOM!"};
}

Result handle_add(const std::vector<std::string>& files,
                  const std::string& active_branch) {
    static std::string result_message;
    if (files.empty())
        return {Status::Warning, "No files specified, nothing changed"};

    for (const auto& file : files) {
        result_message.append(file).append(": ");
        const fs::path fpath{file};
        if (!fs::exists(fpath)) {
            result_message.append("File does not exist.\n");
            continue;
        }

        if (!valid_file_scope(file)) {
            result_message.append("File is out of repository scope.\n");
            continue;
        }

        const fs::path destination = vconsts::BRANCHES_PATH / active_branch /
                                     vconsts::STAGE_PATH_P /
                                     fpath.relative_path();
        fs::create_directories(destination.parent_path());
        fs::copy(
            fpath, destination,
            fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        result_message.append("File successfully added to stage.\n");
    }

    result_message.pop_back();

    return {Status::Success, result_message};
}

/* ------------------------------ */

int main(int argc, char* argv[]) {
    CLI::App app{"vgit - the barebones local git"};

    auto* init = app.add_subcommand("init", "Initialize repository");

    std::string branch_name;
    bool delete_branch{false};
    auto* branch = app.add_subcommand("branch", "Manage branches");
    branch->add_option("name", branch_name, "Branch name");
    branch->add_flag("-d,-D,--delete", delete_branch, "Delete a branch");

    std::string switch_name;
    auto* swtch = app.add_subcommand("switch", "Switch branch");
    swtch->add_option("name", switch_name, "Branch name");

    std::vector<std::string> add_files;
    auto* add = app.add_subcommand("add", "Add files to stage");
    add->add_option("file", add_files, "File to add");

    auto* nuke =
        app.add_subcommand("nuke", "Delete repository in working directory");

    CLI11_PARSE(app, argc, argv);

    if (*init) finally(handle_init());

    if (!is_inited())
        finally(
            {Status::Error, "Initialize repository before further action."});

    if (*nuke) finally(handle_nuke());

    if (*branch) {
        if (branch_name.empty()) finally(handle_branch());
        if (delete_branch) finally(handle_dbranch(branch_name));
        finally(handle_branch(branch_name));
    }

    if (*swtch) finally(handle_switch(switch_name));

    const std::string active_branch{get_active_branch()};

    if (active_branch.empty())
        finally({Status::Error, "Select branch before further action."});

    if (*add) finally(handle_add(add_files, active_branch));
}