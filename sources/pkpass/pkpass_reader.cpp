#include "pkpass/pkpass_reader.h"

#include "json/json_reader.h"
#include "util/file_io.h"
#include "zip/zip_reader.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace gnome_pkpass::pkpass {
namespace {

// The field groups, in the order a report and the card renderer present them.
const char *const GROUP_NAMES[] = {"headerFields", "primaryFields", "secondaryFields",
                                   "auxiliaryFields", "backFields"};
constexpr std::size_t GROUP_COUNT = sizeof(GROUP_NAMES) / sizeof(GROUP_NAMES[0]);

std::string trim(const std::string &s_text) {
    std::size_t u_begin = 0;
    std::size_t u_end = s_text.size();
    while (u_begin < u_end && 0 != std::isspace(static_cast<unsigned char>(s_text[u_begin]))) {
        ++u_begin;
    }
    while (u_end > u_begin && 0 != std::isspace(static_cast<unsigned char>(s_text[u_end - 1]))) {
        --u_end;
    }
    return s_text.substr(u_begin, u_end - u_begin);
}

std::string lower(const std::string &s_text) {
    std::string s_lowered = s_text;
    std::transform(s_lowered.begin(), s_lowered.end(), s_lowered.begin(),
                   [](unsigned char c_next) { return static_cast<char>(std::tolower(c_next)); });
    return s_lowered;
}

std::string number_text(double d_value) {
    std::ostringstream s_stream;
    if (-1e15 < d_value && 1e15 > d_value &&
        d_value == static_cast<double>(static_cast<long long>(d_value))) {
        s_stream << static_cast<long long>(d_value);
    } else {
        s_stream << d_value;
    }
    return s_stream.str();
}

// Render a scalar member as text.  A field value may be a string, a number, or a
// boolean, and all three are shown as the pass's own text.
std::string scalar_text(const json::value_c *p_value) {
    if (nullptr == p_value) {
        return std::string();
    }
    switch (p_value->kind()) {
        case json::value_c::kind_e::string:
            return p_value->as_string();
        case json::value_c::kind_e::number:
            return number_text(p_value->as_number());
        case json::value_c::kind_e::boolean:
            return p_value->as_boolean() ? "true" : "false";
        default:
            return std::string();
    }
}

field_o field_from_json(const json::value_c &o_field) {
    field_o o_result;
    o_result.s_key = o_field.string_or("key");
    o_result.s_label = o_field.string_or("label");
    o_result.s_value = scalar_text(o_field.member("value"));
    o_result.s_attributed_value = scalar_text(o_field.member("attributedValue"));
    o_result.s_change_message = o_field.string_or("changeMessage");
    o_result.s_text_alignment = o_field.string_or("textAlignment");
    o_result.s_date_style = o_field.string_or("dateStyle");
    o_result.s_time_style = o_field.string_or("timeStyle");
    o_result.s_currency_code = o_field.string_or("currencyCode");
    o_result.s_number_style = o_field.string_or("numberStyle");
    const json::value_c *p_row = o_field.member("row");
    if (nullptr != p_row && p_row->is_number()) {
        o_result.b_has_row = true;
        o_result.i_row = static_cast<int>(p_row->as_number());
    }
    return o_result;
}

style_e style_from_json(const json::value_c &o_root, std::string &s_found_key) {
    struct candidate_o {
        const char *s_key;
        style_e e_style;
    };
    static const candidate_o o_candidates[] = {
        {"generic", style_e::generic},
        {"storeCard", style_e::store_card},
        {"coupon", style_e::coupon},
        {"eventTicket", style_e::event_ticket},
        {"boardingPass", style_e::boarding_pass},
    };

    style_e e_style = style_e::none;
    for (const candidate_o &o_candidate : o_candidates) {
        if (nullptr == o_root.member(o_candidate.s_key)) {
            continue;
        }
        if (style_e::none == e_style) {
            e_style = o_candidate.e_style;
            s_found_key = o_candidate.s_key;
        }
    }
    return e_style;
}

void collect_barcodes(const json::value_c &o_root, std::vector<barcode_o> &o_barcodes) {
    const std::vector<json::value_c> o_array = o_root.array_or_empty("barcodes");
    for (const json::value_c &o_item : o_array) {
        barcode_o o_barcode;
        o_barcode.s_format = o_item.string_or("format");
        o_barcode.s_message = o_item.string_or("message");
        o_barcode.s_message_encoding = o_item.string_or("messageEncoding");
        o_barcode.s_alt_text = o_item.string_or("altText");
        if (!o_barcode.s_format.empty() || !o_barcode.s_message.empty()) {
            o_barcodes.push_back(o_barcode);
        }
    }

    // The legacy singular object is used only when the array said nothing,
    // because Wallet prefers the array and a pass with both would otherwise show
    // the same barcode twice.
    const json::value_c *p_legacy = o_root.member("barcode");
    if (o_barcodes.empty() && nullptr != p_legacy && p_legacy->is_object()) {
        barcode_o o_barcode;
        o_barcode.s_format = p_legacy->string_or("format");
        o_barcode.s_message = p_legacy->string_or("message");
        o_barcode.s_message_encoding = p_legacy->string_or("messageEncoding");
        o_barcode.s_alt_text = p_legacy->string_or("altText");
        if (!o_barcode.s_format.empty() || !o_barcode.s_message.empty()) {
            o_barcodes.push_back(o_barcode);
        }
    }
}

void collect_locations(const json::value_c &o_root, std::vector<location_o> &o_locations) {
    for (const json::value_c &o_item : o_root.array_or_empty("locations")) {
        location_o o_location;
        const json::value_c *p_latitude = o_item.member("latitude");
        const json::value_c *p_longitude = o_item.member("longitude");
        if (nullptr != p_latitude && p_latitude->is_number()) {
            o_location.b_has_latitude = true;
            o_location.d_latitude = p_latitude->as_number();
        }
        if (nullptr != p_longitude && p_longitude->is_number()) {
            o_location.b_has_longitude = true;
            o_location.d_longitude = p_longitude->as_number();
        }
        o_location.s_relevant_text = o_item.string_or("relevantText");
        o_locations.push_back(o_location);
    }
}

bool decode_variable_int(const std::string &s_text, std::size_t u_at, unsigned long &u_value) {
    u_value = 0;
    std::size_t u_index = u_at;
    bool b_any = false;
    while (u_index < s_text.size() && 10 > u_index - u_at) {
        if (0 == std::isdigit(static_cast<unsigned char>(s_text[u_index]))) {
            break;
        }
        u_value = u_value * 10 + static_cast<unsigned long>(s_text[u_index] - '0');
        ++u_index;
        b_any = true;
    }
    return b_any;
}

int clamp_byte(unsigned long u_value) {
    return (255 < u_value) ? 255 : static_cast<int>(u_value);
}

}  // namespace

const char *style_key(style_e e_style) {
    switch (e_style) {
        case style_e::generic:
            return "generic";
        case style_e::store_card:
            return "storeCard";
        case style_e::coupon:
            return "coupon";
        case style_e::event_ticket:
            return "eventTicket";
        case style_e::boarding_pass:
            return "boardingPass";
        case style_e::none:
        default:
            return "";
    }
}

const char *style_name(style_e e_style) {
    switch (e_style) {
        case style_e::generic:
            return "generic";
        case style_e::store_card:
            return "store card";
        case style_e::coupon:
            return "coupon";
        case style_e::event_ticket:
            return "event ticket";
        case style_e::boarding_pass:
            return "boarding pass";
        case style_e::none:
        default:
            return "none";
    }
}

std::string barcode_format_label(const std::string &s_format) {
    if ("PKBarcodeFormatQR" == s_format) {
        return "QR";
    }
    if ("PKBarcodeFormatPDF417" == s_format) {
        return "PDF417";
    }
    if ("PKBarcodeFormatAztec" == s_format) {
        return "Aztec";
    }
    if ("PKBarcodeFormatCode128" == s_format) {
        return "Code 128";
    }
    if ("PKBarcodeFormatCode39" == s_format) {
        return "Code 39";
    }
    if ("PKBarcodeFormatCodabar" == s_format) {
        return "Codabar";
    }
    if ("PKBarcodeFormatEAN13" == s_format) {
        return "EAN-13";
    }
    if ("PKBarcodeFormatI2of5" == s_format) {
        return "Interleaved 2 of 5";
    }
    return s_format.empty() ? "none" : s_format;
}

bool parse_color(const std::string &s_text, color_o &o_color) {
    const std::string s_clean = trim(lower(s_text));
    if (s_clean.empty()) {
        return false;
    }

    if (0 == s_clean.compare(0, 4, "rgb(") || 0 == s_clean.compare(0, 5, "rgba(")) {
        const bool b_alpha = ('a' == s_clean[3]);
        const std::size_t u_open = s_clean.find('(');
        const std::size_t u_close = s_clean.rfind(')');
        if (std::string::npos == u_open || std::string::npos == u_close || u_close < u_open) {
            return false;
        }
        const std::string s_body = s_clean.substr(u_open + 1, u_close - u_open - 1);
        unsigned long o_parts[4] = {0, 0, 0, 255};
        std::size_t u_at = 0;
        int i_part = 0;
        while (i_part < 4) {
            while (u_at < s_body.size() &&
                   0 != std::isspace(static_cast<unsigned char>(s_body[u_at]))) {
                ++u_at;
            }
            if (u_at >= s_body.size()) {
                break;
            }
            unsigned long u_value = 0;
            if (!decode_variable_int(s_body, u_at, u_value)) {
                return false;
            }
            o_parts[i_part] = u_value;
            ++i_part;
            while (u_at < s_body.size() &&
                   0 != std::isdigit(static_cast<unsigned char>(s_body[u_at]))) {
                ++u_at;
            }
            while (u_at < s_body.size() &&
                   0 != std::isspace(static_cast<unsigned char>(s_body[u_at]))) {
                ++u_at;
            }
            if (u_at < s_body.size() && ',' == s_body[u_at]) {
                ++u_at;
            }
        }
        if (3 > i_part || (b_alpha && 4 > i_part)) {
            return false;
        }
        o_color.b_set = true;
        o_color.i_red = clamp_byte(o_parts[0]);
        o_color.i_green = clamp_byte(o_parts[1]);
        o_color.i_blue = clamp_byte(o_parts[2]);
        o_color.i_alpha = b_alpha ? clamp_byte(o_parts[3]) : 255;
        return true;
    }

    if ('#' == s_clean[0]) {
        const std::string s_digits = s_clean.substr(1);
        if (3 != s_digits.size() && 4 != s_digits.size() && 6 != s_digits.size() &&
            8 != s_digits.size()) {
            return false;
        }
        for (const char c_digit : s_digits) {
            if (0 == std::isxdigit(static_cast<unsigned char>(c_digit))) {
                return false;
            }
        }

        unsigned long o_parts[4] = {0, 0, 0, 255};
        int i_parts = 0;
        if (3 == s_digits.size() || 4 == s_digits.size()) {
            for (int i_digit = 0; i_digit < static_cast<int>(s_digits.size()); ++i_digit) {
                const int i_value = std::isdigit(static_cast<unsigned char>(s_digits[static_cast<std::size_t>(i_digit)]))
                                        ? (s_digits[static_cast<std::size_t>(i_digit)] - '0')
                                        : (s_digits[static_cast<std::size_t>(i_digit)] - 'a' + 10);
                o_parts[i_digit] = static_cast<unsigned long>(i_value * 17);
            }
            i_parts = static_cast<int>(s_digits.size());
        } else {
            const int i_channels = (8 == s_digits.size()) ? 4 : 3;
            for (int i_channel = 0; i_channel < i_channels; ++i_channel) {
                const std::string s_pair = s_digits.substr(static_cast<std::size_t>(i_channel) * 2, 2);
                o_parts[i_channel] = std::strtoul(s_pair.c_str(), nullptr, 16);
            }
            i_parts = i_channels;
        }
        // `#aarrggbb` is what the open-source readers accept, so alpha leads
        // when four channels are given.
        o_color.b_set = true;
        if (4 == i_parts) {
            o_color.i_alpha = clamp_byte(o_parts[0]);
            o_color.i_red = clamp_byte(o_parts[1]);
            o_color.i_green = clamp_byte(o_parts[2]);
            o_color.i_blue = clamp_byte(o_parts[3]);
        } else {
            o_color.i_red = clamp_byte(o_parts[0]);
            o_color.i_green = clamp_byte(o_parts[1]);
            o_color.i_blue = clamp_byte(o_parts[2]);
            o_color.i_alpha = 255;
        }
        return true;
    }

    return false;
}

bool parse_barcode_json(const std::string &s_json, barcode_o &o_barcode, std::string &s_error) {
    json::value_c o_value;
    if (!json::value_c::parse(s_json, o_value, s_error)) {
        return false;
    }
    if (!o_value.is_object()) {
        s_error = "a barcode must be a JSON object";
        return false;
    }
    o_barcode.s_format = o_value.string_or("format");
    o_barcode.s_message = o_value.string_or("message");
    o_barcode.s_message_encoding = o_value.string_or("messageEncoding");
    o_barcode.s_alt_text = o_value.string_or("altText");
    return true;
}

bool read_pass(const std::string &s_path, pass_o &o_pass, std::string &s_error) {
    std::string s_data;
    if (!util::read_file(s_path, s_data, s_error)) {
        return false;
    }
    if (!read_pass_bytes(s_data, o_pass, s_error)) {
        return false;
    }
    o_pass.s_path = s_path;
    return true;
}

bool read_pass_bytes(const std::string &s_data, pass_o &o_pass, std::string &s_error) {
    o_pass = pass_o();
    o_pass.u_file_size = s_data.size();

    zip::archive_c o_archive;
    if (!o_archive.load_bytes(s_data, s_error)) {
        return false;
    }
    for (const zip::entry_o &o_entry : o_archive.entries()) {
        o_pass.o_member_names.push_back(o_entry.s_name);
        if (4 < o_entry.s_name.size() &&
            0 == lower(o_entry.s_name).compare(o_entry.s_name.size() - 4, 4, ".png")) {
            image_o o_image;
            o_image.s_name = o_entry.s_name;
            o_image.u_size = static_cast<std::size_t>(o_entry.u_size_uncompressed);
            o_pass.o_images.push_back(o_image);
        }
    }

    std::string s_pass_json;
    if (!o_archive.extract("pass.json", s_pass_json, s_error)) {
        // `pass.json` may live in a subdirectory of a wrongly built archive; the
        // format requires it at the root, so say so rather than guessing.
        s_error = "the archive has no pass.json at its root";
        return false;
    }
    o_pass.b_has_pass_json = true;
    o_pass.s_pass_json_text = s_pass_json;

    json::value_c o_root;
    if (!json::value_c::parse(s_pass_json, o_root, s_error)) {
        s_error = "pass.json: " + s_error;
        return false;
    }
    if (!o_root.is_object()) {
        s_error = "pass.json is not a JSON object";
        return false;
    }

    const json::value_c *p_format = o_root.member("formatVersion");
    if (nullptr != p_format && p_format->is_number()) {
        o_pass.b_has_format_version = true;
        o_pass.i_format_version = static_cast<int>(p_format->as_number());
    } else {
        o_pass.o_warnings.emplace_back("pass.json has no numeric formatVersion");
    }
    if (o_pass.b_has_format_version && 1 != o_pass.i_format_version) {
        o_pass.o_warnings.emplace_back("formatVersion is " +
                                       number_text(static_cast<double>(o_pass.i_format_version)) +
                                       "; the format defines version 1");
    }

    o_pass.s_pass_type_identifier = o_root.string_or("passTypeIdentifier");
    o_pass.s_serial_number = o_root.string_or("serialNumber");
    o_pass.s_team_identifier = o_root.string_or("teamIdentifier");
    o_pass.s_organization_name = o_root.string_or("organizationName");
    o_pass.s_description = o_root.string_or("description");
    o_pass.s_logo_text = o_root.string_or("logoText");
    o_pass.s_relevant_date = o_root.string_or("relevantDate");
    o_pass.s_expiration_date = o_root.string_or("expirationDate");
    o_pass.b_voided = o_root.boolean_or("voided", false);
    o_pass.s_grouping_identifier = o_root.string_or("groupingIdentifier");
    o_pass.b_sharing_prohibited = o_root.boolean_or("sharingProhibited", false);
    o_pass.s_web_service_url = o_root.string_or("webServiceURL");
    o_pass.b_has_authentication_token = nullptr != o_root.member("authenticationToken");
    o_pass.b_has_nfc = nullptr != o_root.member("nfc");

    for (const char *s_required : {"passTypeIdentifier", "serialNumber", "teamIdentifier"}) {
        if (nullptr == o_root.member(s_required)) {
            o_pass.o_warnings.emplace_back(std::string("pass.json has no ") + s_required);
        }
    }

    std::string s_style_key;
    o_pass.e_style = style_from_json(o_root, s_style_key);
    if (style_e::none == o_pass.e_style) {
        o_pass.o_warnings.emplace_back("pass.json names no pass style");
    } else {
        const json::value_c *p_style = o_root.member(s_style_key);
        if (nullptr != p_style && p_style->is_object()) {
            o_pass.s_transit_type = p_style->string_or("transitType");
            for (std::size_t u_group = 0; u_group < GROUP_COUNT; ++u_group) {
                field_group_o o_group;
                o_group.s_name = GROUP_NAMES[u_group];
                for (const json::value_c &o_field : p_style->array_or_empty(GROUP_NAMES[u_group])) {
                    o_group.o_fields.push_back(field_from_json(o_field));
                }
                o_pass.o_field_groups.push_back(o_group);
            }
        } else {
            o_pass.o_warnings.emplace_back(std::string("the ") + s_style_key +
                                           " style is not an object");
        }
    }

    collect_barcodes(o_root, o_pass.o_barcodes);
    collect_locations(o_root, o_pass.o_locations);

    // A colour the pass names but this parser cannot read is worth a warning:
    // it is the difference between a purple pass and a black one.
    struct color_slot_o {
        const char *s_key;
        color_o *p_color;
    };
    const color_slot_o o_color_slots[] = {
        {"backgroundColor", &o_pass.o_background_color},
        {"foregroundColor", &o_pass.o_foreground_color},
        {"labelColor", &o_pass.o_label_color},
    };
    for (const color_slot_o &o_slot : o_color_slots) {
        const json::value_c *p_color = o_root.member(o_slot.s_key);
        if (nullptr == p_color || !p_color->is_string()) {
            continue;
        }
        if (!parse_color(p_color->as_string(), *o_slot.p_color)) {
            o_pass.o_warnings.emplace_back(std::string("cannot read the colour ") + o_slot.s_key +
                                           ": " + p_color->as_string());
        }
    }

    const zip::entry_o *p_manifest = o_archive.find("manifest.json");
    if (nullptr == p_manifest) {
        o_pass.o_warnings.emplace_back("the archive has no manifest.json");
    } else {
        std::string s_manifest;
        if (!o_archive.extract(*p_manifest, s_manifest, s_error)) {
            o_pass.o_warnings.emplace_back("cannot read manifest.json: " + s_error);
        } else {
            o_pass.b_has_manifest = true;
            o_pass.s_manifest_json_text = s_manifest;
            json::value_c o_manifest;
            std::string s_manifest_error;
            if (!json::value_c::parse(s_manifest, o_manifest, s_manifest_error)) {
                o_pass.o_warnings.emplace_back("manifest.json: " + s_manifest_error);
            } else if (!o_manifest.is_object()) {
                o_pass.o_warnings.emplace_back("manifest.json is not a JSON object");
            } else {
                for (const std::pair<std::string, json::value_c> &o_member : o_manifest.members()) {
                    manifest_entry_o o_entry;
                    o_entry.s_name = o_member.first;
                    o_entry.s_digest = scalar_text(&o_member.second);
                    o_pass.o_manifest.push_back(o_entry);
                }
            }
        }
    }

    const zip::entry_o *p_signature = o_archive.find("signature");
    if (nullptr == p_signature) {
        o_pass.o_warnings.emplace_back("the archive has no signature");
    } else {
        o_pass.b_has_signature = true;
        o_pass.u_signature_size = static_cast<std::size_t>(p_signature->u_size_uncompressed);
        std::string s_signature_error;
        if (!o_archive.extract(*p_signature, o_pass.s_signature_data, s_signature_error)) {
            o_pass.o_warnings.emplace_back("cannot read the signature: " + s_signature_error);
        }
    }

    return true;
}

}  // namespace gnome_pkpass::pkpass
