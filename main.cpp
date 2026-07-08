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

constexpr const char* VGIT_RD{".vgit"};
constexpr const char* STAGE_D{".vgit/active_stage.json"};
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
inline bool is_inited() { return std::filesystem::is_directory(VGIT_RD); }

/* Initializes an empty repository at this directory */
int init() { return !is_inited() && !mkdir(VGIT_RD, ROOT_PERMS); }

/* The finals are killers. One evil, one good.*/

void final(ERROR e) {
    std::cout << ERROR_MESSAGES[static_cast<size_t>(e)] << std::endl;
    exit(static_cast<int>(e));
}

void final(SUCCESS s) {
    std::cout << SUCCESS_MESSAGES[static_cast<size_t>(s)] << std::endl;
    exit(0);
}

int add(const char* filename) {
    std::fstream f{filename};
    if (!f.good()) return 0;
    f.close();

    if (!std::filesystem::exists(STAGE_D)) {
        std::ofstream out{STAGE_D};
        out << empty_json_list;
    }

    std::ifstream in(STAGE_D);
    nlohmann::json loaded;
    in >> loaded;
    in.close();

    for (const auto item : loaded) {
        if (!strcmp(item.get<std::string>().c_str(), filename)) return -1;
    }

    // loaded.push_back(filename);

    std::ofstream out(STAGE_D, std::ios::trunc);
    out << loaded;

    return 1;
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

/*
Hi. In the first variation, there will only be support for vgit:
- add
- remove
- commit -m "..."
- push
*/
int main(int argc, char* argv[]) {
    if (argc < 2) return 0;  // should print description

    if (!strcmp(argv[1], s_init)) {
        if (init())
            final(SUCCESS::INITED);
        else
            final(ERROR::CANT_INIT);
    }

    if (!is_inited()) {
        final(ERROR::NOT_INITED);
    }

    if (!strcmp(argv[1], s_add)) {
        for (int i{2}; i < argc; ++i)
            std::cout << argv[i] << ": " << add_resolver(add(argv[i]))
                      << std::endl;

        return 0;

    } else {
        // should warn about unrecognized command
        ;
    }
}