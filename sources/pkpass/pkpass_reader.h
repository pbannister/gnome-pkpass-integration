#pragma once

#include <string>

#include "pkpass/pass_model.h"

namespace gnome_pkpass::pkpass {

// Read one `.pkpass`: a ZIP whose `pass.json` describes a Wallet pass, with
// `manifest.json` and `signature` alongside it.
//
// A pass that is structurally readable is returned even with warnings, because a
// viewer is more useful than a refusal; only a container that is not a readable
// ZIP, or one with no parseable `pass.json`, fails.  The returned pass records
// what it found so a caller can verify or report it.
bool read_pass(const std::string &s_path, pass_o &o_pass, std::string &s_error);

// The same, from bytes already in memory; used by the tests.
bool read_pass_bytes(const std::string &s_data, pass_o &o_pass, std::string &s_error);

// Parse an `rgb(r,g,b)`, `rgba(r,g,b,a)`, `#rrggbb`, or `#aarrggbb` colour.
// Apple documents only the first form, and real passes include spaces after the
// commas; the hex forms are what the open-source readers accept, so both are
// taken and anything else is refused rather than guessed.
bool parse_color(const std::string &s_text, color_o &o_color);

// Parse the `Pass.Barcode` dictionary shape into the model.  Kept public so a
// test can drive it directly.
bool parse_barcode_json(const std::string &s_json, barcode_o &o_barcode, std::string &s_error);

}  // namespace gnome_pkpass::pkpass
