#pragma once

#include <string>
#include <vector>

#include "pkpass/pass_model.h"
#include "qr/qr_encoder.h"

namespace gnome_pkpass::render {

struct options_o {
    // Render the reverse of the pass, which is where the back fields live.
    bool b_render_back = false;
    // Embed the pass's own images as data URIs, so one SVG file stands alone.
    bool b_embed_images = true;
    // Draw the barcode as a scannable QR symbol.  When the payload is too long
    // for a version this project encodes, the card says so instead.
    bool b_draw_barcode = true;
};

// One run of display text: literal text, or text that is a link.
struct text_run_o {
    std::string s_text;
    std::string s_url;  // empty for literal text
};

// Split text into literal runs and link runs.  Only `http://` and `https://`
// become links, so a pass cannot smuggle another scheme (`javascript:`,
// `data:`) into the card.
//
// With `b_markup`, the text is treated as the HTML that `attributedValue` may
// carry: an inline `<a href="...">text</a>` is reduced to its text and URL and
// any other tag is dropped.  A plain `value` is literal text and is passed
// through untouched, so a `<` in it stays a `<`.
std::vector<text_run_o> linkify(const std::string &s_text, bool b_markup = false);

// The barcode message as it should be shown.  A message that is a JSON object
// or array is re-indented; anything else is returned unchanged.  This affects
// only what a person reads: the symbol always encodes the original bytes.
std::string display_message(const std::string &s_message);

// The same text as Pango markup for a GTK label, with links as `<a href>`.
// Used by the graphical viewer so it can offer the same clickable links.
std::string viewer_markup(const std::string &s_text, bool b_markup = false);

// The QR symbol for one barcode, at the level this project encodes.
bool encode_barcode(const pkpass::barcode_o &o_barcode, qr::level_e e_level, qr::matrix_o &o_matrix,
                    std::string &s_error);

// Render one pass as an SVG card.
//
// The barcode is drawn as a real QR symbol, encoded from the message bytes; the
// payload itself is shown below it, re-indented when it is JSON, with any URL
// clickable.
//
// The path is needed for the pass's images; when it cannot be read the card is
// still rendered without them.
std::string render_svg(const std::string &s_path, const pkpass::pass_o &o_pass,
                       const options_o &o_options, std::string &s_error);

// The same card built from a pass already in memory, with no image bytes
// available; used by the tests to check layout without a container.
std::string render_svg_without_images(const pkpass::pass_o &o_pass, const options_o &o_options);

}  // namespace gnome_pkpass::render
