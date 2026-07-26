#include <bits/stdc++.h>
#include <sys/stat.h>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

constexpr const char* s_init{"init"};
constexpr const char* s_branch{"branch"};
constexpr const char* s_add{"add"};
constexpr const char* s_rm{"rm"};
constexpr const char* s_active_branch{"active_branch"};
constexpr const char* s_delete_flag1{"-d"};
constexpr const char* s_delete_flag2{"-D"};

constexpr const char* empty_json_list{"[]"};
constexpr const char* empty_json_dict{"{}"};

const fs::path ROOT_PATH{".vgit"};
const fs::path STAGE_PATH = ROOT_PATH / "active_stage.json";
const fs::path BRANCHES_PATH = ROOT_PATH / "branches";
const fs::path INFO_PATH = ROOT_PATH / "active_info.json";
constexpr __mode_t ROOT_PERMS{0777U};

enum class ERROR {
    NOT_INITED,
    CANT_INIT,
    CANT_ACCESS,
    NO_BRANCH,
    CANT_CREATE,
    NO_SUICIDE,
    COUNT
};

enum class WARNING { NO_EFFECT, COUNT };

enum class SUCCESS { INITED, STAGE_ADD, STAGE_RM, BRANCH, DELETE, COUNT };

static constexpr std::array<std::string_view, static_cast<size_t>(ERROR::COUNT)>
    ERROR_MESSAGES{
        "vgit repository is uninitialized.\n run `vgit init` to initialize.",

        "vgit could not initialize repository here.\n check permissions, make "
        "sure there is no existing repository.",
        "Could not access item.",
        "No active branch selected. To create, vgit branch <name>",
        "Could not create new.",
        "Switch branches before deleting this branch."};

static constexpr std::array<std::string_view,
                            static_cast<size_t>(WARNING::COUNT)>
    WARNING_MESSAGES{"Action had no effect"};

static constexpr std::array<std::string_view,
                            static_cast<size_t>(SUCCESS::COUNT)>
    SUCCESS_MESSAGES{"vgit empty repository successfully initialized.",
                     "File successfully added to staging area.",
                     "File successfully removed from staging area.",
                     "Successfully created branch.", "Successfully deleted."};

/* The finals are killers. One evil, one good.*/

void final(ERROR e) {
    std::cout << ERROR_MESSAGES[static_cast<size_t>(e)] << std::endl;
    exit(static_cast<int>(e));
}

void final(SUCCESS s) {
    std::cout << SUCCESS_MESSAGES[static_cast<size_t>(s)] << std::endl;
    exit(0);
}

/*---------------------------------------------------------------------------*/

constexpr nlohmann::json get_info() {
    if (!fs::exists(INFO_PATH)) {
        std::ofstream out{INFO_PATH};
        out << empty_json_dict;
        out.close();
    }

    std::ifstream in{INFO_PATH};
    nlohmann::json loaded;
    in >> loaded;
    in.close();

    return loaded;
}

constexpr nlohmann::json get_staging() {
    if (!fs::exists(STAGE_PATH)) {
        std::ofstream out{STAGE_PATH};
        out << empty_json_list;
        out.close();
    }

    std::ifstream in{STAGE_PATH};
    nlohmann::json loaded;
    in >> loaded;
    in.close();

    return loaded;
}

constexpr void save_staging(const nlohmann::json& json) {
    std::ofstream out{STAGE_PATH, std::ios::trunc};
    out << json;
    out.close();
}

constexpr void save_info(const nlohmann::json& json) {
    std::ofstream out{INFO_PATH, std::ios::trunc};
    out << json;
    out.close();
}

// returns a random 20 byte hash
constexpr std::string get_random_hash() {
    std::string h;
    for (size_t i{0uz}; i < 20; ++i) h.push_back(rand() % 16);
    return h;
}

/*---------------------------------------------------------------------------*/

/* Checks if directory '.vgit' exists in CWD and is accessible */
inline bool is_inited() { return fs::is_directory(ROOT_PATH); }

bool has_branch() {
    auto json = get_info();
    auto it = json.find(s_active_branch);
    if (it == json.end()) return false;
    std::string alleged_branch{BRANCHES_PATH};
    alleged_branch += *it;
    return fs::is_directory(alleged_branch);
}

std::string get_branch() {
    auto json = get_info();
    auto it = json.find(s_active_branch);
    if (it == json.end()) return std::string{};
    return *it;
}

void set_branch(const std::string& name) {
    auto json = get_info();
    json[s_active_branch] = name;
    save_info(json);
}

/* Initializes an empty repository at this directory */
int init() {
    return !is_inited() && !mkdir(ROOT_PATH.c_str(), ROOT_PERMS) &&
           !mkdir(BRANCHES_PATH.c_str(), ROOT_PERMS);
}

/*---------------------------------------------------------------------------*/

int add(const char* filename) {
    std::fstream f{filename};
    if (!f.good()) return 0;
    f.close();

    auto json = get_staging();

    // check if already added
    for (const auto& item : json) {
        if (!strcmp(item.get<std::string>().c_str(), filename)) return -1;
    }

    json.push_back(filename);

    save_staging(json);

    return 1;
}

int rm(const char* filename) {
    std::fstream f{filename};
    if (!f.good()) return 0;
    f.close();

    auto json = get_staging();

    for (auto it{json.begin()}; it != json.end(); ++it) {
        if (!strcmp(it->get<std::string>().c_str(), filename)) {
            json.erase(it);
            save_staging(json);
            return 1;
        }
    }

    return -1;
}

int branch(const char* name) {
    fs::path alleged_branch{BRANCHES_PATH};
    alleged_branch /= name;
    if (fs::is_directory(alleged_branch)) return 0;
    if (mkdir(alleged_branch.c_str(), ROOT_PERMS)) return 0;
    set_branch(alleged_branch);
    return 1;
};

int d_branch(const char* name) {
    fs::path alleged_branch{BRANCHES_PATH};
    alleged_branch /= name;
    if (!fs::is_directory(alleged_branch)) return 0;
    if (!alleged_branch.filename().compare(name)) return -1;

    return fs::remove_all(alleged_branch);
}

// pushes a new commit to the commit stack
int commit(const char* message) {
    const std::string h{get_random_hash()};
    const std::string m{message};
    const auto json = get_staging();
    return 0;
}

auto add_resolver(int r) {
    switch (r) {
        case 0:
            return ERROR_MESSAGES[static_cast<size_t>(ERROR::CANT_ACCESS)];
        case -1:
            return WARNING_MESSAGES[static_cast<size_t>(WARNING::NO_EFFECT)];
        case 1:
            return SUCCESS_MESSAGES[static_cast<size_t>(SUCCESS::STAGE_ADD)];
        default:
            return std::string_view{};
    }
}

auto rm_resolver(int r) {
    switch (r) {
        case 0:
            return ERROR_MESSAGES[static_cast<size_t>(ERROR::CANT_ACCESS)];
        case -1:
            return WARNING_MESSAGES[static_cast<size_t>(WARNING::NO_EFFECT)];
        case 1:
            return SUCCESS_MESSAGES[static_cast<size_t>(SUCCESS::STAGE_RM)];
        default:
            return std::string_view{};
    }
}

auto branch_resolver(int r) {
    switch (r) {
        case 0:
            return ERROR_MESSAGES[static_cast<size_t>(ERROR::CANT_CREATE)];
        case 1:
            return SUCCESS_MESSAGES[static_cast<size_t>(SUCCESS::BRANCH)];
        default:
            return std::string_view{};
    }
}

auto d_branch_resolver(int r) {
    switch (r) {
        case 0:
            return ERROR_MESSAGES[static_cast<size_t>(ERROR::CANT_ACCESS)];
        case -1:
            return ERROR_MESSAGES[static_cast<size_t>(ERROR::NO_SUICIDE)];
        case 1:
            return SUCCESS_MESSAGES[static_cast<size_t>(SUCCESS::DELETE)];
        default:
            return std::string_view{};
    }
}

/*
Hi. In the first variation, there will only be support for vgit:
- add
- remove
- commit -m "..."
- push
*/

int main(int argc, char* argv[]) {
    srand(std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count());

    if (argc < 2) return 0;  // should print description

    /* --------- INIT ---------*/

    if (!strcmp(argv[1], s_init)) {
        if (init())
            final(SUCCESS::INITED);
        else
            final(ERROR::CANT_INIT);
    }

    if (!is_inited()) {
        final(ERROR::NOT_INITED);
    }

    /*  allow things like status and whatever here.
        anything that impacts the staging area is forbidden. */

    /* --------- BRANCH ---------*/

    if (!strcmp(argv[1], s_branch)) {
        if (argc == 2) {
            // display branches and current
            return 0;
        }

        bool delete_mode = false;

        for (int i{2}; i < argc; ++i) {
            if (!strcmp(argv[i], s_delete_flag1) ||
                !strcmp(argv[i], s_delete_flag2))
                delete_mode = ++i;

            if (!delete_mode)
                std::cout << "Branch '" << argv[i]
                          << "': " << branch_resolver(branch(argv[i]))
                          << std::endl;
            else
                std::cout << "Branch '" << argv[i]
                          << "': " << d_branch_resolver(d_branch(argv[i]))
                          << std::endl;
        }

        return 0;
    }

    if (!has_branch()) {
        final(ERROR::NO_BRANCH);
    }

    /* --------- ADD --------- */

    if (!strcmp(argv[1], s_add)) {
        for (int i{2}; i < argc; ++i)
            std::cout << argv[i] << ": " << add_resolver(add(argv[i]))
                      << std::endl;

        return 0;
    }

    /* --------- REMOVE --------- */

    if (!strcmp(argv[1], s_rm)) {
        for (int i{2}; i < argc; ++i)
            std::cout << argv[i] << ": " << rm_resolver(rm(argv[i]))
                      << std::endl;

        return 0;
    }

    else {
        // should warn about unrecognized command
    }
}