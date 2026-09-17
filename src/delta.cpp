#include "vgit/delta.hpp"

#include <fstream>

void vgit::Delta::handle_run(std::streampos offset, uint8_t byte,
                             bool running) {
    if (running && !_runs.empty())
        _runs.back().diff.push_back(byte);
    else
        _runs.push_back({offset, {byte}});
}

vgit::Delta::Delta(const fs::path& __old, const fs::path& __new)
    : _fsize(fs::file_size(__new)) {
    std::ifstream input_old(__old, std::ios::binary);
    std::ifstream input_new(__new, std::ios::binary);

    // are we currently in a run, or do we need to create a new one?
    bool running{false};
    std::streamoff offset{0};
    char old_byte, new_byte;

    while (input_new.get(new_byte)) {
        if (!input_old.get(old_byte) || old_byte != new_byte) {
            handle_run(offset, new_byte, running);
            running = true;
        } else
            running = false;
        ++offset;
    }
}

// assumes destination exists and patches it directly
void vgit::Delta::apply(const fs::path& __destination) {
    fs::resize_file(__destination, _fsize);
    std::fstream dststream(__destination,
                           std::ios::binary | std::ios::in | std::ios::out);
    for (const auto& run : _runs) {
        dststream.seekp(run.offset);
        dststream.write(reinterpret_cast<const char*>(run.diff.data()),
                        run.diff.size());
    }
}

void vgit::Delta::serialize(const fs::path& __destination) {
    std::ofstream out(__destination, std::ios::binary | std::ios::trunc);

    std::println(out, "{}", _fsize);

    size_t runCount = _runs.size();
    out.write(reinterpret_cast<const char*>(&runCount), sizeof(runCount));

    for (const auto& run : _runs) {
        out.write(reinterpret_cast<const char*>(&run.offset),
                  sizeof(run.offset));

        size_t diffSize = run.diff.size();
        out.write(reinterpret_cast<const char*>(&diffSize), sizeof(diffSize));
        out.write(reinterpret_cast<const char*>(run.diff.data()), diffSize);
    }
}

vgit::Delta::Delta(const fs::path& __serialized) {
    std::ifstream in(__serialized, std::ios::binary);

    {
        std::string line;
        std::getline(in, line);
        _fsize = strtoull(line.c_str(), nullptr, 10);
    }

    size_t runCount;
    in.read(reinterpret_cast<char*>(&runCount), sizeof(runCount));

    for (size_t i = 0; i < runCount; ++i) {
        std::streamoff offset;
        in.read(reinterpret_cast<char*>(&offset), sizeof(offset));

        size_t diffSize;
        in.read(reinterpret_cast<char*>(&diffSize), sizeof(diffSize));

        std::vector<uint8_t> diff(diffSize);
        in.read(reinterpret_cast<char*>(diff.data()), diffSize);

        _runs.push_back({offset, diff});
    }
}