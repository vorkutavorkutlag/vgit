#include <CLI11.hpp>
#include <print>
#include <vgit/cli.hpp>
#include <vgit/repository.hpp>

namespace vgit::cli {
int run(int argc, char* argv[]) {
    CLI::App app{
        "vgit - the barebones local git\nQuick guide:\nvgit init\nvgit branch "
        "main\nvgit add *\nvgit commit -m 'first commit'"};

    auto* init = app.add_subcommand("init", "Initialize repository");

    std::string branch_name;
    bool delete_branch{false};
    auto* branch = app.add_subcommand("branch", "Manage branches");
    branch->add_option("name", branch_name, "Branch name");
    branch->add_flag("-d,-D,--delete", delete_branch, "Delete a branch");

    std::string switch_name;
    auto* swtch = app.add_subcommand("switch", "Switch branch");
    swtch->add_option("name", switch_name, "Branch name");

    bool add_overwrite{false};
    std::vector<std::string> add_files;

    auto* add = app.add_subcommand("add", "Add files to stage");
    add->add_option("files", add_files, "Files to add");
    add->add_flag("-f,-F,--force", add_overwrite, "Overwrite files on stage");

    std::vector<std::string> reset_files;
    auto* reset = app.add_subcommand("reset", "Reset files from stage");
    reset->add_option("files", reset_files, "Files to remove");

    auto* diff = app.add_subcommand("diff", "Show files on stage");

    std::string commit_message;
    auto* commit =
        app.add_subcommand("commit", "Commit current changes in stage");
    commit->add_option("-m,--message", commit_message, "Commit message");

    auto* history =
        app.add_subcommand("history", "View commit history in branch");

    std::string rollback_hash;
    auto* rollback =
        app.add_subcommand("rollback", "Roll bacck to previous commit");
    rollback->add_option("hash", rollback_hash,
                         "First few letters of desired commit hash");

    auto* nuke =
        app.add_subcommand("nuke", "Delete repository in working directory");

    CLI11_PARSE(app, argc, argv);

    if (*init) return vgit::Repository::init();

    if (!vgit::Repository::__is_inited()) {
        std::println(stderr, "Cannot proceed without initialized repository.");
        return EXIT_FAILURE;
    }

    if (*nuke) return vgit::Repository::nuke();

    if (*branch) {
        if (branch_name.empty()) return vgit::Repository::display_branches();
        if (delete_branch) return vgit::Repository::delete_branch(branch_name);
        return vgit::Repository::create_branch(branch_name);
    }

    if (vgit::Repository::__get_active_branch().empty()) {
        std::println(stderr, "Cannot proceed without branches.");
        return EXIT_FAILURE;
    }

    if (*swtch) return vgit::Repository::switch_to_branch(switch_name);

    if (*add) return vgit::Repository::add_to_stage(add_files, add_overwrite);

    if (*reset) return vgit::Repository::reset_stage(reset_files);

    if (*diff) return vgit::Repository::display_stage();

    if (*commit) return vgit::Repository::commit_stage(commit_message);

    if (*history) return vgit::Repository::display_history();

    if (*rollback) return vgit::Repository::rollback_to_commit(rollback_hash);

    std::println("{}", app.help());
    return EXIT_SUCCESS;
}
}  // namespace vgit::cli