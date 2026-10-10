//
// pass_render_test.cpp: the SVG card.
//
// Usage: pass-render-test FILE.pkpass
//
#include "pkpass/pkpass_reader.h"
#include "render/pass_render.h"

#include "test_harness.h"

#include <cstdio>

int main(int i_argument_count, char *o_arguments[]) {
    using gnome_pkpass::pkpass::field_o;
    using gnome_pkpass::pkpass::field_group_o;
    using gnome_pkpass::pkpass::pass_o;
    using gnome_pkpass::render::options_o;
    using gnome_pkpass::render::render_svg;
    using gnome_pkpass::render::render_svg_without_images;

    if (2 > i_argument_count) {
        std::fprintf(stderr, "usage: pass-render-test FILE.pkpass\n");
        return 2;
    }

    pass_o o_pass;
    std::string s_error;
    EXPECT_TRUE(gnome_pkpass::pkpass::read_pass(o_arguments[1], o_pass, s_error));

    options_o o_front;
    const std::string s_front = render_svg_without_images(o_pass, o_front);
    EXPECT_CONTAINS(s_front, "<svg");
    EXPECT_CONTAINS(s_front, "</svg>");
    EXPECT_TRUE(s_front.find("<svg") < s_front.find("</svg>"));
    EXPECT_CONTAINS(s_front, "rgb(12,34,56)");
    EXPECT_CONTAINS(s_front, "rgb(240,240,240)");
    EXPECT_CONTAINS(s_front, "rgb(200,180,120)");
    EXPECT_CONTAINS(s_front, "Ada");
    EXPECT_CONTAINS(s_front, "EVENT DATE");
    EXPECT_CONTAINS(s_front, "QR barcode (not drawn)");
    EXPECT_CONTAINS(s_front, "ID: 0000000001");
    EXPECT_CONTAINS(s_front, "ticket");
    // The front does not carry the reverse.
    EXPECT_NOT_CONTAINS(s_front, "Instructions");

    options_o o_back;
    o_back.b_render_back = true;
    const std::string s_back = render_svg_without_images(o_pass, o_back);
    EXPECT_CONTAINS(s_back, "Back of pass");
    EXPECT_CONTAINS(s_back, "Instructions");
    EXPECT_CONTAINS(s_back, "Show this pass at the entrance.");

    // With the container, the logo is embedded as a data URI so one SVG file
    // stands alone.
    options_o o_embedded;
    const std::string s_with_images = render_svg(o_arguments[1], o_pass, o_embedded, s_error);
    EXPECT_CONTAINS(s_with_images, "data:image/png;base64,");
    EXPECT_CONTAINS(s_with_images, "<image ");

    // An embedded image is dropped on request.
    options_o o_no_images;
    o_no_images.b_embed_images = false;
    const std::string s_plain = render_svg(o_arguments[1], o_pass, o_no_images, s_error);
    EXPECT_NOT_CONTAINS(s_plain, "data:image/png;base64,");

    // A card with no container still renders: the renderer is not allowed to
    // fail because a file moved.
    const std::string s_no_file =
        render_svg("/nonexistent/pass.pkpass", o_pass, o_embedded, s_error);
    EXPECT_CONTAINS(s_no_file, "<svg");
    EXPECT_FALSE(s_error.empty());

    // Text that could close an element is escaped, so a hostile pass cannot
    // inject markup into the SVG.
    pass_o o_hostile;
    o_hostile.e_style = gnome_pkpass::pkpass::style_e::generic;
    o_hostile.s_logo_text = "Logo & <text>";
    field_group_o o_group;
    o_group.s_name = "primaryFields";
    field_o o_field;
    o_field.s_key = "k";
    o_field.s_label = "L \"quoted\"";
    o_field.s_value = "</text><script>alert(1)</script>";
    o_group.o_fields.push_back(o_field);
    o_hostile.o_field_groups.push_back(o_group);

    const std::string s_hostile = render_svg_without_images(o_hostile, o_front);
    EXPECT_NOT_CONTAINS(s_hostile, "<script>");
    EXPECT_CONTAINS(s_hostile, "&lt;script&gt;");
    EXPECT_CONTAINS(s_hostile, "Logo &amp; &lt;text&gt;");
    EXPECT_CONTAINS(s_hostile, "&quot;quoted&quot;");

    return pkpass_test::finish("pass-render-test");
}
