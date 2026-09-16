#include "delta.hpp"

#include <fstream>

void vgit::Delta::handle_run(std::streampos offset, uint8_t byte,
                             bool running) {
    if (running && !_runs.empty())
        _runs.back().diff.push_back(byte);
    else
        _runs.push_back({offset, {byte}});
}

void vgit::Delta::copy_file_bytes(const fs::path& srcpath,
                                  std::ofstream& dststream,
                                  std::size_t copyLeft, std::streamoff offset) {
    std::ifstream src(srcpath, std::ios::binary);
    src.seekg(offset, std::ios::beg);

    const size_t BUFFER_SIZE{4096uz};
    char buffer[BUFFER_SIZE];

    while (copyLeft > 0) {
        std::size_t toRead = std::min(BUFFER_SIZE, copyLeft);

        src.read(buffer, toRead);
        std::streamsize bytesRead = src.gcount();

        // EOF
        if (!bytesRead) break;

        dststream.write(buffer, bytesRead);
        copyLeft -= bytesRead;
    }
}

void vgit::Delta::copy_vec_bytes(std::ofstream& dststream,
                                 const std::vector<uint8_t>& data) {
    dststream.write(reinterpret_cast<const char*>(data.data()), data.size());
}

vgit::Delta::Delta(const fs::path& __old, const fs::path& __new)
    : _fsize(fs::file_size(__new)), _original(__old) {
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

void vgit::Delta::apply(const fs::path& __destination) {
    {
        // touch/create
        std::ofstream out(__destination);
    }
    fs::resize_file(__destination, _fsize);
    {
        // trunc
        std::ofstream out(__destination, std::ios::trunc);
    }

    std::ofstream dststream(__destination, std::ios::app | std::ios::binary);
    std::streamoff currentOffset{0LL};
    for (const auto& run : _runs) {
        copy_file_bytes(_original, dststream, run.offset - currentOffset,
                        currentOffset);
        copy_vec_bytes(dststream, run.diff);
        currentOffset = run.offset + run.diff.size();
    }

    const intmax_t leftOver = _fsize - currentOffset;
    if (leftOver > 0)
        copy_file_bytes(_original, dststream, leftOver, currentOffset);
}

void vgit::Delta::serialize(const fs::path& __destination) {
    std::ofstream out(__destination, std::ios::binary | std::ios::trunc);

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
        std::string line1, line2;
        std::getline(in, line1);
        std::getline(in, line2);
        _fsize = strtoull(line1.c_str(), nullptr, 10);
        _original = line2;
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