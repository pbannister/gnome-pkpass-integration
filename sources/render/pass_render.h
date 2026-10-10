#pragma once

#include <string>

#include "pkpass/pass_model.h"

namespace gnome_pkpass::render {

struct options_o {
    // Render the reverse of the pass, which is where the back fields live.
    bool b_render_back = false;
    // Embed the pass's own images as data URIs, so one SVG file stands alone.
    bool b_embed_images = true;
};

// Render one pass as an SVG card.
//
// The barcode is shown as its payload and the format that carries it, not as a
// scannable symbol: this project does not yet encode a QR or PDF417 bitmap, and
// a symbol that does not scan would be worse than a payload a reader can copy.
// The limitation is recorded in `documents/08-pkpass-format.md` and `TODO.md`.
//
// The path is needed for the pass's images; when it cannot be read the card is
// still rendered without them.
std::string render_svg(const std::string &s_path, const pkpass::pass_o &o_pass,
                       const options_o &o_options, std::string &s_error);

// The same card built from a pass already in memory, with no image bytes
// available; used by the tests to check layout without a container.
std::string render_svg_without_images(const pkpass::pass_o &o_pass, const options_o &o_options);

}  // namespace gnome_pkpass::render
