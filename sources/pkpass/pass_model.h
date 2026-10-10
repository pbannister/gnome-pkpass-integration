#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace gnome_pkpass::pkpass {

// An sRGB colour as the pass names it.  Apple documents the `rgb(r,g,b)` form
// and real passes write spaces after the commas; open-source readers also
// accept hex, and this parser accepts both.
struct color_o {
    bool b_set = false;
    int i_red = 0;
    int i_green = 0;
    int i_blue = 0;
    int i_alpha = 255;
};

// One field of a pass.  A pass field's value is a string, a number, or an ISO
// date; the reader keeps the text as written, because the sample carries
// pre-formatted dates ("October 07, 2026") rather than a style hint.
struct field_o {
    std::string s_key;
    std::string s_label;
    std::string s_value;
    std::string s_attributed_value;
    std::string s_change_message;
    std::string s_text_alignment;
    std::string s_date_style;
    std::string s_time_style;
    std::string s_currency_code;
    std::string s_number_style;
    int i_row = 0;
    bool b_has_row = false;
};

// One of the five fixed field groups.  The group's name is the pass.json key,
// so a report can say where a field came from.
struct field_group_o {
    std::string s_name;
    std::vector<field_o> o_fields;
};

// One barcode.  A pass may carry the legacy singular `barcode` object, the
// `barcodes` array, or both; the array is preferred, as Wallet uses its first
// displayable entry.
struct barcode_o {
    std::string s_format;
    std::string s_message;
    std::string s_message_encoding;
    std::string s_alt_text;
};

// A relevance location, used by the lock screen.
struct location_o {
    bool b_has_latitude = false;
    bool b_has_longitude = false;
    double d_latitude = 0.0;
    double d_longitude = 0.0;
    std::string s_relevant_text;
};

// One entry of manifest.json: a member path and the SHA-1 it must have.
struct manifest_entry_o {
    std::string s_name;
    std::string s_digest;
};

enum class style_e { none, generic, store_card, coupon, event_ticket, boarding_pass };

// An image member of the container, named as the format names it.
struct image_o {
    std::string s_name;
    std::size_t u_size = 0;
};

struct pass_o {
    // Container facts.
    std::string s_path;
    std::size_t u_file_size = 0;
    std::vector<std::string> o_member_names;
    std::vector<image_o> o_images;

    // The top level of pass.json.
    int i_format_version = 0;
    bool b_has_format_version = false;
    std::string s_pass_type_identifier;
    std::string s_serial_number;
    std::string s_team_identifier;
    std::string s_organization_name;
    std::string s_description;
    std::string s_logo_text;
    std::string s_relevant_date;
    std::string s_expiration_date;
    bool b_voided = false;
    std::string s_grouping_identifier;
    bool b_sharing_prohibited = false;
    std::string s_web_service_url;
    bool b_has_authentication_token = false;
    bool b_has_nfc = false;
    style_e e_style = style_e::none;
    std::string s_transit_type;

    color_o o_background_color;
    color_o o_foreground_color;
    color_o o_label_color;

    std::vector<field_group_o> o_field_groups;
    std::vector<barcode_o> o_barcodes;
    std::vector<location_o> o_locations;

    // Integrity facts.
    bool b_has_pass_json = false;
    bool b_has_manifest = false;
    bool b_has_signature = false;
    std::size_t u_signature_size = 0;
    std::vector<manifest_entry_o> o_manifest;
    std::string s_signature_data;

    // The documents as they were read, so `pkpass-inspect --json` can print a
    // pass back without re-reading it.
    std::string s_pass_json_text;
    std::string s_manifest_json_text;

    // Anything a reader noticed but did not refuse: an unknown format version, a
    // missing icon, more than one style key.  A pass is still shown.
    std::vector<std::string> o_warnings;
};

// The pass.json key for a style, and the short label a report uses.
const char *style_key(style_e e_style);
const char *style_name(style_e e_style);

// The short name of a barcode format: "QR" for `PKBarcodeFormatQR`, and so on.
std::string barcode_format_label(const std::string &s_format);

}  // namespace gnome_pkpass::pkpass
