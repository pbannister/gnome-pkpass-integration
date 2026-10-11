#include "render/pass_render.h"

#include "json/json_reader.h"
#include "util/file_io.h"
#include "zip/zip_reader.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

namespace gnome_pkpass::render {
namespace {

constexpr int CARD_WIDTH = 420;
constexpr int MARGIN = 20;
constexpr int CONTENT_WIDTH = CARD_WIDTH - 2 * MARGIN;
constexpr int LOGO_MAX_WIDTH = 220;
constexpr int LOGO_MAX_HEIGHT = 64;
constexpr int BARCODE_PADDING = 12;
constexpr int BARCODE_QR_MAX = 180;
constexpr int BARCODE_LINE_HEIGHT = 14;
constexpr int BARCODE_MAX_LINES = 14;
constexpr int CHARACTER_WIDTH_TENTHS = 6;
constexpr qr::level_e BARCODE_LEVEL = qr::level_e::medium;

struct palette_o {
    std::string s_background;
    std::string s_foreground;
    std::string s_label;
};

struct rich_line_o {
    std::vector<text_run_o> o_runs;
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

bool is_url_start(const std::string &s_text, std::size_t u_at) {
    return 0 == s_text.compare(u_at, 7, "http://") || 0 == s_text.compare(u_at, 8, "https://");
}

// The end of a URL: the first space or markup character, with the sentence
// punctuation that commonly follows a URL left out of the link.
std::size_t url_end(const std::string &s_text, std::size_t u_at) {
    std::size_t u_end = u_at;
    while (u_end < s_text.size()) {
        const char c_next = s_text[u_end];
        if (' ' == c_next || '\t' == c_next || '\n' == c_next || '<' == c_next ||
            '>' == c_next || '"' == c_next) {
            break;
        }
        ++u_end;
    }
    while (u_end > u_at) {
        const char c_last = s_text[u_end - 1];
        if ('.' == c_last || ',' == c_last || ';' == c_last || ':' == c_last || '!' == c_last ||
            '?' == c_last || ')' == c_last || ']' == c_last || '}' == c_last) {
            --u_end;
            continue;
        }
        break;
    }
    return u_end;
}

std::size_t text_width(const std::string &s_text, int i_size) {
    return s_text.size() * static_cast<std::size_t>(i_size) * CHARACTER_WIDTH_TENTHS / 10;
}

// A single line of runs, wrapped to a character budget.  A link is kept whole
// unless it is longer than the line on its own.
std::vector<rich_line_o> wrap_runs(const std::vector<text_run_o> &o_runs, std::size_t u_columns) {
    std::vector<rich_line_o> o_lines;
    rich_line_o o_line;
    std::size_t u_length = 0;
    const std::size_t u_limit = (0 == u_columns) ? 1 : u_columns;

    const auto flush = [&o_lines, &o_line, &u_length]() {
        if (!o_line.o_runs.empty()) {
            o_lines.push_back(o_line);
            o_line = rich_line_o();
            u_length = 0;
        }
    };

    for (const text_run_o &o_run : o_runs) {
        std::size_t u_at = 0;
        while (u_at < o_run.s_text.size()) {
            // A space separates words; a newline both separates and ends the
            // line, because a pass field may carry one ("LAGUNA HILLS HIGH
            // SCHOOL" sits under the holder's name in the real passes).
            std::size_t u_separator = u_at;
            while (u_separator < o_run.s_text.size() && ' ' != o_run.s_text[u_separator] &&
                   '\n' != o_run.s_text[u_separator]) {
                ++u_separator;
            }
            const bool b_newline =
                (u_separator < o_run.s_text.size() && '\n' == o_run.s_text[u_separator]);
            std::string s_word = o_run.s_text.substr(u_at, u_separator - u_at);
            u_at = (u_separator < o_run.s_text.size()) ? u_separator + 1 : o_run.s_text.size();
            if (s_word.empty()) {
                if (b_newline) {
                    flush();
                }
                continue;
            }
            while (s_word.size() > u_limit) {
                flush();
                o_line.o_runs.push_back({s_word.substr(0, u_limit), o_run.s_url});
                u_length = u_limit;
                flush();
                s_word = s_word.substr(u_limit);
            }
            if (0 < u_length && u_length + 1 + s_word.size() > u_limit) {
                flush();
            }
            if (0 < u_length) {
                o_line.o_runs.push_back({" ", std::string()});
                ++u_length;
            }
            o_line.o_runs.push_back({s_word, o_run.s_url});
            u_length += s_word.size();
            if (b_newline) {
                flush();
            }
        }
    }
    flush();

    if (o_lines.empty()) {
        o_lines.push_back(rich_line_o());
    }
    return o_lines;
}


// The drawn width of a line, using the same per-character estimate the wrapper
// uses; SVG has no text metrics to ask for.
int line_pixel_width(const rich_line_o &o_line, int i_size) {
    int i_width = 0;
    for (const text_run_o &o_run : o_line.o_runs) {
        i_width += static_cast<int>(text_width(o_run.s_text, i_size));
    }
    return i_width;
}

// Split on newlines, keeping empty lines: a blank line in a pretty-printed JSON
// payload is part of its shape.
std::vector<std::string> split_lines(const std::string &s_text) {
    std::vector<std::string> o_lines;
    std::size_t u_at = 0;
    for (;;) {
        const std::size_t u_end = s_text.find('\n', u_at);
        if (std::string::npos == u_end) {
            o_lines.push_back(s_text.substr(u_at));
            break;
        }
        o_lines.push_back(s_text.substr(u_at, u_end - u_at));
        u_at = u_end + 1;
    }
    if (o_lines.empty()) {
        o_lines.emplace_back();
    }
    return o_lines;
}

// Emit one run: literal text as a <text>, a link as an <a> around its <text>.
// The two href spellings are both written because SVG 1.1 readers use xlink and
// SVG 2 readers use the plain attribute.
void add_run(std::ostringstream &s_body, int i_x, int i_y, int i_size, const text_run_o &o_run,
             const std::string &s_fill, bool b_bold, bool b_monospace, const std::string &s_link_fill) {
    if (o_run.s_text.empty()) {
        return;
    }
    const bool b_link = !o_run.s_url.empty();
    if (b_link) {
        const std::string s_url = xml_escape(o_run.s_url);
        s_body << "  <a xlink:href=\"" << s_url << "\" href=\"" << s_url << "\" target=\"_blank\">\n";
    }
    s_body << "  <text x=\"" << i_x << "\" y=\"" << i_y << "\" font-size=\"" << i_size
           << "\" fill=\"" << (b_link ? s_link_fill : s_fill) << '"';
    if (b_bold) {
        s_body << " font-weight=\"bold\"";
    }
    if (b_monospace) {
        s_body << " font-family=\"DejaVu Sans Mono, monospace\"";
    }
    if (b_link) {
        s_body << " text-decoration=\"underline\"";
    }
    s_body << '>' << xml_escape(o_run.s_text) << "</text>\n";
    if (b_link) {
        s_body << "  </a>\n";
    }
}

// One wrapped line of runs, left edge at i_x, so the caller can centre it.
void add_rich_line(std::ostringstream &s_body, int i_x, int i_y, int i_size,
                   const rich_line_o &o_line, const std::string &s_fill, bool b_bold,
                   bool b_monospace, const std::string &s_link_fill) {
    // Consecutive literal runs become one <text>, so a phrase stays a phrase in
    // the document and the card does not carry a element per word; only a link
    // needs its own element.
    int i_cursor = i_x;
    int i_literal_x = i_x;
    std::string s_literal;
    const auto flush_literal = [&]() {
        if (!s_literal.empty()) {
            const text_run_o o_run{s_literal, std::string()};
            add_run(s_body, i_literal_x, i_y, i_size, o_run, s_fill, b_bold, b_monospace,
                    s_link_fill);
            s_literal.clear();
        }
    };

    for (const text_run_o &o_run : o_line.o_runs) {
        if (o_run.s_url.empty()) {
            if (s_literal.empty()) {
                i_literal_x = i_cursor;
            }
            s_literal += o_run.s_text;
        } else {
            flush_literal();
            add_run(s_body, i_cursor, i_y, i_size, o_run, s_fill, b_bold, b_monospace, s_link_fill);
        }
        i_cursor += static_cast<int>(text_width(o_run.s_text, i_size));
    }
    flush_literal();
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

std::size_t columns_for(int i_width, int i_size) {
    const int i_characters = i_width * 10 / (i_size * CHARACTER_WIDTH_TENTHS);
    return (1 > i_characters) ? 1 : static_cast<std::size_t>(i_characters);
}

bool png_dimensions(const std::string &s_data, unsigned int &u_width, unsigned int &u_height) {
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

std::string field_value(const pkpass::field_o &o_field) {
    return o_field.s_attributed_value.empty() ? o_field.s_value : o_field.s_attributed_value;
}

// An `attributedValue` may carry HTML, so only then is the text treated as
// markup; a plain `value` is literal.
bool field_is_markup(const pkpass::field_o &o_field) {
    return !o_field.s_attributed_value.empty();
}

// One row of fields, each in its own column, label above value.  A value with a
// link keeps its links; the line is centred on the column using the same width
// estimate the wrapper uses.
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
        const std::vector<rich_line_o> o_lines =
            wrap_runs(linkify(field_value(o_field), field_is_markup(o_field)),
                      columns_for(i_column_width - 4, i_value_size));
        i_line_y += i_value_size + 2;
        for (const rich_line_o &o_line : o_lines) {
            const int i_start = i_center - line_pixel_width(o_line, i_value_size) / 2;
            add_rich_line(s_body, i_start, i_line_y, i_value_size, o_line, o_palette.s_foreground,
                          false, false, s_label_color);
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
            for (const rich_line_o &o_line :
                 wrap_runs(linkify(field_value(o_field), field_is_markup(o_field)),
                          columns_for(CONTENT_WIDTH, 13))) {
                add_rich_line(s_body, MARGIN, i_line_y, 13, o_line, o_palette.s_foreground, false,
                              false, s_label_color);
                i_line_y += 17;
            }
            i_line_y += 8;
        }
    }
    return i_line_y;
}

// The barcode block: a white tile with the QR symbol, the alternate text, and
// the payload.  The payload text is the message re-indented when it is JSON, but
// the symbol always encodes the original bytes.
int add_barcode(std::ostringstream &s_body, int i_y, const pkpass::pass_o &o_pass,
                bool b_draw_symbol) {
    if (o_pass.o_barcodes.empty()) {
        return i_y;
    }
    const pkpass::barcode_o &o_barcode = o_pass.o_barcodes.front();
    const std::string s_format = pkpass::barcode_format_label(o_barcode.s_format);

    qr::matrix_o o_matrix;
    std::string s_qr_error;
    const bool b_encoded = b_draw_symbol && encode_barcode(o_barcode, BARCODE_LEVEL, o_matrix, s_qr_error);

    // The payload text keeps its own line breaks and indentation: a JSON
    // payload is unreadable once its indentation is collapsed, and the symbol
    // above it is what a scanner reads anyway.
    const std::string s_message = display_message(o_barcode.s_message);
    const int i_message_columns =
        static_cast<int>(columns_for(CONTENT_WIDTH - 2 * BARCODE_PADDING, 11));
    std::vector<std::pair<int, rich_line_o>> o_message_lines;
    bool b_truncated = false;
    for (const std::string &s_source : split_lines(s_message)) {
        if (BARCODE_MAX_LINES <= static_cast<int>(o_message_lines.size())) {
            b_truncated = true;
            break;
        }
        std::size_t u_at = 0;
        int i_indent = 0;
        while (u_at < s_source.size() && (' ' == s_source[u_at] || '\t' == s_source[u_at])) {
            i_indent += ('\t' == s_source[u_at]) ? 4 : 1;
            ++u_at;
        }
        const int i_available =
            (i_message_columns > i_indent + 4) ? (i_message_columns - i_indent) : 4;
        for (const rich_line_o &o_line :
             wrap_runs(linkify(s_source.substr(u_at)), static_cast<std::size_t>(i_available))) {
            if (BARCODE_MAX_LINES <= static_cast<int>(o_message_lines.size())) {
                b_truncated = true;
                break;
            }
            o_message_lines.emplace_back(i_indent, o_line);
        }
    }
    if (o_message_lines.empty()) {
        o_message_lines.emplace_back(0, rich_line_o());
    }

    int i_pixel = 0;
    int i_symbol_size = 0;
    if (b_encoded) {
        i_pixel = BARCODE_QR_MAX / o_matrix.i_size;
        if (2 > i_pixel) {
            i_pixel = 2;
        }
        i_symbol_size = o_matrix.i_size * i_pixel;
        if (i_symbol_size > CONTENT_WIDTH - 2 * BARCODE_PADDING) {
            i_pixel = (CONTENT_WIDTH - 2 * BARCODE_PADDING) / o_matrix.i_size;
            i_symbol_size = o_matrix.i_size * i_pixel;
        }
    }

    int i_height = BARCODE_PADDING * 2;
    if (b_encoded) {
        i_height += i_symbol_size + 10;
    } else {
        i_height += 16;
    }
    if (!o_barcode.s_alt_text.empty()) {
        i_height += 22;
    }
    i_height += static_cast<int>(o_message_lines.size()) * BARCODE_LINE_HEIGHT;

    s_body << "  <rect x=\"" << MARGIN << "\" y=\"" << i_y << "\" width=\"" << CONTENT_WIDTH
           << "\" height=\"" << i_height << "\" rx=\"8\" fill=\"#ffffff\"/>\n";

    int i_cursor = i_y + BARCODE_PADDING;

    if (b_encoded) {
        const int i_x = (CARD_WIDTH - i_symbol_size) / 2;
        s_body << "  <path fill=\"#000000\" d=\""
               << qr::to_svg_path(o_matrix, i_x, i_cursor, i_pixel) << "\"/>\n";
        i_cursor += i_symbol_size + 10;
    } else {
        add_text(s_body, CARD_WIDTH / 2, i_cursor + 12, 11, "middle", "#666666", false,
                 s_format + " barcode could not be encoded");
        i_cursor += 16;
        if (!s_qr_error.empty()) {
            add_text(s_body, CARD_WIDTH / 2, i_cursor + 12, 10, "middle", "#666666", false,
                     s_qr_error);
            i_cursor += 16;
        }
    }

    if (!o_barcode.s_alt_text.empty()) {
        add_text(s_body, CARD_WIDTH / 2, i_cursor + 12, 16, "middle", "#111111", true,
                 o_barcode.s_alt_text);
        i_cursor += 22;
    }

    for (const std::pair<int, rich_line_o> &o_entry : o_message_lines) {
        const int i_indent_width = static_cast<int>(
            text_width(std::string(static_cast<std::size_t>(o_entry.first), ' '), 11));
        add_rich_line(s_body, MARGIN + BARCODE_PADDING + i_indent_width, i_cursor + 10, 11,
                      o_entry.second, "#111111", false, true, "#0645ad");
        i_cursor += BARCODE_LINE_HEIGHT;
    }
    if (b_truncated) {
        add_text(s_body, MARGIN + BARCODE_PADDING, i_cursor + 10, 11, "start", "#666666", false,
                 "…");
    }

    return i_y + i_height + 10;
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

    i_y = add_barcode(s_body, i_y + 4, o_pass, o_options.b_draw_barcode);

    if (o_options.b_render_back) {
        add_text(s_body, CARD_WIDTH / 2, i_y + 18, 13, "middle", s_label_color, true,
                 "Back of pass");
        i_y = add_back_fields(s_body, i_y + 34, o_pass, o_palette, s_label_color);
    }

    i_y += MARGIN;
    return compose(s_body.str(), i_y, o_palette);
}

}  // namespace

std::vector<text_run_o> linkify(const std::string &s_text, bool b_markup) {
    std::vector<text_run_o> o_runs;
    std::string s_literal;

    const auto flush_literal = [&o_runs, &s_literal]() {
        if (!s_literal.empty()) {
            o_runs.push_back({s_literal, std::string()});
            s_literal.clear();
        }
    };

    std::size_t u_at = 0;
    while (u_at < s_text.size()) {
        // An inline anchor, which is what `attributedValue` may carry.
        if (b_markup && 0 == s_text.compare(u_at, 9, "<a href=\"")) {
            const std::size_t u_quote = u_at + 9;
            const std::size_t u_close = s_text.find('"', u_quote);
            if (std::string::npos != u_close) {
                const std::size_t u_gt = s_text.find('>', u_close);
                const std::size_t u_end_tag =
                    (std::string::npos == u_gt) ? std::string::npos : s_text.find("</a>", u_gt);
                if (std::string::npos != u_end_tag) {
                    flush_literal();
                    o_runs.push_back({s_text.substr(u_gt + 1, u_end_tag - u_gt - 1),
                                      s_text.substr(u_quote, u_close - u_quote)});
                    u_at = u_end_tag + 4;
                    continue;
                }
            }
        }

        if (is_url_start(s_text, u_at)) {
            const std::size_t u_end = url_end(s_text, u_at);
            if (u_end > u_at) {
                flush_literal();
                const std::string s_url = s_text.substr(u_at, u_end - u_at);
                o_runs.push_back({s_url, s_url});
                u_at = u_end;
                continue;
            }
        }

        // Any other tag is dropped rather than shown as raw markup, but only
        // when the text really is markup.
        if (b_markup && '<' == s_text[u_at] && u_at + 1 < s_text.size() &&
            (0 != std::isalpha(static_cast<unsigned char>(s_text[u_at + 1])) ||
             '/' == s_text[u_at + 1])) {
            const std::size_t u_gt = s_text.find('>', u_at);
            if (std::string::npos != u_gt) {
                u_at = u_gt + 1;
                continue;
            }
        }

        s_literal.push_back(s_text[u_at]);
        ++u_at;
    }
    flush_literal();

    if (o_runs.empty()) {
        o_runs.push_back({std::string(), std::string()});
    }
    return o_runs;
}

std::string display_message(const std::string &s_message) {
    json::value_c o_value;
    std::string s_error;
    if (!json::value_c::parse(s_message, o_value, s_error)) {
        return s_message;
    }
    if (!o_value.is_object() && !o_value.is_array()) {
        return s_message;
    }
    return o_value.to_text();
}

std::string viewer_markup(const std::string &s_text, bool b_markup) {
    std::string s_markup;
    for (const text_run_o &o_run : linkify(s_text, b_markup)) {
        const std::string s_escaped = xml_escape(o_run.s_text);
        if (o_run.s_url.empty()) {
            s_markup += s_escaped;
        } else {
            s_markup += "<a href=\"" + xml_escape(o_run.s_url) + "\">" + s_escaped + "</a>";
        }
    }
    return s_markup;
}

bool encode_barcode(const pkpass::barcode_o &o_barcode, qr::level_e e_level, qr::matrix_o &o_matrix,
                    std::string &s_error) {
    if (o_barcode.s_message.empty()) {
        s_error = "the barcode carries no message to encode";
        return false;
    }
    return qr::encode(o_barcode.s_message, e_level, o_matrix, s_error);
}

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
