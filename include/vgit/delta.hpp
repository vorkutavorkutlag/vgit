#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace vgit {
class Delta {
   public:
    Delta(const fs::path& __old, const fs::path& __new);

    void apply(const fs::path& __destination);

    void serialize(const fs::path& __destination);

    Delta(const fs::path& __serialized);

   private:
    struct run_t {
        std::streamoff offset;
        std::vector<uint8_t> diff;
    };

    uintmax_t _fsize{};
    std::vector<run_t> _runs{};

    void handle_run(std::streampos offset, uint8_t byte, bool running);
};
}  // namespace vgit