#include "crypto/sha1.h"

namespace gnome_pkpass::crypto {
namespace {

constexpr std::uint32_t rotate_left(std::uint32_t u_value, int i_bits) {
    return (u_value << i_bits) | (u_value >> (32 - i_bits));
}

constexpr int DIGITS_PER_WORD = 8;

}  // namespace

void sha1_c::transform(const std::uint8_t *p_block) {
    std::uint32_t o_words[80];

    for (int i_word = 0; i_word < 16; ++i_word) {
        o_words[i_word] = (static_cast<std::uint32_t>(p_block[i_word * 4]) << 24) |
                          (static_cast<std::uint32_t>(p_block[i_word * 4 + 1]) << 16) |
                          (static_cast<std::uint32_t>(p_block[i_word * 4 + 2]) << 8) |
                          (static_cast<std::uint32_t>(p_block[i_word * 4 + 3]));
    }
    for (int i_word = 16; i_word < 80; ++i_word) {
        o_words[i_word] = rotate_left(o_words[i_word - 3] ^ o_words[i_word - 8] ^
                                          o_words[i_word - 14] ^ o_words[i_word - 16],
                                      1);
    }

    std::uint32_t u_a = o_state_[0];
    std::uint32_t u_b = o_state_[1];
    std::uint32_t u_c = o_state_[2];
    std::uint32_t u_d = o_state_[3];
    std::uint32_t u_e = o_state_[4];

    for (int i_word = 0; i_word < 80; ++i_word) {
        std::uint32_t u_function = 0;
        std::uint32_t u_constant = 0;
        if (20 > i_word) {
            u_function = (u_b & u_c) | ((~u_b) & u_d);
            u_constant = 0x5a827999U;
        } else if (40 > i_word) {
            u_function = u_b ^ u_c ^ u_d;
            u_constant = 0x6ed9eba1U;
        } else if (60 > i_word) {
            u_function = (u_b & u_c) | (u_b & u_d) | (u_c & u_d);
            u_constant = 0x8f1bbcdcU;
        } else {
            u_function = u_b ^ u_c ^ u_d;
            u_constant = 0xca62c1d6U;
        }

        const std::uint32_t u_temporary =
            rotate_left(u_a, 5) + u_function + u_e + u_constant + o_words[i_word];
        u_e = u_d;
        u_d = u_c;
        u_c = rotate_left(u_b, 30);
        u_b = u_a;
        u_a = u_temporary;
    }

    o_state_[0] += u_a;
    o_state_[1] += u_b;
    o_state_[2] += u_c;
    o_state_[3] += u_d;
    o_state_[4] += u_e;
}

void sha1_c::update_raw(const std::uint8_t *p_data, std::size_t u_size) {
    for (std::size_t u_index = 0; u_index < u_size; ++u_index) {
        o_buffer_[u_buffered_] = p_data[u_index];
        ++u_buffered_;
        if (64 == u_buffered_) {
            transform(o_buffer_);
            u_buffered_ = 0;
        }
    }
}

void sha1_c::update(const std::uint8_t *p_data, std::size_t u_size) {
    if (b_final_ || 0 == u_size) {
        return;
    }
    u_length_ += static_cast<std::uint64_t>(u_size) * 8;
    update_raw(p_data, u_size);
}

void sha1_c::update(const std::string &s_text) {
    update(reinterpret_cast<const std::uint8_t *>(s_text.data()), s_text.size());
}

std::string sha1_c::hex_digest() {
    if (!b_final_) {
        b_final_ = true;

        // The message length in bits, before the padding is appended.
        const std::uint64_t u_bits = u_length_;

        std::uint8_t o_padding[64] = {};
        o_padding[0] = 0x80;
        const std::size_t u_padding =
            (56 > u_buffered_) ? (56 - u_buffered_) : (120 - u_buffered_);
        update_raw(o_padding, u_padding);

        std::uint8_t o_length[8];
        for (int i_byte = 0; i_byte < 8; ++i_byte) {
            o_length[i_byte] =
                static_cast<std::uint8_t>((u_bits >> (56 - i_byte * 8)) & 0xffU);
        }
        update_raw(o_length, 8);

        static const char *const s_hex = "0123456789abcdef";
        s_digest_.clear();
        s_digest_.reserve(5 * DIGITS_PER_WORD);
        for (int i_word = 0; i_word < 5; ++i_word) {
            for (int i_nibble = DIGITS_PER_WORD - 1; 0 <= i_nibble; --i_nibble) {
                s_digest_.push_back(
                    s_hex[(o_state_[i_word] >> (i_nibble * 4)) & 0x0fU]);
            }
        }
    }

    return s_digest_;
}

std::string sha1_hex(const std::string &s_text) {
    sha1_c o_hash;
    o_hash.update(s_text);
    return o_hash.hex_digest();
}

std::string sha1_hex(const std::uint8_t *p_data, std::size_t u_size) {
    sha1_c o_hash;
    o_hash.update(p_data, u_size);
    return o_hash.hex_digest();
}

}  // namespace gnome_pkpass::crypto
