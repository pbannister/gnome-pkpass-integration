#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gnome_pkpass::zip {

// One member of the archive, as the central directory describes it.  A pkpass
// is a ZIP, and Apple's own sample mixes stored and deflated members, so the
// per-entry method is carried rather than assumed.
struct entry_o {
    std::string s_name;
    std::uint16_t u_method = 0;
    std::uint32_t u_crc32 = 0;
    std::uint64_t u_size_compressed = 0;
    std::uint64_t u_size_uncompressed = 0;
    std::uint64_t u_offset_local = 0;
};

// A read-only ZIP archive held in memory.  It supports the two compression
// methods a pass uses (0 stored, 8 deflate) and refuses ZIP64, which no pass
// needs; a refusal is reported rather than guessed at.
class archive_c {
public:
    bool load_file(const std::string &s_path, std::string &s_error);
    bool load_bytes(const std::string &s_data, std::string &s_error);

    const std::vector<entry_o> &entries() const;
    const entry_o *find(const std::string &s_name) const;
    bool contains(const std::string &s_name) const;

    // Extract one member, checking its CRC.  The result is the uncompressed
    // bytes.
    bool extract(const entry_o &o_entry, std::string &s_out, std::string &s_error) const;
    bool extract(const std::string &s_name, std::string &s_out, std::string &s_error) const;

private:
    std::string s_data_;
    std::vector<entry_o> o_entries_;
};

}  // namespace gnome_pkpass::zip
