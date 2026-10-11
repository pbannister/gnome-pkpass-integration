//
// pass_render_test.cpp: the SVG card, its QR symbol, its link runs, and the
// payload formatting.
//
// Usage: pass-render-test FILE.pkpass
//
#include "pkpass/pkpass_reader.h"
#include "render/pass_render.h"

#include "test_harness.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {

using gnome_pkpass::render::display_message;
using gnome_pkpass::render::linkify;
using gnome_pkpass::render::text_run_o;
using gnome_pkpass::render::viewer_markup;

std::size_t count_occurrences(const std::string &s_haystack, const std::string &s_needle) {
    std::size_t u_count = 0;
    std::size_t u_at = 0;
    while ((u_at = s_haystack.find(s_needle, u_at)) != std::string::npos) {
        ++u_count;
        u_at += s_needle.size();
    }
    return u_count;
}

void check_linkify() {
    // A plain http or https URL becomes one link run.
    const std::vector<text_run_o> o_runs = linkify("See https://example.invalid/a for more.");
    EXPECT_EQ(static_cast<std::size_t>(3), o_runs.size());
    if (3 == o_runs.size()) {
        EXPECT_EQ(std::string("See "), o_runs[0].s_text);
        EXPECT_EQ(std::string(), o_runs[0].s_url);
        EXPECT_EQ(std::string("https://example.invalid/a"), o_runs[1].s_text);
        EXPECT_EQ(std::string("https://example.invalid/a"), o_runs[1].s_url);
        EXPECT_EQ(std::string(" for more."), o_runs[2].s_text);
    }

    // Sentence punctuation after a URL is not part of the link; it stays in the
    // text that follows it.
    const std::vector<text_run_o> o_sentence = linkify("Go to http://example.invalid/x.");
    EXPECT_EQ(static_cast<std::size_t>(3), o_sentence.size());
    if (3 == o_sentence.size()) {
        EXPECT_EQ(std::string("http://example.invalid/x"), o_sentence[1].s_url);
        EXPECT_EQ(std::string("Go to "), o_sentence[0].s_text);
        EXPECT_EQ(std::string("."), o_sentence[2].s_text);
        EXPECT_EQ(std::string(), o_sentence[2].s_url);
    }

    // A plain value is literal text: a tag in it is not stripped, because only
    // `attributedValue` is HTML.  Escaping is the renderer's job, not the
    // splitter's.
    const std::vector<text_run_o> o_literal = linkify("a <b> bold </b> claim");
    EXPECT_EQ(std::string("a <b> bold </b> claim"), o_literal[0].s_text);
    const std::vector<text_run_o> o_markup = linkify("a <b> bold </b> claim", true);
    std::string s_markup_text;
    for (const text_run_o &o_run : o_markup) {
        s_markup_text += o_run.s_text;
    }
    EXPECT_NOT_CONTAINS(s_markup_text, "<b>");
    EXPECT_NOT_CONTAINS(s_markup_text, "</b>");
    EXPECT_CONTAINS(s_markup_text, "bold");

    // Another scheme is not made clickable, so a pass cannot smuggle one in.
    for (const char *s_bad : {"javascript:alert(1)", "data:text/html,x", "file:///etc"}) {
        for (const text_run_o &o_run : linkify(s_bad)) {
            EXPECT_EQ(std::string(), o_run.s_url);
        }
    }

    // An inline anchor, which `attributedValue` may carry, becomes its text and
    // its URL; other tags are dropped rather than shown as markup.
    const std::vector<text_run_o> o_anchor =
        linkify("Open <a href=\"https://example.invalid/y\">the page</a> now", true);
    bool b_found = false;
    for (const text_run_o &o_run : o_anchor) {
        if ("https://example.invalid/y" == o_run.s_url) {
            b_found = true;
            EXPECT_EQ(std::string("the page"), o_run.s_text);
        }
    }
    EXPECT_TRUE(b_found);
    for (const text_run_o &o_run : o_anchor) {
        EXPECT_TRUE(std::string::npos == o_run.s_text.find("<a "));
    }
}

void check_display_message() {
    // A JSON object is re-indented, one member per line.
    const std::string s_pretty = display_message("{\"a\":1,\"b\":\"two\"}");
    EXPECT_CONTAINS(s_pretty, "{\n");
    EXPECT_CONTAINS(s_pretty, "\n  \"a\": 1,\n");
    EXPECT_CONTAINS(s_pretty, "\n  \"b\": \"two\"\n");
    EXPECT_CONTAINS(s_pretty, "\n}");

    // An array is re-indented too.
    EXPECT_CONTAINS(display_message("[1,2]"), "\n  1,");

    // Anything else is left exactly as it was.
    EXPECT_EQ(std::string("not json"), display_message("not json"));
    EXPECT_EQ(std::string("\"just a string\""), display_message("\"just a string\""));
    EXPECT_EQ(std::string("42"), display_message("42"));
    EXPECT_EQ(std::string("{\"broken\""), display_message("{\"broken\""));
}

void check_viewer_markup() {
    // The link is markup, and the text around it is escaped so the label is not
    // asked to parse arbitrary markup from a pass.
    const std::string s_markup = viewer_markup("a < b & c https://example.invalid/z");
    EXPECT_CONTAINS(s_markup, "a &lt; b &amp; c ");
    EXPECT_CONTAINS(s_markup,
                    "<a href=\"https://example.invalid/z\">https://example.invalid/z</a>");
    EXPECT_TRUE(std::string::npos == s_markup.find("<a href=\"javascript"));
    EXPECT_CONTAINS(viewer_markup("javascript:alert(1)"), "javascript:alert(1)");
}

}  // namespace

int main(int i_argument_count, char *o_arguments[]) {
    using gnome_pkpass::pkpass::field_o;
    using gnome_pkpass::pkpass::field_group_o;
    using gnome_pkpass::pkpass::pass_o;
    using gnome_pkpass::render::options_o;
    using gnome_pkpass::render::render_svg;
    using gnome_pkpass::render::render_svg_without_images;

    check_linkify();
    check_display_message();
    check_viewer_markup();

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

    // The barcode is a real symbol now: a dark path inside the white tile, and
    // no "not drawn" placeholder.
    EXPECT_CONTAINS(s_front, "<path fill=\"#000000\" d=\"M");
    EXPECT_NOT_CONTAINS(s_front, "not drawn");
    EXPECT_NOT_CONTAINS(s_front, "could not be encoded");

    // The payload is shown re-indented when it is JSON, with the alt text above
    // it, and the JSON quotes survive as entities in the SVG text.
    EXPECT_CONTAINS(s_front, "ID: 0000000001");
    EXPECT_CONTAINS(s_front, "&quot;ticket&quot;:");
    EXPECT_CONTAINS(s_front, "&quot;0000000001&quot;");
    EXPECT_CONTAINS(s_front, "&quot;seat&quot;:");
    EXPECT_TRUE(2 <= count_occurrences(s_front, "&quot;"));

    // A field value that carries a newline is drawn as two lines.
    EXPECT_CONTAINS(s_front, "Ada");
    EXPECT_CONTAINS(s_front, "EXAMPLE");
    EXPECT_CONTAINS(s_front, "SCHOOL");

    // The front does not carry the reverse, and this fixture's front values hold
    // no link.
    EXPECT_NOT_CONTAINS(s_front, "Instructions");
    EXPECT_NOT_CONTAINS(s_front, "<a xlink:href=");

    // The reverse carries the URL as a clickable link.
    options_o o_back;
    o_back.b_render_back = true;
    const std::string s_back = render_svg_without_images(o_pass, o_back);
    EXPECT_CONTAINS(s_back, "Back of pass");
    EXPECT_CONTAINS(s_back, "Instructions");
    EXPECT_CONTAINS(s_back, "Show this pass at the entrance.");
    EXPECT_CONTAINS(s_back, "<a xlink:href=\"https://example.invalid/profile/1\"");
    EXPECT_CONTAINS(s_back, "href=\"https://example.invalid/profile/1\"");
    EXPECT_CONTAINS(s_back, "text-decoration=\"underline\"");

    // The symbol can be turned off, and then the card says why it is absent.
    options_o o_no_symbol;
    o_no_symbol.b_draw_barcode = false;
    const std::string s_no_symbol = render_svg_without_images(o_pass, o_no_symbol);
    EXPECT_NOT_CONTAINS(s_no_symbol, "<path fill=\"#000000\" d=\"M");
    EXPECT_CONTAINS(s_no_symbol, "could not be encoded");

    // With the container, the logo is embedded as a data URI.
    options_o o_embedded;
    const std::string s_with_images = render_svg(o_arguments[1], o_pass, o_embedded, s_error);
    EXPECT_CONTAINS(s_with_images, "data:image/png;base64,");
    EXPECT_CONTAINS(s_with_images, "<image ");

    options_o o_no_images;
    o_no_images.b_embed_images = false;
    const std::string s_plain = render_svg(o_arguments[1], o_pass, o_no_images, s_error);
    EXPECT_NOT_CONTAINS(s_plain, "data:image/png;base64,");

    // A card with no container still renders.
    const std::string s_no_file =
        render_svg("/nonexistent/pass.pkpass", o_pass, o_embedded, s_error);
    EXPECT_CONTAINS(s_no_file, "<svg");
    EXPECT_FALSE(s_error.empty());

    // Text that could close an element is escaped, in fields and in links.
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
    field_o o_link_field;
    o_link_field.s_key = "l";
    o_link_field.s_label = "Link";
    o_link_field.s_value = "javascript:alert(2)";
    o_group.o_fields.push_back(o_link_field);
    o_hostile.o_field_groups.push_back(o_group);

    const std::string s_hostile = render_svg_without_images(o_hostile, o_front);
    // The angle brackets are entities, so no raw element can appear; the text
    // itself survives.  It may be wrapped mid-token, which is why the check is
    // on a fragment rather than the whole tag.
    EXPECT_TRUE(std::string::npos == s_hostile.find("<script"));
    EXPECT_CONTAINS(s_hostile, "alert(1)");
    EXPECT_CONTAINS(s_hostile, "&lt;");
    EXPECT_CONTAINS(s_hostile, "&gt;");
    EXPECT_CONTAINS(s_hostile, "Logo &amp; &lt;text&gt;");
    EXPECT_CONTAINS(s_hostile, "&quot;quoted&quot;");
    EXPECT_NOT_CONTAINS(s_hostile, "<a xlink:href=\"javascript");

    return pkpass_test::finish("pass-render-test");
}
