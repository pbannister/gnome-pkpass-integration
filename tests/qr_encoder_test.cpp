//
// qr_encoder_test.cpp: the QR encoder against ground truth from an independent
// implementation.
//
// The expected matrices below were produced by libqrencode (the C library that
// most Linux tooling uses) in explicit 8-bit byte mode, at the mask it chose;
// the test forces that mask, because two correct encoders may otherwise pick
// different (equally valid) masks.  A matrix that matches module for module
// cannot be a symbol that fails to scan.
//
// The vectors cover version 1, version 3, version 2 at level H, and version 7.
// Version 7 brings in the version information and the three-column alignment
// pattern layout; version 2 at H exercises the two-block interleave.
//
#include "qr/qr_encoder.h"

#include "test_harness.h"

#include <string>
#include <vector>

namespace {

// "HELLO WORLD", level M, version 1, mask 3, 21x21.
const char *const VECTOR_ONE[] = {
    "111111101000101111111", "100000101000101000001", "101110100000001011101",
    "101110101010101011101", "101110100111001011101", "100000100011101000001",
    "111111101010101111111", "000000001111100000000", "101101110101101001011",
    "011000010111111101100", "000001111101010100011", "101011011001000101010",
    "100010110110110000101", "000000001011001100101", "111111101011111110000",
    "100000101110010101111", "101110100100101001000", "101110101110001001110",
    "101110101100100100100", "100000100111011110001", "111111101101010100000",
};
constexpr int VECTOR_ONE_VERSION = 1;
constexpr int VECTOR_ONE_MASK = 3;

// The fixture's barcode message, level M, version 3, mask 2, 29x29.
const char *const VECTOR_TWO[] = {
    "11111110011000011111001111111", "10000010011110000000001000001",
    "10111010111010101011001011101", "10111010100000000110001011101",
    "10111010101011110011001011101", "10000010100101111110101000001",
    "11111110101010101010101111111", "00000000101011011010000000000",
    "10111110011010101100101111100", "10010100000110001001001010111",
    "01010011110100010110010011000", "01101101101110100000010011010",
    "00111111000010010100110100100", "10010000001101100011001010001",
    "11011011110111111000010001100", "00001100000011010000010011011",
    "00101010101110111100110000110", "11001001011010000011001010111",
    "10101111010100011010010001110", "10111001101000110000010001001",
    "10000011001100010100111110111", "00000000101001111001100011001",
    "11111110011001110001101011000", "10000010111011000010100011001",
    "10111010100000110110111111110", "10111010110011000010010101011",
    "10111010101001010110001010010", "10000010010000000011110000010",
    "11111110111101001110010010100",
};
constexpr int VECTOR_TWO_VERSION = 3;
constexpr int VECTOR_TWO_MASK = 2;

// A 110-byte payload, level M, version 7, mask 1, 45x45.
const char *const VECTOR_SEVEN[] = {
    "111111101010000010101010101100110000101111111",
    "100000100010011011001011010101010001001000001",
    "101110101111010111001011011010101101001011101",
    "101110100111010101010101010100110101101011101",
    "101110100011101110101111111100110111101011101",
    "100000101101100001101000110101010100001000001",
    "111111101010101010101010101010101010101111111",
    "000000000100100110101000110101010101000000000",
    "101000110000001010001111110011001100100100101",
    "001111011110100000010101010011001101010011010",
    "101110110111111110101101101010101011001011001",
    "000110001100000110100101010001010101010101001",
    "001100100101101110101011110001001100110010101",
    "001000011101100111000111010011001101010011010",
    "000100110100011010101100101000101011001011001",
    "000001000100100110110110110101010101010101001",
    "000101100011110100110110110011001100110010101",
    "010010011101101111010101010011001101010011010",
    "111010101100000110101100101010101011001011001",
    "011011000100111110110100111101010101010101001",
    "100111111011011010111111110011001100111110101",
    "111110001100000011011000111011001100100011010",
    "110010101001010011101010111010101010101011001",
    "010110001001010001001000110101010100100011001",
    "000111111110011011101111110011001101111110101",
    "101110010100001011001101010011001101101101010",
    "110000100011110010011010101010101011001101001",
    "010100010001110001111011010001010100110101001",
    "001100100110111001011011010111001101101010100",
    "101110010110101000101101010011001101101101010",
    "100000110101101010001011101010101011001101001",
    "001000001110100111011011110101010100110101001",
    "011101100100110000100011010011001101101010101",
    "011010010000110011001101110011001101101101010",
    "000010110101111100011110101010101011001101001",
    "011110001110101101011101000101010100110101001",
    "100110110101011010111111101011001101111110101",
    "000000001000011111011000100011001100100011010",
    "111111101001010110011010101010101010101011001",
    "100000100100111000011000110101010101100011001",
    "101110100001100011111111110011001101111110101",
    "101110100010011111101000110011001101010101001",
    "101110101010110110001010101010101010110011001",
    "100000100100111001010101010101010100110011000",
    "111111101110000010101100110011001100101010101",
};
constexpr int VECTOR_SEVEN_VERSION = 7;
constexpr int VECTOR_SEVEN_MASK = 1;

// "HELLO WORLD", level H, version 2, mask 3, 25x25.
const char *const VECTOR_HIGH[] = {
    "1111111000000111001111111", "1000001000101010001000001", "1011101001110010001011101",
    "1011101001100100101011101", "1011101011000100001011101", "1000001000010110101000001",
    "1111111010101010101111111", "0000000010011101000000000", "0011001110000010011010000",
    "1001100001100111101100101", "0001101000100100101000000", "0001100010101101000101110",
    "1001101110010101011101011", "0000010001010010110111110", "0111011101101001110010110",
    "1011100011001100101110001", "0001001000110001111110011", "0000000010001101100011000",
    "1111111011101101101010111", "1000001001011010100010001", "1011101000000100111110001",
    "1011101010010100001001001", "1011101011000010010110010", "1000001001011000111011100",
    "1111111001000110100100011",
};
constexpr int VECTOR_HIGH_VERSION = 2;
constexpr int VECTOR_HIGH_MASK = 3;

std::string expected_text(const char *const *o_rows, int i_size) {
    std::string s_text;
    for (int i_row = 0; i_row < i_size; ++i_row) {
        if (0 < i_row) {
            s_text.push_back('\n');
        }
        s_text += o_rows[i_row];
    }
    return s_text;
}

void check_vector(const char *s_name, const std::string &s_payload, gnome_pkpass::qr::level_e e_level,
                  int i_version, int i_mask, const char *const *o_rows, int i_size) {
    gnome_pkpass::qr::matrix_o o_matrix;
    std::string s_error;
    EXPECT_TRUE(gnome_pkpass::qr::encode_masked(s_payload, e_level, i_mask, o_matrix, s_error));
    EXPECT_EQ(i_version, o_matrix.i_version);
    EXPECT_EQ(i_size, o_matrix.i_size);
    EXPECT_EQ(i_mask, o_matrix.i_mask);
    if (expected_text(o_rows, i_size) != gnome_pkpass::qr::to_text(o_matrix)) {
        pkpass_test::fail(__FILE__, __LINE__,
                          std::string("the matrix for ") + s_name +
                              " differs from the libqrencode ground truth");
    }
}

// The format information is 15 bits of BCH over the level and the mask, XORed
// with a fixed pattern.  Reading it back proves the symbol says which mask and
// level it used, which is what a scanner relies on.
unsigned int read_format_bits(const gnome_pkpass::qr::matrix_o &o_matrix) {
    const std::pair<int, int> o_positions[] = {{8, 0}, {8, 1}, {8, 2}, {8, 3}, {8, 4}, {8, 5},
                                               {8, 7}, {8, 8}, {7, 8}, {5, 8}, {4, 8}, {3, 8},
                                               {2, 8}, {1, 8}, {0, 8}};
    unsigned int u_bits = 0;
    for (int i_index = 0; i_index < 15; ++i_index) {
        if (o_matrix.is_dark(o_positions[i_index].first, o_positions[i_index].second)) {
            u_bits |= (1U << i_index);
        }
    }
    return u_bits ^ 0x5412U;
}

void check_structure() {
    gnome_pkpass::qr::matrix_o o_matrix;
    std::string s_error;
    EXPECT_TRUE(gnome_pkpass::qr::encode_masked("STRUCTURE", gnome_pkpass::qr::level_e::quartile, 5,
                                                o_matrix, s_error));
    const int i_size = o_matrix.i_size;

    // The format information decodes to the level and mask that were asked for.
    // The 15 bits are five data bits (two of level, three of mask) followed by
    // ten BCH bits, so the level and mask sit in the top five.
    const unsigned int u_bits = read_format_bits(o_matrix);
    EXPECT_EQ(5, static_cast<int>((u_bits >> 10) & 7U));
    EXPECT_EQ(3, static_cast<int>((u_bits >> 13) & 3U));  // Q is 3 in the format field

    // Finder patterns: the outer ring is dark, the inner ring light, the centre
    // dark, and the separator light.
    const int o_corners[3][2] = {{0, 0}, {i_size - 7, 0}, {0, i_size - 7}};
    for (const auto &o_corner : o_corners) {
        EXPECT_TRUE(o_matrix.is_dark(o_corner[0] + 0, o_corner[1] + 0));
        EXPECT_TRUE(o_matrix.is_dark(o_corner[0] + 3, o_corner[1] + 3));
        EXPECT_FALSE(o_matrix.is_dark(o_corner[0] + 1, o_corner[1] + 1));
    }
    EXPECT_FALSE(o_matrix.is_dark(7, 7));  // top-left separator

    // Timing patterns alternate along row 6 and column 6.
    for (int i_index = 8; i_index < i_size - 8; ++i_index) {
        EXPECT_EQ((0 == (i_index % 2)) ? 1 : 0, o_matrix.is_dark(i_index, 6) ? 1 : 0);
        EXPECT_EQ((0 == (i_index % 2)) ? 1 : 0, o_matrix.is_dark(6, i_index) ? 1 : 0);
    }

    // The dark module is always set.
    EXPECT_TRUE(o_matrix.is_dark(8, i_size - 8));

    // Version 2 carries exactly one alignment pattern, centred at (18, 18).
    gnome_pkpass::qr::matrix_o o_version_two;
    EXPECT_TRUE(gnome_pkpass::qr::encode_masked("HELLO WORLD", gnome_pkpass::qr::level_e::high, 3,
                                                o_version_two, s_error));
    EXPECT_EQ(2, o_version_two.i_version);
    EXPECT_TRUE(o_version_two.is_dark(18, 18));
    EXPECT_TRUE(o_version_two.is_dark(16, 16));  // the pattern's outer corner
    EXPECT_FALSE(o_version_two.is_dark(17, 17));  // its light ring

    // Version information is present from version 7 and decodes back to 7.
    gnome_pkpass::qr::matrix_o o_version_seven;
    EXPECT_TRUE(gnome_pkpass::qr::encode_masked(std::string(110, 'V'),
                                                gnome_pkpass::qr::level_e::medium, 1,
                                                o_version_seven, s_error));
    EXPECT_EQ(7, o_version_seven.i_version);
    unsigned int u_version_bits = 0;
    for (int i_index = 0; i_index < 18; ++i_index) {
        if (o_version_seven.is_dark(o_version_seven.i_size - 11 + (i_index % 3), i_index / 3)) {
            u_version_bits |= (1U << i_index);
        }
    }
    EXPECT_EQ(7, static_cast<int>(u_version_bits >> 12));
}

}  // namespace

int main() {
    using gnome_pkpass::qr::capacity_bytes;
    using gnome_pkpass::qr::encode;
    using gnome_pkpass::qr::level_e;
    using gnome_pkpass::qr::level_name;
    using gnome_pkpass::qr::matrix_o;
    using gnome_pkpass::qr::to_png;
    using gnome_pkpass::qr::to_svg_path;

    // Byte-mode capacities, as ISO/IEC 18004 Table 7 gives them.
    EXPECT_EQ(17, capacity_bytes(1, level_e::low));
    EXPECT_EQ(14, capacity_bytes(1, level_e::medium));
    EXPECT_EQ(11, capacity_bytes(1, level_e::quartile));
    EXPECT_EQ(7, capacity_bytes(1, level_e::high));
    EXPECT_EQ(271, capacity_bytes(10, level_e::low));
    EXPECT_EQ(213, capacity_bytes(10, level_e::medium));
    EXPECT_EQ(0, capacity_bytes(0, level_e::medium));
    EXPECT_EQ(0, capacity_bytes(11, level_e::medium));

    EXPECT_EQ(std::string("L"), level_name(level_e::low));
    EXPECT_EQ(std::string("M"), level_name(level_e::medium));
    EXPECT_EQ(std::string("Q"), level_name(level_e::quartile));
    EXPECT_EQ(std::string("H"), level_name(level_e::high));

    check_vector("HELLO WORLD", "HELLO WORLD", level_e::medium, VECTOR_ONE_VERSION,
                 VECTOR_ONE_MASK, VECTOR_ONE, 21);
    check_vector("the fixture message", "{\"ticket\":\"0000000001\",\"seat\":\"A1\"}",
                 level_e::medium, VECTOR_TWO_VERSION, VECTOR_TWO_MASK, VECTOR_TWO, 29);
    check_vector("a version 7 payload", std::string(110, 'V'), level_e::medium,
                 VECTOR_SEVEN_VERSION, VECTOR_SEVEN_MASK, VECTOR_SEVEN, 45);
    check_vector("HELLO WORLD at level H", "HELLO WORLD", level_e::high, VECTOR_HIGH_VERSION,
                 VECTOR_HIGH_MASK, VECTOR_HIGH, 25);

    check_structure();

    // The chosen mask is a real mask, and the version grows with the payload.
    matrix_o o_auto;
    std::string s_error;
    EXPECT_TRUE(encode("HELLO WORLD", level_e::medium, o_auto, s_error));
    EXPECT_TRUE(0 <= o_auto.i_mask && 7 >= o_auto.i_mask);
    EXPECT_EQ(1, o_auto.i_version);

    matrix_o o_large;
    EXPECT_TRUE(encode(std::string(200, 'z'), level_e::medium, o_large, s_error));
    EXPECT_EQ(10, o_large.i_version);

    // A payload that does not fit the versions this project encodes is refused
    // with a reason rather than drawn as a symbol that would not scan.
    matrix_o o_too_big;
    std::string s_too_big_error;
    EXPECT_FALSE(encode(std::string(400, 'z'), level_e::high, o_too_big, s_too_big_error));
    EXPECT_FALSE(s_too_big_error.empty());

    // The PNG starts with the signature, and the SVG path is a path.
    const std::string s_png = to_png(o_auto, 4, 4);
    EXPECT_TRUE(8 < s_png.size());
    EXPECT_EQ(std::string("\x89PNG\r\n\x1a\n", 8), s_png.substr(0, 8));
    const std::string s_path = to_svg_path(o_auto, 10, 20, 5);
    EXPECT_FALSE(s_path.empty());
    EXPECT_EQ('M', s_path[0]);
    EXPECT_TRUE(std::string::npos != s_path.find('h'));

    return pkpass_test::finish("qr-encoder-test");
}
