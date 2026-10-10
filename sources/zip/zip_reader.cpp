#include "zip/zip_reader.h"

#include "util/file_io.h"

#include <zlib.h>

#include <sstream>

namespace gnome_pkpass::zip {
namespace {

constexpr std::uint32_t SIGNATURE_LOCAL = 0x04034b50U;
constexpr std::uint32_t SIGNATURE_CENTRAL = 0x02014b50U;
constexpr std::uint32_t SIGNATURE_END = 0x06054b50U;
constexpr std::uint16_t METHOD_STORED = 0;
constexpr std::uint16_t METHOD_DEFLATE = 8;
constexpr std::uint32_t SIZE_UNKNOWN = 0xffffffffU;
constexpr std::size_t END_MINIMUM = 22;
// The end record carries a comment of at most 65535 bytes, so the signature
// cannot be further than that from the end.
constexpr std::size_t END_SEARCH = END_MINIMUM + 65535;

std::uint16_t read_u16(const std::string &s_data, std::size_t u_at) {
    return static_cast<std::uint16_t>(
        static_cast<unsigned char>(s_data[u_at]) |
        (static_cast<std::uint16_t>(static_cast<unsigned char>(s_data[u_at + 1])) << 8));
}

std::uint32_t read_u32(const std::string &s_data, std::size_t u_at) {
    return static_cast<std::uint32_t>(static_cast<unsigned char>(s_data[u_at])) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(s_data[u_at + 1])) << 8) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(s_data[u_at + 2])) << 16) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(s_data[u_at + 3])) << 24);
}

bool range_ok(std::size_t u_size, std::size_t u_at, std::size_t u_want) {
    return u_at <= u_size && u_want <= u_size - u_at;
}

bool inflate_raw(const std::string &s_input, std::uint64_t u_size_expected,
                 std::string &s_output, std::string &s_error) {
    s_output.assign(static_cast<std::size_t>(u_size_expected), '\0');

    z_stream o_stream{};
    o_stream.next_in =
        reinterpret_cast<Bytef *>(const_cast<char *>(s_input.data()));
    o_stream.avail_in = static_cast<uInt>(s_input.size());
    o_stream.next_out = reinterpret_cast<Bytef *>(s_output.data());
    o_stream.avail_out = static_cast<uInt>(s_output.size());

    if (Z_OK != inflateInit2(&o_stream, -MAX_WBITS)) {
        s_error = "cannot start the deflate decoder";
        return false;
    }
    const int i_result = inflate(&o_stream, Z_FINISH);
    inflateEnd(&o_stream);

    if (Z_STREAM_END != i_result) {
        std::ostringstream s_stream;
        s_stream << "deflate stream did not end cleanly (zlib " << i_result << ")";
        s_error = s_stream.str();
        return false;
    }
    if (static_cast<std::uint64_t>(o_stream.total_out) != u_size_expected) {
        s_error = "deflate stream produced the wrong number of bytes";
        return false;
    }
    return true;
}

}  // namespace

bool archive_c::load_file(const std::string &s_path, std::string &s_error) {
    std::string s_data;
    if (!util::read_file(s_path, s_data, s_error)) {
        return false;
    }
    return load_bytes(s_data, s_error);
}

bool archive_c::load_bytes(const std::string &s_data, std::string &s_error) {
    s_data_ = s_data;
    o_entries_.clear();

    if (!range_ok(s_data_.size(), 0, 4) || SIGNATURE_LOCAL != read_u32(s_data_, 0)) {
        s_error = "not a ZIP archive: no local file header at the start";
        return false;
    }

    // Find the end-of-central-directory record by scanning back from the end.
    std::size_t u_end = 0;
    bool b_found = false;
    const std::size_t u_earliest =
        (END_SEARCH < s_data_.size()) ? (s_data_.size() - END_SEARCH) : 0;
    for (std::size_t u_scan = s_data_.size() - END_MINIMUM + 1; u_scan-- > u_earliest;) {
        if (SIGNATURE_END == read_u32(s_data_, u_scan)) {
            u_end = u_scan;
            b_found = true;
            break;
        }
    }
    if (!b_found) {
        s_error = "not a ZIP archive: no end-of-central-directory record";
        return false;
    }

    const std::uint16_t u_count = read_u16(s_data_, u_end + 10);
    const std::uint32_t u_size_directory = read_u32(s_data_, u_end + 12);
    const std::uint32_t u_offset_directory = read_u32(s_data_, u_end + 16);

    if (SIZE_UNKNOWN == u_offset_directory || SIZE_UNKNOWN == u_size_directory ||
        static_cast<std::uint16_t>(0xffff) == u_count) {
        s_error = "ZIP64 archives are not supported";
        return false;
    }
    if (!range_ok(s_data_.size(), u_offset_directory, u_size_directory)) {
        s_error = "the central directory runs past the end of the file";
        return false;
    }

    std::size_t u_at = u_offset_directory;
    for (std::uint16_t u_index = 0; u_index < u_count; ++u_index) {
        if (!range_ok(s_data_.size(), u_at, 46) || SIGNATURE_CENTRAL != read_u32(s_data_, u_at)) {
            s_error = "the central directory is malformed";
            return false;
        }
        entry_o o_entry;
        o_entry.u_method = read_u16(s_data_, u_at + 10);
        o_entry.u_crc32 = read_u32(s_data_, u_at + 16);
        o_entry.u_size_compressed = read_u32(s_data_, u_at + 20);
        o_entry.u_size_uncompressed = read_u32(s_data_, u_at + 24);
        const std::uint16_t u_size_name = read_u16(s_data_, u_at + 28);
        const std::uint16_t u_size_extra = read_u16(s_data_, u_at + 30);
        const std::uint16_t u_size_comment = read_u16(s_data_, u_at + 32);
        o_entry.u_offset_local = read_u32(s_data_, u_at + 42);

        if (!range_ok(s_data_.size(), u_at + 46, u_size_name)) {
            s_error = "a member name runs past the end of the file";
            return false;
        }
        o_entry.s_name = s_data_.substr(u_at + 46, u_size_name);

        if (SIZE_UNKNOWN == o_entry.u_size_compressed ||
            SIZE_UNKNOWN == o_entry.u_size_uncompressed ||
            SIZE_UNKNOWN == o_entry.u_offset_local) {
            s_error = "ZIP64 members are not supported";
            return false;
        }

        o_entries_.push_back(o_entry);
        u_at += 46 + u_size_name + u_size_extra + u_size_comment;
    }

    return true;
}

const std::vector<entry_o> &archive_c::entries() const {
    return o_entries_;
}

const entry_o *archive_c::find(const std::string &s_name) const {
    for (const entry_o &o_entry : o_entries_) {
        if (o_entry.s_name == s_name) {
            return &o_entry;
        }
    }
    return nullptr;
}

bool archive_c::contains(const std::string &s_name) const {
    return nullptr != find(s_name);
}

bool archive_c::extract(const std::string &s_name, std::string &s_out,
                        std::string &s_error) const {
    const entry_o *p_entry = find(s_name);
    if (nullptr == p_entry) {
        s_error = "no such member: " + s_name;
        return false;
    }
    return extract(*p_entry, s_out, s_error);
}

bool archive_c::extract(const entry_o &o_entry, std::string &s_out,
                        std::string &s_error) const {
    const std::size_t u_local = static_cast<std::size_t>(o_entry.u_offset_local);
    if (!range_ok(s_data_.size(), u_local, 30) || SIGNATURE_LOCAL != read_u32(s_data_, u_local)) {
        s_error = "the local header for " + o_entry.s_name + " is malformed";
        return false;
    }
    const std::uint16_t u_size_name = read_u16(s_data_, u_local + 26);
    const std::uint16_t u_size_extra = read_u16(s_data_, u_local + 28);
    const std::size_t u_data = u_local + 30 + u_size_name + u_size_extra;
    const std::size_t u_size_compressed = static_cast<std::size_t>(o_entry.u_size_compressed);

    if (!range_ok(s_data_.size(), u_data, u_size_compressed)) {
        s_error = "the data for " + o_entry.s_name + " runs past the end of the file";
        return false;
    }
    const std::string s_compressed = s_data_.substr(u_data, u_size_compressed);

    if (METHOD_STORED == o_entry.u_method) {
        s_out = s_compressed;
    } else if (METHOD_DEFLATE == o_entry.u_method) {
        if (!inflate_raw(s_compressed, o_entry.u_size_uncompressed, s_out, s_error)) {
            s_error = o_entry.s_name + ": " + s_error;
            return false;
        }
    } else {
        std::ostringstream s_stream;
        s_stream << o_entry.s_name << ": unsupported compression method " << o_entry.u_method;
        s_error = s_stream.str();
        return false;
    }

    const std::uint32_t u_crc =
        static_cast<std::uint32_t>(crc32(0, reinterpret_cast<const Bytef *>(s_out.data()),
                                        static_cast<uInt>(s_out.size())));
    if (u_crc != o_entry.u_crc32) {
        s_error = o_entry.s_name + ": CRC does not match the central directory";
        return false;
    }

    return true;
}

}  // namespace gnome_pkpass::zip
