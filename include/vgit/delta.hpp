#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace vgit {
class Delta {
   private:
    struct run_t {
        std::streamoff offset;
        std::vector<uint8_t> diff;
    };

    void handle_run(std::streampos offset, uint8_t byte, bool running);

    void copy_file_bytes(const fs::path& srcpath, std::ofstream& dststream,
                         std::size_t copyLeft, std::streamoff offset);

    void copy_vec_bytes(std::ofstream& dststream,
                        const std::vector<uint8_t>& data);

   public:
    uintmax_t _fsize{};
    fs::path _original{};
    std::vector<run_t> _runs{};

    Delta(const fs::path& __old, const fs::path& __new);

    void apply(const fs::path& __destination);

    void serialize(const fs::path& __destination);

    Delta(const fs::path& __serialized);
};
}  // namespace vgit