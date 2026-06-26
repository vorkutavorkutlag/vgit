#include <sys/stat.h>

#include <array>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

constexpr const char* s_init{"init"};
constexpr const char* s_add{"add"};

constexpr const char* VGIT_RD{".vgit"};
constexpr const __mode_t ROOT_PERMS{0777U};

enum class ERROR { NOT_INITED, CANT_INIT, COUNT };

enum class SUCCESS { INITED, COUNT };

static constexpr std::array<std::string_view, static_cast<size_t>(ERROR::COUNT)>
    ERROR_MESSAGES{
        "vgit repository is uninitialized.\n run `vgit init` to initialize.",

        "vgit could not initialize repository here.\n check permissions, make "
        "sure there is no existing repository.",
    };

static constexpr std::array<std::string_view,
                            static_cast<size_t>(SUCCESS::COUNT)>
    SUCCESS_MESSAGES{"vgit empty repository successfully initialized."};

/* Checks if directory '.vgit' exists in CWD and is accessible */
bool is_inited() {
    struct stat info;
    return (stat(VGIT_RD, &info)) && (info.st_mode & S_IFDIR);
}

/* Initializes an empty repository at this directory */
int init() { return !is_inited() && !mkdir(VGIT_RD, ROOT_PERMS); }

void final(ERROR e) {
    std::cout << ERROR_MESSAGES[static_cast<size_t>(e)] << std::endl;
    exit(static_cast<int>(e));
}

void final(SUCCESS s) {
    std::cout << SUCCESS_MESSAGES[static_cast<size_t>(s)] << std::endl;
    exit(0);
}

/* Initialized empty repository.*/
void FINAL_INIT() { final(SUCCESS::INITED); }

int main(int argc, char* argv[]) {
    if (argc < 2) return 0;

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
        // handle adding to staging

        return 0;

    } else if (1) {
        ;
    }
}