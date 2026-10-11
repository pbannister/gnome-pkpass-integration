#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gnome_pkpass::qr {

// The four error correction levels of ISO/IEC 18004, ordered as the format
// information orders them.
enum class level_e { low, medium, quartile, high };

// A rendered QR symbol: the module grid, and the parameters that produced it.
struct matrix_o {
    int i_version = 0;
    int i_size = 0;
    level_e e_level = level_e::medium;
    int i_mask = 0;
    // Row-major, i_size by i_size; 1 is a dark module.
    std::vector<std::uint8_t> o_modules;

    bool is_dark(int i_x, int i_y) const;
};

// Encode bytes in 8-bit byte mode, choosing the smallest version that fits.
//
// Only versions 1 to 10 are supported, which is between 14 and 271 bytes at
// level M depending on the version; the largest that fits here is 271 bytes at
// level L and 213 at level M.  A payload that does not fit is refused with a
// reason rather than drawn as a symbol that would not scan.
bool encode(const std::string &s_data, level_e e_level, matrix_o &o_matrix, std::string &s_error);

// The same, with the mask forced instead of chosen.  A test uses it to compare
// the matrix against an independent encoder's output at a known mask, because
// two correct encoders may otherwise pick different (equally valid) masks.
bool encode_masked(const std::string &s_data, level_e e_level, int i_mask, matrix_o &o_matrix,
                   std::string &s_error);

// The number of payload bytes one version and level holds, or 0 when the
// version is not supported.
int capacity_bytes(int i_version, level_e e_level);

// The level's name, as the format and a report write it: "L", "M", "Q", or "H".
const char *level_name(level_e e_level);

// The matrix as rows of '0' and '1', for a test and for debugging.
std::string to_text(const matrix_o &o_matrix);

// A PNG of the symbol: i_scale pixels per module, a quiet zone of i_quiet
// modules around it, black on white.  Written by hand, because the project has
// no image library and a decoder has to be able to read the result.
std::string to_png(const matrix_o &o_matrix, int i_scale, int i_quiet);

// The symbol as an SVG path that occupies a i_pixel-per-module square whose
// top-left corner is (i_x, i_y); the caller supplies the fill.
std::string to_svg_path(const matrix_o &o_matrix, int i_x, int i_y, int i_pixel);

}  // namespace gnome_pkpass::qr
