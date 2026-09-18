#include <sys/stat.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <print>
#include <random>
#include <ranges>
#include <set>
#include <vgit/constants.hpp>
#include <vgit/delta.hpp>
#include <vgit/environment.hpp>
#include <vgit/repository.hpp>

/* ------------ private implementation ------------ */

int vgit::Repository::copy_branch(std::string_view src, std::string_view dst) {
    const auto src_history{vgit::Consts::BRANCHES_PATH / src};
    const auto dst_history{vgit::Consts::BRANCHES_PATH / dst};

    assert(fs::is_directory(src_history));

    std::error_code ec;
    fs::copy(src_history, dst_history, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    return ec.value();
}

void vgit::Repository::rec_path(const fs::path& p, const std::string& buffer) {
    fs::directory_iterator dir_it{p};
    for (fs::path file : dir_it) {
        std::println("{}{}", buffer, file.replace_extension().filename().string());

        if (fs::is_directory(file)) {
            std::println("{}/", file.filename().string());
            rec_path(file, buffer + "   |");
        }
    }
}

std::string vgit::Repository::get_random_hash() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 15);

    std::string h(vgit::Consts::commit_hash_length, '\0');

    for (auto& c : h) {
        c = vgit::Consts::hex_digits[dist(gen)];
    }

    return h;
}

/*  pretty spaghetti, im not proud of this, must be a better way.
    makes commit file paths relative to their respective branch */
fs::path vgit::Repository::isolate_commit_path(const fs::path& fp) {
    fs::path f_iter = fp;
    fs::path prev_f_iter{};
    for (;; f_iter = f_iter.parent_path()) {
        if (f_iter.filename().compare(__get_active_branch()))
            prev_f_iter = f_iter;
        else
            return fs::relative(fp, prev_f_iter);
    }
}

bool vgit::Repository::create_commit_symlink(const fs::path& commit_dst, const fs::path& file) {
    std::error_code ec;
    const auto canon{fs::canonical(file)};
    const auto isolated{commit_dst / isolate_commit_path(canon)};
    fs::create_directories(isolated.parent_path());
    fs::create_symlink(canon, isolated, ec);
    return ec.value();
}

std::string vgit::Repository::get_commit_message(std::string_view hash) {
    const auto commit_path{vgit::Consts::BRANCHES_PATH / __get_active_branch() / hash};
    const auto message_path{commit_path / vgit::Consts::p_commit_message_path};
    if (!fs::is_directory(commit_path)) return std::string{};
    if (!fs::is_regular_file(message_path)) return std::string{};

    std::ifstream t{message_path};
    if (!t) return std::string{};
    std::string message{(std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>()};

    return message;
}

fs::directory_iterator vgit::Repository::get_branches() { return fs::directory_iterator(vgit::Consts::BRANCHES_PATH); }

/* ------------ public implementation ------------ */

bool vgit::Repository::__is_inited() { return fs::is_directory(vgit::Consts::VGIT_ROOT); }

const std::string& vgit::Repository::__get_active_branch() {
    if (_active_branch.empty()) _active_branch = vgit::Environment::get_active_branch();
    return _active_branch;
}

bool vgit::Repository::__set_active_branch(std::string_view name) {
    if (!vgit::Environment::set_active_branch(name)) return false;
    _active_branch = name;
    return true;
}

const std::string& vgit::Repository::__get_head_hash() {
    if (_head_hash.empty()) _head_hash = vgit::Environment::get_head_hash();
    return _head_hash;
}

bool vgit::Repository::__set_head_hash(std::string_view hash) {
    if (!vgit::Environment::set_head_hash(hash)) return false;
    _head_hash = hash;
    return true;
}

int vgit::Repository::init() {
    if (__is_inited()) {
        std::println(stderr, "Repository is already initialized.");
        return EXIT_FAILURE;
    }

    if (mkdir(vgit::Consts::VGIT_ROOT.c_str(), vgit::Consts::VGIT_PERMS) ||
        mkdir(vgit::Consts::BRANCHES_PATH.c_str(), vgit::Consts::VGIT_PERMS)) {
        std::println(stderr, "Lacked permissions to create repository.");
        return EXIT_FAILURE;
    }

    std::println("Sucessfully initialized empty repository.");
    return EXIT_SUCCESS;
};

int vgit::Repository::nuke() {
    std::println(
        "Are you sure you want to delete the current "
        "repository?\n[y/N]");
    std::string input;
    std::getline(std::cin, input);

    if (input != "y" && input != "Y") {
        std::println("Nuke aborted.");
        return EXIT_SUCCESS;
    }

    std::error_code ec;
    fs::remove_all(vgit::Consts::VGIT_ROOT, ec);

    if (ec) {
        std::println(stderr, "Failed to delete entire repository.");
        return EXIT_FAILURE;
    }

    std::println(vgit::Consts::BOOM);
    return EXIT_SUCCESS;
}

int vgit::Repository::create_branch(std::string_view name) {
    fs::path proposed_branch_path{vgit::Consts::BRANCHES_PATH / name};

    if (!vgit::Environment::valid_branch_scope(fs::weakly_canonical(proposed_branch_path))) {
        std::println(stderr, "Branch possesses invalid naming.");
        return EXIT_FAILURE;
    }

    if (fs::is_directory(proposed_branch_path)) {
        std::println(stderr, "Branch by the same name already exists.");
        return EXIT_FAILURE;
    }

    if (mkdir(proposed_branch_path.c_str(), vgit::Consts::VGIT_PERMS)) {
        std::println(stderr, "Lacked permissions to create branch.");
        return EXIT_FAILURE;
    }

    // should still have perms here
    mkdir((proposed_branch_path / vgit::Consts::p_stage_path).c_str(), vgit::Consts::VGIT_PERMS);

    std::println("Created new branch: {}", name);

    const auto active_branch{__get_active_branch()};
    if (!active_branch.empty()) {
        if (copy_branch(active_branch, name)) {
            std::println(stderr, "Error occured while copying branch history.");
        }
    }

    if (!__set_active_branch(name)) {
        std::println(stderr, "Error occured while switching to new branch.");
        return EXIT_FAILURE;
    }

    std::println("Switched to new branch: {}", name);
    return EXIT_SUCCESS;
}

int vgit::Repository::delete_branch(std::string_view name) {
    fs::path proposed_branch_path{vgit::Consts::BRANCHES_PATH / name};

    if (!vgit::Environment::valid_branch_scope(fs::weakly_canonical(proposed_branch_path))) {
        std::println(stderr, "Branch possesses invalid naming.");
        return EXIT_FAILURE;
    }

    if (__get_active_branch() == name) {
        std::println(stderr, "Cannot suicide. Switch branch before deleting.");
        return EXIT_FAILURE;
    }

    std::error_code ec;
    fs::remove_all(proposed_branch_path, ec);

    if (ec) {
        std::println(stderr, "Failed to remove branch from repository.");
        return EXIT_FAILURE;
    }

    std::println("Branch successfully deleted.");
    return EXIT_SUCCESS;
}

int vgit::Repository::switch_to_branch(std::string_view name) {
    fs::path proposed_branch_path{vgit::Consts::BRANCHES_PATH / name};
    if (!vgit::Environment::valid_branch_scope(fs::weakly_canonical(proposed_branch_path))) {
        std::println(stderr, "Branch possesses invalid naming.");
        return EXIT_FAILURE;
    }

    if (vgit::Environment::set_active_branch(name)) {
        std::println("Switched to branch: {}", name);
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

int vgit::Repository::add_to_stage(std::span<std::string const> files, bool overwrite) {
    if (files.empty()) {
        std::println("No files specified, no files added to stage.");
        return EXIT_SUCCESS;
    }

    for (const auto& file : files) {
        std::error_code ec;
        fs::path fpath{fs::canonical(file, ec)};

        std::print("{}: ", file);

        if (ec) {
            std::println("File does not exist.");
            continue;
        }

        if (!vgit::Environment::valid_file_scope(fpath)) {
            std::println("File is out of repository scope.");
            continue;
        }

        const fs::path relative = fs::relative(fpath, vgit::Consts::CWD);
        const fs::path destination =
            vgit::Consts::BRANCHES_PATH / __get_active_branch() / vgit::Consts::p_stage_path / relative;

        if (fs::exists(destination) && !overwrite) {
            std::println("File already exists on stage. Run with -f to overwrite.");
            continue;
        }

        fs::create_directories(destination.parent_path(), ec);

        if (ec || !fs::is_directory(destination.parent_path())) {
            std::println("Could not create directories for file: {}", destination.parent_path().string());
            continue;
        }

        fs::copy(fpath, destination, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);

        if (ec) {
            std::println("Could not add file to stage.");
            continue;
        }

        std::println("File successfully added to stage.");
    }

    return EXIT_SUCCESS;
}

int vgit::Repository::display_branches() {
    const auto active_branch{__get_active_branch()};
    if (active_branch.empty()) return EXIT_SUCCESS;

    for (const auto& branch : get_branches()) {
        const std::string branch_name = branch.path().filename().string();
        if (branch_name == active_branch) std::print(" + ");
        std::println("{}", branch_name);
    }

    return EXIT_SUCCESS;
}

int vgit::Repository::reset_stage(std::span<std::string const> files) {
    if (files.empty()) {
        for (const auto& file :
             fs::directory_iterator{vgit::Consts::BRANCHES_PATH / __get_active_branch() / vgit::Consts::p_stage_path}) {
            fs::remove_all(file);
        }

        std::println("Successfully reset the stage.");
        return EXIT_SUCCESS;
    }

    for (const auto& file : files) {
        std::error_code ec;
        const auto can = fs::canonical(file, ec);

        std::print("{}: ", file);

        if (ec) {
            std::println("File does not exist.");
            continue;
        }

        if (!vgit::Environment::valid_file_scope(can)) {
            std::println("File is out of repository scope. What are you doing here? ;]");
            continue;
        }

        const auto rel = fs::relative(can, vgit::Consts::CWD);

        const auto proposed_file =
            vgit::Consts::BRANCHES_PATH / __get_active_branch() / vgit::Consts::p_stage_path / rel;

        fs::remove_all(proposed_file, ec);

        if (ec) {
            std::println("Couldn't remove file from stage.");
            continue;
        }
        std::println("File successfully removed from stage.");
    }

    return EXIT_SUCCESS;
}

int vgit::Repository::display_stage() {
    const auto sp{vgit::Consts::BRANCHES_PATH / __get_active_branch() / vgit::Consts::p_stage_path};
    if (fs::is_empty(sp)) {
        std::println("Stage is empty.");
    } else {
        rec_path(sp, "|");
    }

    return EXIT_SUCCESS;
}

int vgit::Repository::commit_stage(std::string_view message) {
    auto commit_hash = get_random_hash();

    const auto stage_path{vgit::Consts::BRANCHES_PATH / __get_active_branch() / vgit::Consts::p_stage_path};
    const auto commit_path{vgit::Consts::BRANCHES_PATH / __get_active_branch() / commit_hash};
    const auto& commit_history = vgit::Environment::get_commit_history();

    if (fs::is_empty(stage_path)) {
        std::println("No changes staged, nothing committed.");
        return EXIT_SUCCESS;
    }

    // optional message for commit
    if (!message.empty()) {
        const auto message_path{stage_path / vgit::Consts::p_commit_message_path};
        std::ofstream ofs(message_path);
        ofs << message << std::endl;
        ofs.close();
    }

    fs::rename(stage_path, commit_path);

    const auto head_hash{__get_head_hash()};
    const auto prev_commit_path{vgit::Consts::BRANCHES_PATH / __get_active_branch() / head_hash};

    /*
        we have files on stage, which we renamed to the commit hash
        find the delta between these and their most recent file versions
        get most recent file version by accumulating deltas through commits
    */

    for (const auto& file : fs::recursive_directory_iterator(commit_path)) {
        // assuming regular files for sake of simplicity
        if (file.is_directory()) continue;

        const auto relfile = fs::relative(file, commit_path);

        // what if there is no basefile?
        const auto basefile_hash = vgit::Environment::get_basefile_hash(relfile);
        if (!basefile_hash) continue;  // this commit is now the basefile

        // get most recent file version
        // who's responsible? probably Environment
        fs::path version_tmpfile = file;
        fs::path final_destination = commit_path / relfile;  // i should watch that
        version_tmpfile += vgit::Consts::tmp_extension;
        final_destination += vgit::Consts::delta_extension;
        if (!vgit::Environment::create_most_recent_version(*basefile_hash, relfile, version_tmpfile)) {
            std::println(stderr, "Could not calculate deltas. Corruption suspected.");
            return EXIT_FAILURE;
        }

        vgit::Delta delta(version_tmpfile, file);
        delta.serialize(final_destination);
        fs::remove(version_tmpfile);
        fs::remove(file);  // now replaced by delta representation
    }

    if (!vgit::Environment::update_history(commit_hash)) {
        std::println(stderr, "Encountered error while updating history.");
        return EXIT_FAILURE;
    }

    // re-create active stage environment
    if (mkdir(stage_path.c_str(), vgit::Consts::VGIT_PERMS)) {
        std::println(stderr, "Failed to recreate stage after committing.");
        return EXIT_FAILURE;
    }

    if (!__set_head_hash(commit_hash)) {
        std::println(stderr, "Failed to set new head for commits.");
        return EXIT_FAILURE;
    }

    std::println("Successfully committed the changes.");
    return EXIT_SUCCESS;
}

int vgit::Repository::rollback_to_commit(std::string_view hash) {
    const auto& commit_history = vgit::Environment::get_commit_history();

    if (commit_history.empty()) {
        std::println(stderr, "No commits to roll back to.");
        return EXIT_FAILURE;
    }

    if (hash.length() >= vgit::Consts::commit_hash_length) {
        std::println(stderr, "Couldn't roll back: abnormal hash length.");
        return EXIT_FAILURE;
    }

    if (hash.empty()) {
        const auto& head = __get_head_hash();
        if (!vgit::Environment::rebuild_commit_at_cwd(head)) {
            std::println("Error occurred while rebuilding commits");
            return EXIT_FAILURE;
        }

        std::println("Rolled back to commit: {}", head);
        return EXIT_SUCCESS;
    }

    std::vector<fs::path> matches{};
    const fs::directory_iterator dir_it{vgit::Consts::BRANCHES_PATH / __get_active_branch()};
    for (const auto& file : dir_it) {
        if (!file.is_directory()) continue;
        if (file.path().filename() == vgit::Consts::p_stage_path) continue;

        auto fstring{file.path().filename().string()};
        if (fstring.starts_with(hash)) matches.push_back(file.path());
    }

    if (matches.empty()) {
        std::println(stderr, "Couldn't roll back: no such commit hash.");
        return EXIT_FAILURE;
    }

    if (matches.size() > 1) {
        std::println(stderr, "Couldn't roll back: ambiguous commit hash.");
        return EXIT_FAILURE;
    }

    const auto& match = matches.front().filename().string();

    if (!vgit::Environment::rebuild_commit_at_cwd(match)) {
        std::println("Error occurred while rebuilding commits");
        return EXIT_FAILURE;
    }

    if (!__set_head_hash(match)) {
        std::println("Error occurred while setting new head hash");
        return EXIT_FAILURE;
    }

    std::println("Rolled back to commit: {}", match);
    return EXIT_SUCCESS;
}

int vgit::Repository::display_history() {
    const auto& history = vgit::Environment::get_commit_history();
    const auto head_hash = __get_head_hash();

    if (history.empty()) {
        std::println("No history on current branch.");
        return EXIT_SUCCESS;
    }

    std::println("History of branch: {}", __get_active_branch());

    for (const auto& commit : history) {
        // commit hash
        std::print("commit - {}", commit.get<std::string_view>());
        if (commit == head_hash) std::print(" <--- HEAD IS HERE");
        std::println();

        // commit message
        const auto message = get_commit_message(commit.get<std::string_view>());
        if (!message.empty()) std::println("message: '{}'", message);

        // files
        std::println("Files present:");
        rec_path(vgit::Consts::BRANCHES_PATH / __get_active_branch() / commit, "|");

        std::println("\n");
    }

    return EXIT_SUCCESS;
}