//
// sha1_test.cpp: SHA-1 against the published vectors, and streaming against
// one-shot hashing.
//
#include "crypto/sha1.h"

#include "test_harness.h"

int main() {
    using gnome_pkpass::crypto::sha1_c;
    using gnome_pkpass::crypto::sha1_hex;

    // FIPS 180-4 / RFC 3174 vectors.
    EXPECT_EQ(std::string("da39a3ee5e6b4b0d3255bfef95601890afd80709"), sha1_hex(""));
    EXPECT_EQ(std::string("a9993e364706816aba3e25717850c26c9cd0d89d"), sha1_hex("abc"));
    EXPECT_EQ(std::string("84983e441c3bd26ebaae4aa1f95129e5e54670f1"),
              sha1_hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"));
    EXPECT_EQ(std::string("2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"),
              sha1_hex("The quick brown fox jumps over the lazy dog"));

    // The compression path only runs once the message passes 64 bytes, so a
    // multi-block input must hash the same way as a one-shot call.
    const std::string s_long(1000, 'a');
    const std::string s_expected = sha1_hex(s_long);

    sha1_c o_hash;
    for (std::size_t u_at = 0; u_at < s_long.size(); u_at += 7) {
        o_hash.update(s_long.substr(u_at, 7));
    }
    EXPECT_EQ(s_expected, o_hash.hex_digest());

    // Calling hex_digest twice must not change the answer.
    EXPECT_EQ(s_expected, o_hash.hex_digest());

    // A null byte is data, not a terminator.
    const std::string s_binary("a\0b", 3);
    EXPECT_EQ(sha1_hex(s_binary), sha1_hex(s_binary));
    EXPECT_FALSE(sha1_hex(s_binary) == sha1_hex("a"));

    return pkpass_test::finish("sha1-test");
}
