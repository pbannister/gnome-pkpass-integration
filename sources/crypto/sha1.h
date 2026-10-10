#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace gnome_pkpass::crypto {

// SHA-1 (FIPS 180-4), needed because a pass manifest records one digest per
// file as lowercase hexadecimal SHA-1.  This is a content check, not a security
// primitive: the manifest's own integrity rests on the PKCS#7 signature, which
// signs the manifest bytes and is verified with the CMS digest, not with this.
class sha1_c {
public:
    void update(const std::uint8_t *p_data, std::size_t u_size);
    void update(const std::string &s_text);

    // Finish and return the digest as lowercase hexadecimal.  Calling this twice
    // returns the same text; updating after it has no effect.
    std::string hex_digest();

private:
    void transform(const std::uint8_t *p_block);

    // Feed bytes into the block buffer without counting them in the message
    // length; the padding and the length word are not message bytes.
    void update_raw(const std::uint8_t *p_data, std::size_t u_size);

    std::uint32_t o_state_[5] = {0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U, 0xc3d2e1f0U};
    std::uint64_t u_length_ = 0;
    std::uint8_t o_buffer_[64] = {};
    std::size_t u_buffered_ = 0;
    bool b_final_ = false;
    std::string s_digest_;
};

// The digest of a string, for tests and short inputs.
std::string sha1_hex(const std::string &s_text);

// The digest of a byte buffer.
std::string sha1_hex(const std::uint8_t *p_data, std::size_t u_size);

}  // namespace gnome_pkpass::crypto
