#include "qr/qr_encoder.h"

#include <zlib.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace gnome_pkpass::qr {
namespace {

constexpr int MIN_VERSION = 1;
constexpr int MAX_VERSION = 10;
constexpr int MODE_BYTE = 0x4;

// The format-information bits for each level, in the order of level_e.
constexpr int LEVEL_BITS[] = {1, 0, 3, 2};  // L, M, Q, H

// One run of equal-sized blocks: ISO/IEC 18004 Table 9, the
// "error correction characteristics".
struct block_group_o {
    int i_blocks;
    int i_total;  // codewords per block
    int i_data;   // data codewords per block
};

struct version_ecc_o {
    block_group_o o_first;
    block_group_o o_second;  // all zero when the version and level have one group
    int i_group_count;
};

const block_group_o &group_at(const version_ecc_o &o_version, int i_group) {
    return (0 == i_group) ? o_version.o_first : o_version.o_second;
}

// Versions 1 to 10, levels L, M, Q, H.  A version and level with two group
// sizes (the second group holds one more codeword per block) carries both; the
// rest carry one.
const version_ecc_o VERSION_ECC[MAX_VERSION + 1][4] = {
    {},  // version 0 is unused
    {{{1, 26, 19}, {0, 0, 0}, 1}, {{1, 26, 16}, {0, 0, 0}, 1}, {{1, 26, 13}, {0, 0, 0}, 1}, {{1, 26, 9}, {0, 0, 0}, 1}},
    {{{1, 44, 34}, {0, 0, 0}, 1}, {{1, 44, 28}, {0, 0, 0}, 1}, {{1, 44, 22}, {0, 0, 0}, 1}, {{1, 44, 16}, {0, 0, 0}, 1}},
    {{{1, 70, 55}, {0, 0, 0}, 1}, {{1, 70, 44}, {0, 0, 0}, 1}, {{2, 35, 17}, {0, 0, 0}, 1}, {{2, 35, 13}, {0, 0, 0}, 1}},
    {{{1, 100, 80}, {0, 0, 0}, 1}, {{2, 50, 32}, {0, 0, 0}, 1}, {{2, 50, 24}, {0, 0, 0}, 1}, {{4, 25, 9}, {0, 0, 0}, 1}},
    {{{1, 134, 108}, {0, 0, 0}, 1}, {{2, 67, 43}, {0, 0, 0}, 1}, {{2, 33, 15}, {2, 34, 16}, 2}, {{2, 33, 11}, {2, 34, 12}, 2}},
    {{{2, 86, 68}, {0, 0, 0}, 1}, {{4, 43, 27}, {0, 0, 0}, 1}, {{4, 43, 19}, {0, 0, 0}, 1}, {{4, 43, 15}, {0, 0, 0}, 1}},
    {{{2, 98, 78}, {0, 0, 0}, 1}, {{4, 49, 31}, {0, 0, 0}, 1}, {{2, 32, 14}, {4, 33, 15}, 2}, {{4, 39, 13}, {1, 40, 14}, 2}},
    {{{2, 121, 97}, {0, 0, 0}, 1}, {{2, 60, 38}, {2, 61, 39}, 2}, {{4, 40, 18}, {2, 41, 19}, 2}, {{4, 40, 14}, {2, 41, 15}, 2}},
    {{{2, 146, 116}, {0, 0, 0}, 1}, {{3, 58, 36}, {2, 59, 37}, 2}, {{4, 36, 16}, {4, 37, 17}, 2}, {{4, 36, 12}, {4, 37, 13}, 2}},
    {{{2, 86, 68}, {2, 87, 69}, 2}, {{4, 69, 43}, {1, 70, 44}, 2}, {{6, 43, 19}, {2, 44, 20}, 2}, {{6, 43, 15}, {2, 44, 16}, 2}},
};

// The alignment-pattern centre coordinates for versions 1 to 10; version 1 has
// none.  A pair whose two coordinates are both an end lands on a finder
// pattern and is skipped.
const std::vector<int> &alignment_positions(int i_version) {
    static const std::vector<int> o_positions[MAX_VERSION + 1] = {
        {},
        {},
        {6, 18},
        {6, 22},
        {6, 26},
        {6, 30},
        {6, 34},
        {6, 22, 38},
        {6, 24, 42},
        {6, 26, 46},
        {6, 28, 50},
    };
    return o_positions[i_version];
}

// GF(256) with the primitive polynomial x^8+x^4+x^3+x^2+1, the field QR uses.
struct field_o {
    std::uint8_t o_exp[512] = {};
    std::uint8_t o_log[256] = {};

    field_o() {
        int i_value = 1;
        for (int i_power = 0; i_power < 255; ++i_power) {
            o_exp[i_power] = static_cast<std::uint8_t>(i_value);
            o_log[i_value] = static_cast<std::uint8_t>(i_power);
            i_value <<= 1;
            if (0 != (i_value & 0x100)) {
                i_value ^= 0x11d;
            }
        }
        for (int i_power = 255; i_power < 512; ++i_power) {
            o_exp[i_power] = o_exp[i_power - 255];
        }
    }

    std::uint8_t multiply(std::uint8_t u_left, std::uint8_t u_right) const {
        if (0 == u_left || 0 == u_right) {
            return 0;
        }
        return o_exp[o_log[u_left] + o_log[u_right]];
    }
};

const field_o &field() {
    static const field_o o_field;
    return o_field;
}

// The Reed-Solomon divisor for a block of i_degree error correction codewords:
// the generator polynomial's coefficients below its leading term, highest
// power first.  For degree 10 this is
// 216, 194, 159, 111, 199, 94, 95, 113, 157, 193.
std::vector<std::uint8_t> rs_divisor(int i_degree) {
    std::vector<std::uint8_t> o_result(static_cast<std::size_t>(i_degree), 0);
    o_result[static_cast<std::size_t>(i_degree) - 1] = 1;
    std::uint8_t u_root = 1;
    for (int i_step = 0; i_step < i_degree; ++i_step) {
        for (std::size_t u_index = 0; u_index < o_result.size(); ++u_index) {
            o_result[u_index] = field().multiply(o_result[u_index], u_root);
            if (u_index + 1 < o_result.size()) {
                o_result[u_index] ^= o_result[u_index + 1];
            }
        }
        u_root = field().multiply(u_root, 0x02);
    }
    return o_result;
}

// The remainder of the data polynomial divided by the divisor: the error
// correction codewords for one block.
std::vector<std::uint8_t> rs_remainder(const std::vector<std::uint8_t> &o_data,
                                       const std::vector<std::uint8_t> &o_divisor) {
    std::vector<std::uint8_t> o_result(o_divisor.size(), 0);
    for (const std::uint8_t u_byte : o_data) {
        const std::uint8_t u_factor = static_cast<std::uint8_t>(u_byte ^ o_result[0]);
        for (std::size_t u_index = 0; u_index + 1 < o_result.size(); ++u_index) {
            o_result[u_index] = o_result[u_index + 1];
        }
        if (!o_result.empty()) {
            o_result.back() = 0;
        }
        for (std::size_t u_index = 0; u_index < o_result.size(); ++u_index) {
            o_result[u_index] ^= field().multiply(o_divisor[u_index], u_factor);
        }
    }
    return o_result;
}

int count_bits_for_version(int i_version) {
    return (10 > i_version) ? 8 : 16;
}

int data_codewords(int i_version, level_e e_level) {
    if (MIN_VERSION > i_version || MAX_VERSION < i_version) {
        return 0;
    }
    const version_ecc_o &o_version = VERSION_ECC[i_version][static_cast<int>(e_level)];
    int i_total = 0;
    for (int i_group = 0; i_group < o_version.i_group_count; ++i_group) {
        i_total += group_at(o_version, i_group).i_blocks * group_at(o_version, i_group).i_data;
    }
    return i_total;
}

// Build the data codewords: the mode indicator, the byte count, the payload,
// the terminator, and the alternating pad bytes.
//
// The bit buffer is sized once and written by index rather than grown, because
// a growing bit vector is the shape that trips a known GCC 15 false positive
// (`-Wfree-nonheap-object`) at -O3, and a fixed buffer is also simply faster.
std::vector<std::uint8_t> build_data_codewords(const std::string &s_data, int i_version,
                                               level_e e_level) {
    const int i_capacity = data_codewords(i_version, e_level);
    const std::size_t u_capacity_bits = static_cast<std::size_t>(i_capacity) * 8;
    std::vector<std::uint8_t> o_bits(u_capacity_bits, 0);
    std::size_t u_at = 0;

    const auto push_bits = [&o_bits, &u_at](unsigned int u_value, int i_count) {
        for (int i_bit = i_count - 1; 0 <= i_bit; --i_bit) {
            o_bits[u_at] = static_cast<std::uint8_t>((u_value >> i_bit) & 1U);
            ++u_at;
        }
    };

    push_bits(MODE_BYTE, 4);
    push_bits(static_cast<unsigned int>(s_data.size()), count_bits_for_version(i_version));
    for (const char c_byte : s_data) {
        push_bits(static_cast<unsigned int>(static_cast<unsigned char>(c_byte)), 8);
    }

    // The terminator is up to four zero bits, and the byte padding is zero bits;
    // the buffer is already zero, so only the cursor has to move.
    u_at += std::min<std::size_t>(4, u_capacity_bits - u_at);
    while (0 != (u_at % 8) && u_at < u_capacity_bits) {
        ++u_at;
    }

    const std::size_t u_used = (u_at + 7) / 8;
    std::vector<std::uint8_t> o_codewords(static_cast<std::size_t>(i_capacity), 0);
    for (std::size_t u_byte = 0; u_byte < u_used && u_byte < o_codewords.size(); ++u_byte) {
        std::uint8_t u_value = 0;
        for (int i_bit = 0; i_bit < 8; ++i_bit) {
            u_value = static_cast<std::uint8_t>(
                (u_value << 1) | o_bits[u_byte * 8 + static_cast<std::size_t>(i_bit)]);
        }
        o_codewords[u_byte] = u_value;
    }

    // The two pad codewords alternate until the block is full.
    bool b_use_ec = true;
    for (std::size_t u_byte = u_used; u_byte < o_codewords.size(); ++u_byte) {
        o_codewords[u_byte] = b_use_ec ? 0xec : 0x11;
        b_use_ec = !b_use_ec;
    }
    return o_codewords;
}

// Split the codewords into blocks, compute each block's error correction
// codewords, and interleave both as ISO/IEC 18004 section 8.6 requires.
std::vector<std::uint8_t> interleave(const std::vector<std::uint8_t> &o_codewords, int i_version,
                                     level_e e_level) {
    const version_ecc_o &o_version = VERSION_ECC[i_version][static_cast<int>(e_level)];

    std::vector<std::vector<std::uint8_t>> o_blocks_data;
    std::vector<std::vector<std::uint8_t>> o_blocks_ecc;
    std::size_t u_at = 0;
    int i_ecc_length = 0;

    for (int i_group = 0; i_group < o_version.i_group_count; ++i_group) {
        const block_group_o &o_group = group_at(o_version, i_group);
        i_ecc_length = o_group.i_total - o_group.i_data;
        const std::vector<std::uint8_t> o_divisor = rs_divisor(i_ecc_length);
        for (int i_block = 0; i_block < o_group.i_blocks; ++i_block) {
            std::vector<std::uint8_t> o_data(
                o_codewords.begin() + static_cast<std::ptrdiff_t>(u_at),
                o_codewords.begin() + static_cast<std::ptrdiff_t>(u_at) + o_group.i_data);
            u_at += static_cast<std::size_t>(o_group.i_data);
            o_blocks_data.push_back(o_data);
            o_blocks_ecc.push_back(rs_remainder(o_data, o_divisor));
        }
    }

    std::vector<std::uint8_t> o_result;
    o_result.reserve(o_codewords.size() + static_cast<std::size_t>(i_ecc_length) * o_blocks_data.size());

    std::size_t u_longest = 0;
    for (const std::vector<std::uint8_t> &o_block : o_blocks_data) {
        u_longest = std::max(u_longest, o_block.size());
    }
    for (std::size_t u_index = 0; u_index < u_longest; ++u_index) {
        for (const std::vector<std::uint8_t> &o_block : o_blocks_data) {
            if (u_index < o_block.size()) {
                o_result.push_back(o_block[u_index]);
            }
        }
    }
    for (int i_index = 0; i_index < i_ecc_length; ++i_index) {
        for (const std::vector<std::uint8_t> &o_block : o_blocks_ecc) {
            o_result.push_back(o_block[static_cast<std::size_t>(i_index)]);
        }
    }
    return o_result;
}

bool mask_applies(int i_mask, int i_x, int i_y) {
    switch (i_mask) {
        case 0:
            return 0 == ((i_x + i_y) % 2);
        case 1:
            return 0 == (i_y % 2);
        case 2:
            return 0 == (i_x % 3);
        case 3:
            return 0 == ((i_x + i_y) % 3);
        case 4:
            return 0 == ((i_y / 2 + i_x / 3) % 2);
        case 5:
            return 0 == ((i_x * i_y) % 2 + (i_x * i_y) % 3);
        case 6:
            return 0 == (((i_x * i_y) % 2 + (i_x * i_y) % 3) % 2);
        case 7:
            return 0 == (((i_x + i_y) % 2 + (i_x * i_y) % 3) % 2);
        default:
            return false;
    }
}

// The matrix under construction: the modules, and which of them belong to a
// function pattern and so are never masked or written with data.
struct builder_o {
    int i_version;
    int i_size;
    std::vector<std::uint8_t> o_modules;
    std::vector<std::uint8_t> o_function;

    explicit builder_o(int i_version_in)
        : i_version(i_version_in),
          i_size(17 + 4 * i_version_in),
          o_modules(static_cast<std::size_t>(17 + 4 * i_version_in) * static_cast<std::size_t>(17 + 4 * i_version_in), 0),
          o_function(static_cast<std::size_t>(17 + 4 * i_version_in) * static_cast<std::size_t>(17 + 4 * i_version_in), 0) {}

    void set(int i_x, int i_y, bool b_dark) {
        o_modules[static_cast<std::size_t>(i_y) * static_cast<std::size_t>(i_size) +
                  static_cast<std::size_t>(i_x)] = b_dark ? 1 : 0;
    }

    void set_function(int i_x, int i_y, bool b_dark) {
        set(i_x, i_y, b_dark);
        o_function[static_cast<std::size_t>(i_y) * static_cast<std::size_t>(i_size) +
                    static_cast<std::size_t>(i_x)] = 1;
    }

    bool is_function(int i_x, int i_y) const {
        return 0 != o_function[static_cast<std::size_t>(i_y) * static_cast<std::size_t>(i_size) +
                                static_cast<std::size_t>(i_x)];
    }

    bool is_dark(int i_x, int i_y) const {
        return 0 != o_modules[static_cast<std::size_t>(i_y) * static_cast<std::size_t>(i_size) +
                              static_cast<std::size_t>(i_x)];
    }

    void draw_finder(int i_center_x, int i_center_y) {
        for (int i_dy = -4; 4 >= i_dy; ++i_dy) {
            for (int i_dx = -4; 4 >= i_dx; ++i_dx) {
                const int i_x = i_center_x + i_dx;
                const int i_y = i_center_y + i_dy;
                if (0 > i_x || 0 > i_y || i_size <= i_x || i_size <= i_y) {
                    continue;
                }
                const int i_distance = std::max(std::abs(i_dx), std::abs(i_dy));
                set_function(i_x, i_y, 2 != i_distance && 4 != i_distance);
            }
        }
    }

    void draw_alignment(int i_center_x, int i_center_y) {
        for (int i_dy = -2; 2 >= i_dy; ++i_dy) {
            for (int i_dx = -2; 2 >= i_dx; ++i_dx) {
                const int i_distance = std::max(std::abs(i_dx), std::abs(i_dy));
                set_function(i_center_x + i_dx, i_center_y + i_dy, 1 != i_distance);
            }
        }
    }

    void draw_function_patterns() {
        // Timing patterns.
        for (int i_index = 0; i_index < i_size; ++i_index) {
            set_function(6, i_index, 0 == (i_index % 2));
            set_function(i_index, 6, 0 == (i_index % 2));
        }

        // Finder patterns with their separators.
        draw_finder(3, 3);
        draw_finder(i_size - 4, 3);
        draw_finder(3, i_size - 4);

        // Alignment patterns.  Exactly three of the coordinate pairs land on a
        // finder pattern corner and are skipped: left-top, left-bottom, and
        // right-top.  The right-bottom pair is kept, which is why version 2
        // carries one alignment pattern at (18, 18) and not none.
        const std::vector<int> &o_positions = alignment_positions(i_version);
        for (const int i_center_x : o_positions) {
            for (const int i_center_y : o_positions) {
                const bool b_left = (6 == i_center_x);
                const bool b_right = (i_size - 7 == i_center_x);
                const bool b_top = (6 == i_center_y);
                const bool b_bottom = (i_size - 7 == i_center_y);
                if ((b_left && b_top) || (b_left && b_bottom) || (b_right && b_top)) {
                    continue;
                }
                draw_alignment(i_center_x, i_center_y);
            }
        }

        // The format information area is reserved before the data is placed;
        // the bits themselves are written once the mask is chosen.  The two
        // modules at (8,6) and (6,8) belong to the timing patterns, so they are
        // left with the value the timing loop gave them.
        for (int i_index = 0; 8 >= i_index; ++i_index) {
            if (6 != i_index) {
                set_function(8, i_index, false);
                set_function(i_index, 8, false);
            }
        }
        for (int i_index = 0; 7 >= i_index; ++i_index) {
            set_function(i_size - 1 - i_index, 8, false);
            set_function(8, i_size - 1 - i_index, false);
        }

        // The version information, for a version that carries it.
        if (7 <= i_version) {
            int i_remainder = i_version;
            for (int i_step = 0; 12 > i_step; ++i_step) {
                i_remainder = (i_remainder << 1) ^ ((i_remainder >> 11) * 0x1f25);
            }
            const int i_bits = (i_version << 12) | i_remainder;
            for (int i_index = 0; 18 > i_index; ++i_index) {
                const bool b_dark = 0 != ((i_bits >> i_index) & 1);
                const int i_a = i_size - 11 + (i_index % 3);
                const int i_b = i_index / 3;
                set_function(i_a, i_b, b_dark);
                set_function(i_b, i_a, b_dark);
            }
        }


    }

    // Place the data bits in the zigzag order, from the bottom-right corner,
    // two columns at a time, skipping the timing column.
    void draw_codewords(const std::vector<std::uint8_t> &o_codewords) {
        std::size_t u_bit = 0;
        const std::size_t u_total = o_codewords.size() * 8;
        for (int i_right = i_size - 1; 1 <= i_right; i_right -= 2) {
            if (6 == i_right) {
                --i_right;
            }
            for (int i_step = 0; i_step < i_size; ++i_step) {
                for (int i_column = 0; 2 > i_column; ++i_column) {
                    const int i_x = i_right - i_column;
                    const bool b_upward = 0 == (((i_right + 1) & 2));
                    const int i_y = b_upward ? (i_size - 1 - i_step) : i_step;
                    if (is_function(i_x, i_y)) {
                        continue;
                    }
                    bool b_dark = false;
                    if (u_bit < u_total) {
                        b_dark = 0 != ((o_codewords[u_bit / 8] >> (7 - (u_bit % 8))) & 1);
                    }
                    set(i_x, i_y, b_dark);
                    ++u_bit;
                }
            }
        }
    }

    void apply_mask(int i_mask) {
        for (int i_y = 0; i_y < i_size; ++i_y) {
            for (int i_x = 0; i_x < i_size; ++i_x) {
                if (is_function(i_x, i_y)) {
                    continue;
                }
                if (mask_applies(i_mask, i_x, i_y)) {
                    set(i_x, i_y, !is_dark(i_x, i_y));
                }
            }
        }
    }

    void draw_format_bits(int i_mask, level_e e_level) {
        const int i_data = (LEVEL_BITS[static_cast<int>(e_level)] << 3) | i_mask;
        int i_remainder = i_data;
        for (int i_step = 0; 10 > i_step; ++i_step) {
            i_remainder = (i_remainder << 1) ^ ((i_remainder >> 9) * 0x537);
        }
        const int i_bits = ((i_data << 10) | i_remainder) ^ 0x5412;

        const auto bit_at = [i_bits](int i_index) { return 0 != ((i_bits >> i_index) & 1); };

        for (int i_index = 0; 6 > i_index; ++i_index) {
            set_function(8, i_index, bit_at(i_index));
        }
        set_function(8, 7, bit_at(6));
        set_function(8, 8, bit_at(7));
        set_function(7, 8, bit_at(8));
        for (int i_index = 9; 15 > i_index; ++i_index) {
            set_function(14 - i_index, 8, bit_at(i_index));
        }
        for (int i_index = 0; 8 > i_index; ++i_index) {
            set_function(i_size - 1 - i_index, 8, bit_at(i_index));
        }
        for (int i_index = 8; 15 > i_index; ++i_index) {
            set_function(8, i_size - 15 + i_index, bit_at(i_index));
        }
        // The module above the lower-left format copy is always dark.
        set_function(8, i_size - 8, true);
    }

    int penalty() const {
        int i_penalty = 0;

        // Rule 1: runs of five or more equal modules in a row or a column.
        for (int i_line = 0; i_line < i_size; ++i_line) {
            for (int i_axis = 0; 2 > i_axis; ++i_axis) {
                int i_run = 1;
                for (int i_at = 1; i_at < i_size; ++i_at) {
                    const bool b_previous =
                        (0 == i_axis) ? is_dark(i_at - 1, i_line) : is_dark(i_line, i_at - 1);
                    const bool b_current =
                        (0 == i_axis) ? is_dark(i_at, i_line) : is_dark(i_line, i_at);
                    if (b_current == b_previous) {
                        ++i_run;
                        continue;
                    }
                    if (5 <= i_run) {
                        i_penalty += 3 + (i_run - 5);
                    }
                    i_run = 1;
                }
                if (5 <= i_run) {
                    i_penalty += 3 + (i_run - 5);
                }
            }
        }

        // Rule 2: 2x2 blocks of one colour.
        for (int i_y = 0; i_y + 1 < i_size; ++i_y) {
            for (int i_x = 0; i_x + 1 < i_size; ++i_x) {
                const bool b_dark = is_dark(i_x, i_y);
                if (b_dark == is_dark(i_x + 1, i_y) && b_dark == is_dark(i_x, i_y + 1) &&
                    b_dark == is_dark(i_x + 1, i_y + 1)) {
                    i_penalty += 3;
                }
            }
        }

        // Rule 3: the finder-like pattern, with four light modules on either
        // side, in a row or a column.
        static const char *const s_patterns[] = {"10111010000", "00001011101"};
        for (int i_line = 0; i_line < i_size; ++i_line) {
            for (int i_axis = 0; 2 > i_axis; ++i_axis) {
                for (int i_at = 0; i_at + 11 <= i_size; ++i_at) {
                    for (const char *s_pattern : s_patterns) {
                        bool b_match = true;
                        for (int i_step = 0; 11 > i_step; ++i_step) {
                            const bool b_dark =
                                (0 == i_axis) ? is_dark(i_at + i_step, i_line)
                                              : is_dark(i_line, i_at + i_step);
                            if (b_dark != ('1' == s_pattern[i_step])) {
                                b_match = false;
                                break;
                            }
                        }
                        if (b_match) {
                            i_penalty += 40;
                        }
                    }
                }
            }
        }

        // Rule 4: how far the dark proportion is from one half.
        int i_dark = 0;
        for (const std::uint8_t u_module : o_modules) {
            i_dark += (0 != u_module) ? 1 : 0;
        }
        const int i_total = i_size * i_size;
        i_penalty += 10 * (std::abs(i_dark * 100 - i_total * 50) / (i_total * 5));

        return i_penalty;
    }
};

void append_u32_be(std::string &s_text, std::uint32_t u_value) {
    s_text.push_back(static_cast<char>((u_value >> 24) & 0xff));
    s_text.push_back(static_cast<char>((u_value >> 16) & 0xff));
    s_text.push_back(static_cast<char>((u_value >> 8) & 0xff));
    s_text.push_back(static_cast<char>(u_value & 0xff));
}

void append_chunk(std::string &s_png, const char *s_type, const std::string &s_data) {
    append_u32_be(s_png, static_cast<std::uint32_t>(s_data.size()));
    const std::size_t u_start = s_png.size();
    s_png.append(s_type, 4);
    s_png.append(s_data);
    const std::uint32_t u_crc = static_cast<std::uint32_t>(
        crc32(0, reinterpret_cast<const Bytef *>(s_png.data() + u_start),
              static_cast<uInt>(4 + s_data.size())));
    append_u32_be(s_png, u_crc);
}

bool encode_impl(const std::string &s_data, level_e e_level, int i_forced_mask, matrix_o &o_matrix,
                 std::string &s_error) {
    int i_version = 0;
    for (int i_candidate = MIN_VERSION; i_candidate <= MAX_VERSION; ++i_candidate) {
        const int i_capacity = capacity_bytes(i_candidate, e_level);
        if (0 < i_capacity && static_cast<int>(s_data.size()) <= i_capacity) {
            i_version = i_candidate;
            break;
        }
    }
    if (0 == i_version) {
        std::ostringstream s_stream;
        s_stream << "the barcode payload is " << s_data.size()
                 << " bytes, which does not fit a QR version 1 to 10 at level "
                 << level_name(e_level);
        s_error = s_stream.str();
        return false;
    }

    const std::vector<std::uint8_t> o_data = build_data_codewords(s_data, i_version, e_level);
    const std::vector<std::uint8_t> o_codewords = interleave(o_data, i_version, e_level);

    int i_best_mask = 0;
    if (0 <= i_forced_mask) {
        i_best_mask = i_forced_mask;
    }

    builder_o o_builder(i_version);
    o_builder.draw_function_patterns();
    o_builder.draw_codewords(o_codewords);
    o_builder.apply_mask(i_best_mask);
    o_builder.draw_format_bits(i_best_mask, e_level);

    if (0 > i_forced_mask) {
        int i_best_penalty = -1;
        for (int i_mask = 0; 8 > i_mask; ++i_mask) {
            builder_o o_trial(i_version);
            o_trial.draw_function_patterns();
            o_trial.draw_codewords(o_codewords);
            o_trial.apply_mask(i_mask);
            o_trial.draw_format_bits(i_mask, e_level);
            const int i_penalty = o_trial.penalty();
            if (0 > i_best_penalty || i_penalty < i_best_penalty) {
                i_best_penalty = i_penalty;
                i_best_mask = i_mask;
            }
        }
        o_builder = builder_o(i_version);
        o_builder.draw_function_patterns();
        o_builder.draw_codewords(o_codewords);
        o_builder.apply_mask(i_best_mask);
        o_builder.draw_format_bits(i_best_mask, e_level);
    }

    o_matrix.i_version = i_version;
    o_matrix.i_size = o_builder.i_size;
    o_matrix.e_level = e_level;
    o_matrix.i_mask = i_best_mask;
    o_matrix.o_modules = o_builder.o_modules;
    return true;
}

}  // namespace

bool matrix_o::is_dark(int i_x, int i_y) const {
    if (0 > i_x || 0 > i_y || i_size <= i_x || i_size <= i_y) {
        return false;
    }
    return 0 != o_modules[static_cast<std::size_t>(i_y) * static_cast<std::size_t>(i_size) +
                          static_cast<std::size_t>(i_x)];
}

const char *level_name(level_e e_level) {
    switch (e_level) {
        case level_e::low:
            return "L";
        case level_e::medium:
            return "M";
        case level_e::quartile:
            return "Q";
        case level_e::high:
        default:
            return "H";
    }
}

int capacity_bytes(int i_version, level_e e_level) {
    const int i_codewords = data_codewords(i_version, e_level);
    if (0 == i_codewords) {
        return 0;
    }
    const int i_bits = i_codewords * 8 - 4 - count_bits_for_version(i_version);
    return (0 < i_bits) ? (i_bits / 8) : 0;
}

bool encode(const std::string &s_data, level_e e_level, matrix_o &o_matrix, std::string &s_error) {
    return encode_impl(s_data, e_level, -1, o_matrix, s_error);
}

bool encode_masked(const std::string &s_data, level_e e_level, int i_mask, matrix_o &o_matrix,
                   std::string &s_error) {
    if (0 > i_mask || 7 < i_mask) {
        s_error = "a mask is a number from 0 to 7";
        return false;
    }
    return encode_impl(s_data, e_level, i_mask, o_matrix, s_error);
}

std::string to_text(const matrix_o &o_matrix) {
    std::string s_text;
    for (int i_y = 0; i_y < o_matrix.i_size; ++i_y) {
        for (int i_x = 0; i_x < o_matrix.i_size; ++i_x) {
            s_text.push_back(o_matrix.is_dark(i_x, i_y) ? '1' : '0');
        }
        if (i_y + 1 < o_matrix.i_size) {
            s_text.push_back('\n');
        }
    }
    return s_text;
}

std::string to_png(const matrix_o &o_matrix, int i_scale, int i_quiet) {
    const int i_side = (o_matrix.i_size + 2 * i_quiet) * i_scale;

    // One scanline per pixel row: a filter byte and the grayscale samples.
    std::string s_raw;
    s_raw.reserve(static_cast<std::size_t>(i_side) * (static_cast<std::size_t>(i_side) + 1));
    for (int i_y = 0; i_y < i_side; ++i_y) {
        s_raw.push_back('\0');
        for (int i_x = 0; i_x < i_side; ++i_x) {
            const int i_module_x = i_x / i_scale - i_quiet;
            const int i_module_y = i_y / i_scale - i_quiet;
            const bool b_dark = o_matrix.is_dark(i_module_x, i_module_y);
            s_raw.push_back(b_dark ? '\0' : static_cast<char>(0xff));
        }
    }

    uLongf u_compressed_size = compressBound(static_cast<uLong>(s_raw.size()));
    std::string s_compressed(static_cast<std::size_t>(u_compressed_size), '\0');
    if (Z_OK != compress2(reinterpret_cast<Bytef *>(&s_compressed[0]), &u_compressed_size,
                          reinterpret_cast<const Bytef *>(s_raw.data()),
                          static_cast<uLong>(s_raw.size()), 9)) {
        return std::string();
    }
    s_compressed.resize(static_cast<std::size_t>(u_compressed_size));

    std::string s_png("\x89PNG\r\n\x1a\n", 8);
    std::string s_header;
    append_u32_be(s_header, static_cast<std::uint32_t>(i_side));
    append_u32_be(s_header, static_cast<std::uint32_t>(i_side));
    s_header.push_back(8);  // bit depth
    s_header.push_back(0);  // colour type: grayscale
    s_header.push_back(0);  // compression
    s_header.push_back(0);  // filter
    s_header.push_back(0);  // interlace
    append_chunk(s_png, "IHDR", s_header);
    append_chunk(s_png, "IDAT", s_compressed);
    append_chunk(s_png, "IEND", std::string());
    return s_png;
}

std::string to_svg_path(const matrix_o &o_matrix, int i_x, int i_y, int i_pixel) {
    std::ostringstream s_path;
    for (int i_row = 0; i_row < o_matrix.i_size; ++i_row) {
        int i_run = 0;
        for (int i_column = 0; i_column <= o_matrix.i_size; ++i_column) {
            const bool b_dark =
                (i_column < o_matrix.i_size) && o_matrix.is_dark(i_column, i_row);
            if (b_dark) {
                ++i_run;
                continue;
            }
            if (0 < i_run) {
                const int i_left = i_x + (i_column - i_run) * i_pixel;
                const int i_top = i_y + i_row * i_pixel;
                s_path << 'M' << i_left << ' ' << i_top << 'h' << i_run * i_pixel << 'v' << i_pixel
                       << 'h' << -(i_run * i_pixel) << 'z';
                i_run = 0;
            }
        }
    }
    return s_path.str();
}

}  // namespace gnome_pkpass::qr
