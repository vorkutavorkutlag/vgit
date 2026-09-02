#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace vgit {
class Repository {
   public:
    /* Initializes empty repository at CWD */
    static int init();

    /* Creates new branch by that name */
    static int create_branch(std::string_view branch_name);

    /* Deletes existing branch by that name */
    static int delete_branch(std::string_view branch_name);

    /* Replaces the active branch with other */
    static int switch_to_branch(std::string_view branch_name);

    /* Prints existing branches and highlights active branch */
    static int display_branches();

    /* Adds suggested files to stage, overwrites if necessary*/
    static int add_to_stage(std::span<std::string const>, bool overwrite);

    /* Removes given files from stage, wipes clean if no files specified */
    static int reset_stage(std::span<std::string const> files);

    /* Displays the staged files to standard output */
    static int display_stage();

    /* Seals the stage as a commit, creates new empty stage */
    static int commit_stage(std::string_view message);

    /* Copies proposed commit's files into CWD, overwrites */
    static int rollback_to_commit(std::string_view hash);

    /* Displays history of commits on branch to standard output */
    static int display_history();

    /* "You're lucky I'm on your side," says Doom-shroom. "I could destroy
     * everything you hold dear. It wouldn't be hard." */
    static int nuke();

    static const std::string& __get_active_branch();
    [[nodiscard]] static bool __set_active_branch(std::string_view name);

    static const std::string& __get_head_hash();
    [[nodiscard]] static bool __set_head_hash(std::string_view hash);

    [[nodiscard]] static bool __is_inited();

   private:
    static inline std::string _active_branch;
    static inline std::string _head_hash;

    /* ------------------------------ */

    static int copy_branch(std::string_view src, std::string_view dst);

    static void rec_path(const fs::path& fp, const std::string& buffer);

    static std::string get_random_hash();

    static fs::path isolate_commit_path(const fs::path& fp);

    /* accepted files are only non-directory files. */
    [[nodiscard]] static bool create_commit_symlink(const fs::path& commit_dst,
                                                    const fs::path& file);

    [[nodiscard]] static bool copy_commit_data(const fs::path& commit_path);

    static std::string get_commit_message(std::string_view hash);

    static fs::directory_iterator get_branches();
};
}  // namespace vgit