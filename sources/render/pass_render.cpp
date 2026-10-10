#include "render/pass_render.h"

#include "util/file_io.h"
#include "zip/zip_reader.h"

#include <sstream>
#include <vector>

namespace gnome_pkpass::render {
namespace {

constexpr int CARD_WIDTH = 420;
constexpr int MARGIN = 20;
constexpr int CONTENT_WIDTH = CARD_WIDTH - 2 * MARGIN;
constexpr int LOGO_MAX_WIDTH = 220;
constexpr int LOGO_MAX_HEIGHT = 64;
constexpr int BARCODE_HEIGHT = 132;
constexpr int CHARACTER_WIDTH_TENTHS = 6;

struct palette_o {
    std::string s_background;
    std::string s_foreground;
    std::string s_label;
};

std::string xml_escape(const std::string &s_text) {
    std::string s_escaped;
    s_escaped.reserve(s_text.size());
    for (const char c_next : s_text) {
        switch (c_next) {
            case '&':
                s_escaped += "&amp;";
                break;
            case '<':
                s_escaped += "&lt;";
                break;
            case '>':
                s_escaped += "&gt;";
                break;
            case '"':
                s_escaped += "&quot;";
                break;
            case '\'':
                s_escaped += "&apos;";
                break;
            default:
                s_escaped.push_back(c_next);
                break;
        }
    }
    return s_escaped;
}

std::string color_text(const pkpass::color_o &o_color, const std::string &s_fallback) {
    if (!o_color.b_set) {
        return s_fallback;
    }
    std::ostringstream s_stream;
    s_stream << "rgb(" << o_color.i_red << ',' << o_color.i_green << ',' << o_color.i_blue << ')';
    return s_stream.str();
}

void add_text(std::ostringstream &s_body, int i_x, int i_y, int i_size, const char *s_anchor,
              const std::string &s_fill, bool b_bold, const std::string &s_text) {
    if (s_text.empty()) {
        return;
    }
    s_body << "  <text x=\"" << i_x << "\" y=\"" << i_y << "\" font-size=\"" << i_size
           << "\" text-anchor=\"" << s_anchor << "\" fill=\"" << s_fill << '"';
    if (b_bold) {
        s_body << " font-weight=\"bold\"";
    }
    s_body << '>' << xml_escape(s_text) << "</text>\n";
}

// Break text into lines of at most u_columns characters, at word boundaries
// where one exists.  SVG has no wrapping of its own, and a long event name must
// not run off the card.
std::vector<std::string> wrap_text(const std::string &s_text, std::size_t u_columns) {
    std::vector<std::string> o_lines;
    std::istringstream s_stream(s_text);
    std::string s_word;
    std::string s_line;
    while (s_stream >> s_word) {
        if (s_line.empty()) {
            s_line = s_word;
        } else if (s_line.size() + 1 + s_word.size() <= u_columns) {
            s_line += " " + s_word;
        } else {
            o_lines.push_back(s_line);
            s_line = s_word;
        }
        while (s_line.size() > u_columns) {
            o_lines.push_back(s_line.substr(0, u_columns));
            s_line = s_line.substr(u_columns);
        }
    }
    if (!s_line.empty()) {
        o_lines.push_back(s_line);
    }
    if (o_lines.empty()) {
        o_lines.emplace_back();
    }
    return o_lines;
}

std::size_t columns_for(int i_width, int i_size) {
    const int i_characters = i_width * 10 / (i_size * CHARACTER_WIDTH_TENTHS);
    return (1 > i_characters) ? 1 : static_cast<std::size_t>(i_characters);
}

bool png_dimensions(const std::string &s_data, unsigned int &u_width, unsigned int &u_height) {
    // A PNG is a signature, then the IHDR chunk at a fixed offset; its width and
    // height are the first two big-endian 32-bit words of the chunk data.
    if (24 > s_data.size() || 0 != s_data.compare(0, 8, "\x89PNG\r\n\x1a\n", 8)) {
        return false;
    }
    const auto read_word = [&s_data](std::size_t u_at) {
        return (static_cast<unsigned int>(static_cast<unsigned char>(s_data[u_at])) << 24) |
               (static_cast<unsigned int>(static_cast<unsigned char>(s_data[u_at + 1])) << 16) |
               (static_cast<unsigned int>(static_cast<unsigned char>(s_data[u_at + 2])) << 8) |
               static_cast<unsigned int>(static_cast<unsigned char>(s_data[u_at + 3]));
    };
    u_width = read_word(16);
    u_height = read_word(20);
    return 0 != u_width && 0 != u_height;
}

// Pick an image of a family: the 2x copy when the pass carries it, otherwise the
// plain name, otherwise 3x.  2x is the size the card is drawn at, so the plain
// copy would be softer and 3x would be stored for no visible gain.
std::string best_image(const zip::archive_c &o_archive, const std::string &s_base) {
    for (const char *s_suffix : {"@2x", "", "@3x"}) {
        const std::string s_name = s_base + s_suffix + ".png";
        if (o_archive.contains(s_name)) {
            return s_name;
        }
    }
    return std::string();
}

palette_o palette_for(const pkpass::pass_o &o_pass) {
    palette_o o_palette;
    o_palette.s_background = color_text(o_pass.o_background_color, "#263238");
    o_palette.s_foreground = color_text(o_pass.o_foreground_color, "#ffffff");
    o_palette.s_label = color_text(o_pass.o_label_color, o_palette.s_foreground);
    return o_palette;
}

const pkpass::field_group_o *group_named(const pkpass::pass_o &o_pass, const char *s_name) {
    for (const pkpass::field_group_o &o_group : o_pass.o_field_groups) {
        if (o_group.s_name == s_name) {
            return &o_group;
        }
    }
    return nullptr;
}

// One row of fields, each in its own column, label above value.  Returns the
// height the row advanced to.
int add_field_row(std::ostringstream &s_body, int i_y, const pkpass::field_group_o &o_group,
                  const palette_o &o_palette, const std::string &s_label_color, int i_label_size,
                  int i_value_size) {
    if (o_group.o_fields.empty()) {
        return i_y;
    }
    const int i_count = static_cast<int>(o_group.o_fields.size());
    const int i_column_width = CONTENT_WIDTH / i_count;
    int i_value_lines = 1;

    for (int i_field = 0; i_field < i_count; ++i_field) {
        const pkpass::field_o &o_field = o_group.o_fields[static_cast<std::size_t>(i_field)];
        const int i_center = MARGIN + i_column_width * i_field + i_column_width / 2;
        int i_line_y = i_y;
        if (!o_field.s_label.empty()) {
            add_text(s_body, i_center, i_line_y, i_label_size, "middle", s_label_color, false,
                     o_field.s_label);
        }
        const std::string s_value =
            o_field.s_attributed_value.empty() ? o_field.s_value : o_field.s_attributed_value;
        const std::vector<std::string> o_lines =
            wrap_text(s_value, columns_for(i_column_width - 4, i_value_size));
        i_line_y += i_value_size + 2;
        for (const std::string &s_line : o_lines) {
            add_text(s_body, i_center, i_line_y, i_value_size, "middle", o_palette.s_foreground,
                     false, s_line);
            i_line_y += i_value_size + 3;
        }
        const int i_used = static_cast<int>(o_lines.size());
        if (i_value_lines < i_used) {
            i_value_lines = i_used;
        }
    }

    return i_y + i_label_size + 4 + i_value_lines * (i_value_size + 3) + 6;
}

int add_back_fields(std::ostringstream &s_body, int i_y, const pkpass::pass_o &o_pass,
                    const palette_o &o_palette, const std::string &s_label_color) {
    int i_line_y = i_y;
    for (const pkpass::field_group_o &o_group : o_pass.o_field_groups) {
        if ("backFields" != o_group.s_name) {
            continue;
        }
        for (const pkpass::field_o &o_field : o_group.o_fields) {
            if (!o_field.s_label.empty()) {
                add_text(s_body, MARGIN, i_line_y, 12, "start", s_label_color, true,
                         o_field.s_label);
                i_line_y += 16;
            }
            const std::string s_value =
                o_field.s_attributed_value.empty() ? o_field.s_value : o_field.s_attributed_value;
            for (const std::string &s_line : wrap_text(s_value, columns_for(CONTENT_WIDTH, 13))) {
                add_text(s_body, MARGIN, i_line_y, 13, "start", o_palette.s_foreground, false,
                         s_line);
                i_line_y += 17;
            }
            i_line_y += 8;
        }
    }
    return i_line_y;
}

int add_barcode(std::ostringstream &s_body, int i_y, const pkpass::pass_o &o_pass) {
    if (o_pass.o_barcodes.empty()) {
        return i_y;
    }
    const pkpass::barcode_o &o_barcode = o_pass.o_barcodes.front();
    const std::string s_format = pkpass::barcode_format_label(o_barcode.s_format);

    s_body << "  <rect x=\"" << MARGIN << "\" y=\"" << i_y << "\" width=\"" << CONTENT_WIDTH
           << "\" height=\"" << BARCODE_HEIGHT << "\" rx=\"8\" fill=\"#ffffff\"/>\n";
    // The symbol itself is not drawn: a scannable bitmap needs a QR and PDF417
    // encoder this project does not have yet, and a symbol that might not scan
    // would be worse than the payload a person can read and copy.
    add_text(s_body, CARD_WIDTH / 2, i_y + 20, 11, "middle", "#666666", false,
             s_format + " barcode (not drawn)");
    if (!o_barcode.s_alt_text.empty()) {
        add_text(s_body, CARD_WIDTH / 2, i_y + 42, 16, "middle", "#111111", true,
                 o_barcode.s_alt_text);
    }
    int i_line_y = i_y + 64;
    const std::vector<std::string> o_lines =
        wrap_text(o_barcode.s_message, columns_for(CONTENT_WIDTH - 24, 11));
    for (std::size_t u_line = 0; u_line < o_lines.size(); ++u_line) {
        if (i_y + BARCODE_HEIGHT - 8 < i_line_y) {
            add_text(s_body, MARGIN + 12, i_line_y, 11, "start", "#666666", false, "…");
            break;
        }
        add_text(s_body, MARGIN + 12, i_line_y, 11, "start", "#111111", false, o_lines[u_line]);
        i_line_y += 14;
    }
    return i_y + BARCODE_HEIGHT + 10;
}

std::string compose(const std::string &s_body, int i_height, const palette_o &o_palette) {
    std::ostringstream s_svg;
    s_svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    s_svg << "<svg xmlns=\"http://www.w3.org/2000/svg\""
             " xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\""
          << CARD_WIDTH << "\" height=\"" << i_height << "\" viewBox=\"0 0 " << CARD_WIDTH << ' '
          << i_height << "\">\n";
    s_svg << "  <rect x=\"0\" y=\"0\" width=\"" << CARD_WIDTH << "\" height=\"" << i_height
          << "\" rx=\"14\" fill=\"" << o_palette.s_background << "\"/>\n";
    s_svg << "  <g font-family=\"Cantarell, DejaVu Sans, Helvetica, sans-serif\">\n";
    s_svg << s_body;
    s_svg << "  </g>\n</svg>\n";
    return s_svg.str();
}

std::string render_impl(const zip::archive_c *p_archive, const pkpass::pass_o &o_pass,
                        const options_o &o_options) {
    const palette_o o_palette = palette_for(o_pass);
    const std::string s_label_color =
        o_pass.o_label_color.b_set ? o_palette.s_label : o_palette.s_foreground;
    std::ostringstream s_body;
    int i_y = MARGIN;

    if (nullptr != p_archive && o_options.b_embed_images) {
        const std::string s_logo_name = best_image(*p_archive, "logo");
        if (!s_logo_name.empty()) {
            std::string s_logo;
            std::string s_logo_error;
            unsigned int u_width = 0;
            unsigned int u_height = 0;
            if (p_archive->extract(s_logo_name, s_logo, s_logo_error) &&
                png_dimensions(s_logo, u_width, u_height)) {
                int i_draw_width = LOGO_MAX_WIDTH;
                int i_draw_height = static_cast<int>(
                    static_cast<unsigned long long>(u_height) *
                    static_cast<unsigned long long>(i_draw_width) / u_width);
                if (LOGO_MAX_HEIGHT < i_draw_height) {
                    i_draw_height = LOGO_MAX_HEIGHT;
                    i_draw_width = static_cast<int>(
                        static_cast<unsigned long long>(u_width) *
                        static_cast<unsigned long long>(i_draw_height) / u_height);
                }
                const int i_x = (CARD_WIDTH - i_draw_width) / 2;
                s_body << "  <image x=\"" << i_x << "\" y=\"" << i_y << "\" width=\""
                       << i_draw_width << "\" height=\"" << i_draw_height
                       << "\" xlink:href=\"data:image/png;base64,"
                       << util::base64_encode(s_logo) << "\"/>\n";
                i_y += i_draw_height + 8;
            }
        }
    }

    if (!o_pass.s_logo_text.empty()) {
        add_text(s_body, CARD_WIDTH / 2, i_y + 16, 20, "middle", o_palette.s_foreground, true,
                 o_pass.s_logo_text);
        i_y += 26;
    } else {
        i_y += 6;
    }

    const pkpass::field_group_o *p_header = group_named(o_pass, "headerFields");
    if (nullptr != p_header) {
        i_y = add_field_row(s_body, i_y, *p_header, o_palette, s_label_color, 11, 16);
    }
    const pkpass::field_group_o *p_primary = group_named(o_pass, "primaryFields");
    if (nullptr != p_primary) {
        i_y = add_field_row(s_body, i_y, *p_primary, o_palette, s_label_color, 12, 24);
    }
    const pkpass::field_group_o *p_secondary = group_named(o_pass, "secondaryFields");
    if (nullptr != p_secondary) {
        i_y = add_field_row(s_body, i_y, *p_secondary, o_palette, s_label_color, 11, 15);
    }
    const pkpass::field_group_o *p_auxiliary = group_named(o_pass, "auxiliaryFields");
    if (nullptr != p_auxiliary) {
        i_y = add_field_row(s_body, i_y, *p_auxiliary, o_palette, s_label_color, 11, 15);
    }

    i_y = add_barcode(s_body, i_y + 4, o_pass);

    if (o_options.b_render_back) {
        add_text(s_body, CARD_WIDTH / 2, i_y + 18, 13, "middle", s_label_color, true,
                 "Back of pass");
        i_y = add_back_fields(s_body, i_y + 34, o_pass, o_palette, s_label_color);
    }

    i_y += MARGIN;
    return compose(s_body.str(), i_y, o_palette);
}

}  // namespace

std::string render_svg_without_images(const pkpass::pass_o &o_pass, const options_o &o_options) {
    return render_impl(nullptr, o_pass, o_options);
}

std::string render_svg(const std::string &s_path, const pkpass::pass_o &o_pass,
                       const options_o &o_options, std::string &s_error) {
    s_error.clear();
    zip::archive_c o_archive;
    std::string s_archive_error;
    const bool b_loaded = o_archive.load_file(s_path, s_archive_error);
    if (!b_loaded) {
        s_error = s_archive_error;
    }
    return render_impl(b_loaded ? &o_archive : nullptr, o_pass, o_options);
}

}  // namespace gnome_pkpass::render
