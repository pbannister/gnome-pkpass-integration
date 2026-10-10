#include "util/file_io.h"

#include <fstream>
#include <iterator>
#include <sys/stat.h>

namespace gnome_pkpass::util {

bool read_file(const std::string &s_path, std::string &s_data, std::string &s_error) {
    std::ifstream o_stream(s_path, std::ios::binary);
    if (!o_stream) {
        s_error = "cannot open " + s_path;
        return false;
    }
    std::string s_contents((std::istreambuf_iterator<char>(o_stream)),
                           std::istreambuf_iterator<char>());
    if (o_stream.bad()) {
        s_error = "cannot read " + s_path;
        return false;
    }
    s_data = std::move(s_contents);
    return true;
}

bool write_file(const std::string &s_path, const std::string &s_data, std::string &s_error) {
    std::ofstream o_stream(s_path, std::ios::binary | std::ios::trunc);
    if (!o_stream) {
        s_error = "cannot create " + s_path;
        return false;
    }
    o_stream.write(s_data.data(), static_cast<std::streamsize>(s_data.size()));
    if (!o_stream) {
        s_error = "cannot write " + s_path;
        return false;
    }
    return true;
}

bool file_exists(const std::string &s_path) {
    struct stat o_status;
    if (0 != ::stat(s_path.c_str(), &o_status)) {
        return false;
    }
    return S_ISREG(o_status.st_mode);
}

std::string base64_encode(const std::string &s_data) {
    static const char *const s_alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string s_encoded;
    s_encoded.reserve(((s_data.size() + 2) / 3) * 4);

    std::size_t u_at = 0;
    while (u_at + 3 <= s_data.size()) {
        const unsigned int u_group = (static_cast<unsigned int>(
                                          static_cast<unsigned char>(s_data[u_at]))
                                      << 16) |
                                     (static_cast<unsigned int>(
                                          static_cast<unsigned char>(s_data[u_at + 1]))
                                      << 8) |
                                     static_cast<unsigned int>(
                                         static_cast<unsigned char>(s_data[u_at + 2]));
        s_encoded.push_back(s_alphabet[(u_group >> 18) & 0x3f]);
        s_encoded.push_back(s_alphabet[(u_group >> 12) & 0x3f]);
        s_encoded.push_back(s_alphabet[(u_group >> 6) & 0x3f]);
        s_encoded.push_back(s_alphabet[u_group & 0x3f]);
        u_at += 3;
    }

    const std::size_t u_left = s_data.size() - u_at;
    if (1 == u_left) {
        const unsigned int u_group =
            static_cast<unsigned int>(static_cast<unsigned char>(s_data[u_at])) << 16;
        s_encoded.push_back(s_alphabet[(u_group >> 18) & 0x3f]);
        s_encoded.push_back(s_alphabet[(u_group >> 12) & 0x3f]);
        s_encoded.push_back('=');
        s_encoded.push_back('=');
    } else if (2 == u_left) {
        const unsigned int u_group =
            (static_cast<unsigned int>(static_cast<unsigned char>(s_data[u_at])) << 16) |
            (static_cast<unsigned int>(static_cast<unsigned char>(s_data[u_at + 1])) << 8);
        s_encoded.push_back(s_alphabet[(u_group >> 18) & 0x3f]);
        s_encoded.push_back(s_alphabet[(u_group >> 12) & 0x3f]);
        s_encoded.push_back(s_alphabet[(u_group >> 6) & 0x3f]);
        s_encoded.push_back('=');
    }

    return s_encoded;
}

}  // namespace gnome_pkpass::util
