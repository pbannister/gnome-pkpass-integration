//
// pkpass_reader_test.cpp: the pass reader, the colour parser, and the barcode
// parser.
//
// Usage: pkpass-reader-test FILE.pkpass
//
#include "pkpass/pkpass_reader.h"

#include "test_harness.h"

#include <cstdio>

namespace {

using gnome_pkpass::pkpass::color_o;
using gnome_pkpass::pkpass::field_group_o;
using gnome_pkpass::pkpass::pass_o;

const field_group_o *group_named(const pass_o &o_pass, const char *s_name) {
    for (const field_group_o &o_group : o_pass.o_field_groups) {
        if (o_group.s_name == s_name) {
            return &o_group;
        }
    }
    return nullptr;
}

void check_colors() {
    using gnome_pkpass::pkpass::parse_color;

    color_o o_color;
    // Apple's own examples and the real passes put spaces after the commas.
    EXPECT_TRUE(parse_color("rgb(86, 39, 126)", o_color));
    EXPECT_EQ(86, o_color.i_red);
    EXPECT_EQ(39, o_color.i_green);
    EXPECT_EQ(126, o_color.i_blue);
    EXPECT_EQ(255, o_color.i_alpha);

    EXPECT_TRUE(parse_color("rgb(1,2,3)", o_color));
    EXPECT_EQ(3, o_color.i_blue);

    EXPECT_TRUE(parse_color("RGBA(1, 2, 3, 128)", o_color));
    EXPECT_EQ(128, o_color.i_alpha);

    EXPECT_TRUE(parse_color("#1a2b3c", o_color));
    EXPECT_EQ(26, o_color.i_red);
    EXPECT_EQ(43, o_color.i_green);
    EXPECT_EQ(60, o_color.i_blue);

    // Four channels lead with alpha, which is what the open-source readers do.
    EXPECT_TRUE(parse_color("#801a2b3c", o_color));
    EXPECT_EQ(128, o_color.i_alpha);
    EXPECT_EQ(26, o_color.i_red);

    EXPECT_TRUE(parse_color("#abc", o_color));
    EXPECT_EQ(170, o_color.i_red);

    EXPECT_FALSE(parse_color("purple", o_color));
    EXPECT_FALSE(parse_color("rgb(1,2)", o_color));
    EXPECT_FALSE(parse_color("", o_color));
    EXPECT_FALSE(parse_color("#12345", o_color));
}

}  // namespace

int main(int i_argument_count, char *o_arguments[]) {
    using gnome_pkpass::pkpass::barcode_o;
    using gnome_pkpass::pkpass::barcode_format_label;
    using gnome_pkpass::pkpass::parse_barcode_json;
    using gnome_pkpass::pkpass::read_pass;
    using gnome_pkpass::pkpass::read_pass_bytes;
    using gnome_pkpass::pkpass::style_e;

    check_colors();

    if (2 > i_argument_count) {
        std::fprintf(stderr, "usage: pkpass-reader-test FILE.pkpass\n");
        return 2;
    }
    pass_o o_pass;
    std::string s_error;
    EXPECT_TRUE(read_pass(o_arguments[1], o_pass, s_error));

    EXPECT_EQ(std::string("pass.com.example.walletpass"), o_pass.s_pass_type_identifier);
    EXPECT_EQ(std::string("0000000001"), o_pass.s_serial_number);
    EXPECT_EQ(std::string("EXAMPLETEAM"), o_pass.s_team_identifier);
    EXPECT_EQ(std::string("Example Organization"), o_pass.s_organization_name);
    EXPECT_EQ(std::string("Example Wallet Pass"), o_pass.s_description);
    EXPECT_TRUE(o_pass.b_has_format_version);
    EXPECT_EQ(1, o_pass.i_format_version);
    EXPECT_TRUE(style_e::generic == o_pass.e_style);
    EXPECT_EQ(std::string("generic"), std::string(gnome_pkpass::pkpass::style_key(o_pass.e_style)));
    EXPECT_FALSE(o_pass.b_has_nfc);
    EXPECT_FALSE(o_pass.b_voided);

    EXPECT_TRUE(o_pass.o_background_color.b_set);
    EXPECT_EQ(12, o_pass.o_background_color.i_red);
    EXPECT_TRUE(o_pass.o_label_color.b_set);
    EXPECT_EQ(200, o_pass.o_label_color.i_red);
    EXPECT_EQ(180, o_pass.o_label_color.i_green);
    EXPECT_EQ(120, o_pass.o_label_color.i_blue);

    // The fixture uses the legacy singular `barcode`, which Wallet still reads.
    EXPECT_EQ(static_cast<std::size_t>(1), o_pass.o_barcodes.size());
    if (!o_pass.o_barcodes.empty()) {
        EXPECT_EQ(std::string("PKBarcodeFormatQR"), o_pass.o_barcodes[0].s_format);
        EXPECT_EQ(std::string("QR"), barcode_format_label(o_pass.o_barcodes[0].s_format));
        EXPECT_EQ(std::string("utf-8"), o_pass.o_barcodes[0].s_message_encoding);
        EXPECT_EQ(std::string("ID: 0000000001"), o_pass.o_barcodes[0].s_alt_text);
        EXPECT_CONTAINS(o_pass.o_barcodes[0].s_message, "ticket");
    }

    EXPECT_EQ(static_cast<std::size_t>(5), o_pass.o_field_groups.size());
    const field_group_o *p_primary = group_named(o_pass, "primaryFields");
    const field_group_o *p_back = group_named(o_pass, "backFields");
    EXPECT_TRUE(nullptr != p_primary);
    EXPECT_TRUE(nullptr != p_back);
    if (nullptr != p_primary && nullptr != p_back) {
        EXPECT_EQ(static_cast<std::size_t>(1), p_primary->o_fields.size());
        EXPECT_EQ(static_cast<std::size_t>(2), p_back->o_fields.size());
        EXPECT_CONTAINS(p_primary->o_fields[0].s_value, "Ada");
        if (2 == p_back->o_fields.size()) {
            EXPECT_EQ(std::string("Instructions"), p_back->o_fields[0].s_label);
        }
    }

    EXPECT_EQ(static_cast<std::size_t>(2), o_pass.o_images.size());
    EXPECT_EQ(static_cast<std::size_t>(3), o_pass.o_manifest.size());
    EXPECT_TRUE(o_pass.b_has_manifest);
    EXPECT_TRUE(o_pass.b_has_pass_json);
    EXPECT_FALSE(o_pass.b_has_signature);

    EXPECT_EQ(static_cast<std::size_t>(1), o_pass.o_locations.size());
    if (!o_pass.o_locations.empty()) {
        EXPECT_TRUE(o_pass.o_locations[0].b_has_latitude);
        EXPECT_EQ(std::string("Event nearby"), o_pass.o_locations[0].s_relevant_text);
    }

    // A container that is not a ZIP, and one with no pass.json, both fail with a
    // reason rather than returning a half-filled pass.
    pass_o o_bad;
    std::string s_bad_error;
    EXPECT_FALSE(read_pass_bytes("not a zip", o_bad, s_bad_error));
    EXPECT_FALSE(s_bad_error.empty());

    barcode_o o_barcode;
    std::string s_barcode_error;
    EXPECT_TRUE(parse_barcode_json(
        "{\"format\":\"PKBarcodeFormatPDF417\",\"message\":\"m\",\"messageEncoding\":\"iso-8859-1\"}",
        o_barcode, s_barcode_error));
    EXPECT_EQ(std::string("PDF417"), barcode_format_label(o_barcode.s_format));
    EXPECT_FALSE(parse_barcode_json("[1,2]", o_barcode, s_barcode_error));
    EXPECT_FALSE(parse_barcode_json("{", o_barcode, s_barcode_error));

    // A label for a format this project does not know is shown as the raw name
    // rather than guessed at.
    EXPECT_EQ(std::string("PKBarcodeFormatFuture"), barcode_format_label("PKBarcodeFormatFuture"));
    EXPECT_EQ(std::string("none"), barcode_format_label(""));

    return pkpass_test::finish("pkpass-reader-test");
}
