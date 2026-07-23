#include <sys/stat.h>

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

constexpr const char* s_init{"init"};
constexpr const char* s_add{"add"};

constexpr const char* empty_json_list{"[]"};

constexpr const char* ROOT_PATH{".vgit"};
constexpr const char* STAGE_PATH{".vgit/active_stage.json"};
constexpr __mode_t ROOT_PERMS{0777U};

enum class ERROR { NOT_INITED, CANT_INIT, CANT_ACCESS, COUNT };

enum class WARNING { ALREADY_DONE, COUNT };

enum class SUCCESS { INITED, STAGE_ADD, COUNT };

static constexpr std::array<std::string_view, static_cast<size_t>(ERROR::COUNT)>
    ERROR_MESSAGES{
        "vgit repository is uninitialized.\n run `vgit init` to initialize.",

        "vgit could not initialize repository here.\n check permissions, make "
        "sure there is no existing repository.",
        "Could not access file."};

static constexpr std::array<std::string_view,
                            static_cast<size_t>(WARNING::COUNT)>
    WARNING_MESSAGES{"Action was already done."};

static constexpr std::array<std::string_view,
                            static_cast<size_t>(SUCCESS::COUNT)>
    SUCCESS_MESSAGES{"vgit empty repository successfully initialized.",
                     "File successfully added to staging area"};

/* Checks if directory '.vgit' exists in CWD and is accessible */
inline bool is_inited() { return std::filesystem::is_directory(ROOT_PATH); }

/* Initializes an empty repository at this directory */
int init() { return !is_inited() && !mkdir(ROOT_PATH, ROOT_PERMS); }

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

constexpr nlohmann::json get_staging() {
    if (!std::filesystem::exists(STAGE_PATH)) {
        std::ofstream out{STAGE_PATH};
        out << empty_json_list;
        out.close();
    }

    std::ifstream in(STAGE_PATH);
    nlohmann::json loaded;
    in >> loaded;
    in.close();

    return loaded;
}

void save_staging(const nlohmann::json& json) {
    std::ofstream out(STAGE_PATH, std::ios::trunc);
    out << json;
    out.close();
}

int add(const char* filename) {
    std::fstream f{filename};
    if (!f.good()) return 0;
    f.close();

    auto json = get_staging();

    // check if already added
    for (const auto item : json) {
        if (!strcmp(item.get<std::string>().c_str(), filename)) return -1;
    }

    json.push_back(filename);

    save_staging(json);

    return 1;
}

int rm(const char* filename) {
    auto json = get_staging();

    for (auto it = json.begin(); it != json.end(); ++it) {
        if (!strcmp(it->get<std::string>().c_str(), filename)) {
            json.erase(it);
            save_staging(json);
            return 1;
        }
    }

    return -1;
}

auto add_resolver(int r) {
    switch (r) {
        case 0:
            return ERROR_MESSAGES[static_cast<size_t>(ERROR::CANT_ACCESS)];
        case -1:
            return WARNING_MESSAGES[static_cast<size_t>(WARNING::ALREADY_DONE)];
        case 1:
            return SUCCESS_MESSAGES[static_cast<size_t>(SUCCESS::STAGE_ADD)];
        default:
            return std::string_view{};
    }
}

auto rm_resolver(int r) { throw(std::logic_error("not implemented")); }

/*
Hi. In the first variation, there will only be support for vgit:
- add
- remove
- commit -m "..."
- push
*/
int main(int argc, char* argv[]) {
    if (argc < 2) return 0;  // should print description

    // case INIT

    if (!strcmp(argv[1], s_init)) {
        if (init())
            final(SUCCESS::INITED);
        else
            final(ERROR::CANT_INIT);
    }

    // case ADD

    if (!is_inited()) {
        final(ERROR::NOT_INITED);
    }

    if (!strcmp(argv[1], s_add)) {
        for (int i{2}; i < argc; ++i)
            std::cout << argv[i] << ": " << add_resolver(add(argv[i]))
                      << std::endl;

        return 0;

        // case OTHER

    } else {
        // should warn about unrecognized command
        ;
    }
}