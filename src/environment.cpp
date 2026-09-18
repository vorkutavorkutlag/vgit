#include <fstream>
#include <iostream>
#include <print>
#include <vgit/delta.hpp>
#include <vgit/environment.hpp>
#include <vgit/repository.hpp>

/* ------------ private implementation ------------ */

nlohmann::json vgit::Environment::get_global_info() {
    if (!fs::exists(vgit::Consts::GLOBAL_INFO_PATH)) {
        std::ofstream out{vgit::Consts::GLOBAL_INFO_PATH};
        if (!out) return nlohmann::json{};
        out << vgit::Consts::empty_json_dict;
        if (!out) return nlohmann::json{};
    }

    nlohmann::json loaded;
    {
        std::ifstream in{vgit::Consts::GLOBAL_INFO_PATH};
        if (!in) return nlohmann::json{};
        in >> loaded;
        if (!in) return nlohmann::json{};
    }
    return loaded;
}

bool vgit::Environment::set_global_info(const nlohmann::json& info) {
    std::ofstream out{vgit::Consts::GLOBAL_INFO_PATH, std::ios::trunc};
    if (!out) return false;
    out << info;
    if (!out) return false;

    return true;
}

bool vgit::Environment::ensure_json_list(const fs::path& fp) {
    if (fs::exists(fp)) return true;

    std::ofstream out{fp};
    if (!out) return false;
    out << vgit::Consts::empty_json_list;
    if (!out) return false;

    return true;
}

bool vgit::Environment::ensure_json_dict(const fs::path& fp) {
    if (fs::exists(fp)) return true;
    std::ofstream out{fp};
    if (!out) return false;
    out << vgit::Consts::empty_json_dict;
    if (!out) return false;
    return true;
}

bool vgit::Environment::remove_commit_unsafe(std::string_view commit_hash) {
    const auto commit_path{vgit::Consts::BRANCHES_PATH / vgit::Repository::__get_active_branch() / commit_hash};
    std::error_code ec;
    fs::remove_all(commit_path, ec);
    return !ec.value();
}

nlohmann::json vgit::Environment::get_branch_info() {
    const auto b_info_path{vgit::Consts::BRANCHES_PATH / vgit::Repository::__get_active_branch() /
                           vgit::Consts::p_branch_info_path};

    if (!ensure_json_dict(b_info_path)) return nlohmann::json{};

    nlohmann::json loaded;
    {
        std::ifstream in{b_info_path};
        if (!in) return nlohmann::json{};
        in >> loaded;
        if (!in) return nlohmann::json{};
    }

    return loaded;
}

bool vgit::Environment::set_branch_info(const nlohmann::json& info) {
    const auto b_info_path{vgit::Consts::BRANCHES_PATH / vgit::Repository::__get_active_branch() /
                           vgit::Consts::p_branch_info_path};
    std::ofstream out{b_info_path, std::ios::trunc};
    if (!out) return false;
    out << info;
    if (!out) return false;
    return true;
}

/* ------------ public implementation ------------ */

std::string vgit::Environment::get_active_branch() {
    auto json = get_global_info();
    auto it = json.find(vgit::Consts::s_active_branch);
    if (it == json.end()) return std::string{};
    return fs::path(*it).filename();
}

bool vgit::Environment::set_active_branch(std::string_view branch) {
    if (!fs::is_directory(vgit::Consts::BRANCHES_PATH / branch)) return false;
    auto json = get_global_info();
    json[vgit::Consts::s_active_branch] = branch;
    return set_global_info(json);
}

std::string vgit::Environment::get_head_hash() {
    const auto json = get_branch_info();
    const auto it = json.find(vgit::Consts::s_head_hash);
    if (it == json.end()) return std::string{};
    return *it;
}

bool vgit::Environment::set_head_hash(std::string_view hash) {
    auto json = get_branch_info();
    json[vgit::Consts::s_head_hash] = hash;
    return set_branch_info(json);
}

bool vgit::Environment::valid_file_scope(const fs::path& fp) {
    auto repo = fs::canonical(vgit::Consts::CWD);
    auto mismatch = std::mismatch(repo.begin(), repo.end(), fp.begin(), fp.end());

    // is within CWD
    if (mismatch.first != repo.end() || fp == repo) return false;

    // is outside .vgit directory
    auto iter = fp;
    while (iter != (iter = iter.parent_path())) {
        if (iter == vgit::Consts::VGIT_ROOT) return false;
        if (iter == vgit::Consts::CWD) return true;
    }
    // shouldn't ever reach this
    return true;
}

bool vgit::Environment::valid_branch_scope(const fs::path& fp) {
    auto repo = fs::canonical(vgit::Consts::CWD);
    auto mismatch =
        std::mismatch(vgit::Consts::BRANCHES_PATH.begin(), vgit::Consts::BRANCHES_PATH.end(), fp.begin(), fp.end());

    // is within branches path
    if (mismatch.first != vgit::Consts::BRANCHES_PATH.end() || fp == repo) return false;

    // is outside any given branch
    return fp.parent_path() == vgit::Consts::BRANCHES_PATH;
}

bool vgit::Environment::update_history(std::string_view new_head_hash) {
    const auto history_path{vgit::Consts::BRANCHES_PATH / vgit::Repository::__get_active_branch() /
                            vgit::Consts::p_commit_history_path};

    auto commit_it = std::find(_commit_history->begin(), _commit_history->end(), vgit::Repository::__get_head_hash());

    if (commit_it != _commit_history->end()) ++commit_it;

    bool errored{false};
    while (commit_it != _commit_history->end()) {
        if (!remove_commit_unsafe(commit_it->get<std::string>())) errored = true;
        commit_it = _commit_history->erase(commit_it);
    }

    _commit_history->push_back(new_head_hash);

    {
        std::ofstream out{history_path, std::ios::trunc};
        if (!out) return false;
        out << *_commit_history;
        if (!out) return false;
    }

    return !errored;
}

const nlohmann::json& vgit::Environment::get_commit_history() {
    const auto history_path{vgit::Consts::BRANCHES_PATH / vgit::Repository::__get_active_branch() /
                            vgit::Consts::p_commit_history_path};

    if (_commit_history) return *_commit_history;
    _commit_history.emplace();

    if (!ensure_json_list(history_path)) throw(std::runtime_error("Problem creating empty JSON"));
    {
        std::ifstream in{history_path};
        in >> *_commit_history;
    }

    return *_commit_history;
}

std::optional<std::string> vgit::Environment::get_basefile_hash(const fs::path& target) {
    for (const auto& commit : *_commit_history) {
        const auto commit_path = vgit::Consts::BRANCHES_PATH / vgit::Repository::__get_active_branch() / commit;
        for (const auto& file : fs::recursive_directory_iterator(commit_path)) {
            if (file.is_directory()) continue;
            if (fs::relative(file, commit_path) == target) return commit;
        }
    }
    return std::nullopt;
}

bool vgit::Environment::create_most_recent_version(const std::string& basef_hash, const fs::path& file,
                                                   const fs::path& destination) {
    auto hash_it = std::find(_commit_history->begin(), _commit_history->end(), basef_hash);

    if (!fs::create_directories(destination.parent_path()) && !fs::is_directory(destination.parent_path()))
        return false;

    const auto base_commit = vgit::Consts::BRANCHES_PATH / vgit::Repository::__get_active_branch() / basef_hash;

    if (!fs::copy_file(base_commit / file, destination)) return false;

    while (++hash_it != _commit_history->end()) {
        const auto cur_commit = vgit::Consts::BRANCHES_PATH / vgit::Repository::__get_active_branch() / *hash_it;
        auto target_f = file;
        target_f += vgit::Consts::delta_extension;
        const auto target = cur_commit / (target_f);

        if (!fs::exists(target)) continue;

        // apply delta unto destination
        vgit::Delta delta(target);  // deserialize
        delta.apply(destination);
    }

    // by the end, destination has had all the deltas applied on it
    return true;
}

// should add false returns too
bool vgit::Environment::rebuild_commit_at_cwd(const std::string& commit_hash) {
    for (const auto& hash : *_commit_history) {
        const auto cur_commit = vgit::Consts::BRANCHES_PATH / vgit::Repository::__get_active_branch() / hash;

        for (const auto& commit_file : fs::recursive_directory_iterator(cur_commit)) {
            if (commit_file.is_directory()) continue;

            const auto relpath = fs::relative(commit_file, cur_commit);
            auto path_in_cwd = vgit::Consts::CWD / relpath;
            if (commit_file.path().string().ends_with(vgit::Consts::delta_extension)) {
                // delta file
                vgit::Delta delta(commit_file);
                delta.apply(path_in_cwd.replace_extension());  // removes extension;
                // this should work on multiple extensions. e.g. something.tar.gz.vdelta
            } else {
                // original file
                fs::copy_file(commit_file, path_in_cwd, fs::copy_options::overwrite_existing);
            }
        }

        if (hash == commit_hash) break;
    }

    return true;
}