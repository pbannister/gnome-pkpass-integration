#include "json/json_reader.h"

#include <cstdint>
#include <cstdlib>
#include <sstream>

namespace gnome_pkpass::json {
namespace {

const std::string &empty_string() {
    static const std::string s_empty;
    return s_empty;
}

const std::vector<value_c> &empty_items() {
    static const std::vector<value_c> s_empty;
    return s_empty;
}

const std::vector<std::pair<std::string, value_c>> &empty_members() {
    static const std::vector<std::pair<std::string, value_c>> s_empty;
    return s_empty;
}

// Append one code point as UTF-8.
void append_utf8(std::string &s_text, std::uint32_t u_code) {
    if (0x80 > u_code) {
        s_text.push_back(static_cast<char>(u_code));
    } else if (0x800 > u_code) {
        s_text.push_back(static_cast<char>(0xc0 | (u_code >> 6)));
        s_text.push_back(static_cast<char>(0x80 | (u_code & 0x3f)));
    } else if (0x10000 > u_code) {
        s_text.push_back(static_cast<char>(0xe0 | (u_code >> 12)));
        s_text.push_back(static_cast<char>(0x80 | ((u_code >> 6) & 0x3f)));
        s_text.push_back(static_cast<char>(0x80 | (u_code & 0x3f)));
    } else {
        s_text.push_back(static_cast<char>(0xf0 | (u_code >> 18)));
        s_text.push_back(static_cast<char>(0x80 | ((u_code >> 12) & 0x3f)));
        s_text.push_back(static_cast<char>(0x80 | ((u_code >> 6) & 0x3f)));
        s_text.push_back(static_cast<char>(0x80 | (u_code & 0x3f)));
    }
}

int hex_value(char c_digit) {
    if ('0' <= c_digit && '9' >= c_digit) {
        return c_digit - '0';
    }
    if ('a' <= c_digit && 'f' >= c_digit) {
        return c_digit - 'a' + 10;
    }
    if ('A' <= c_digit && 'F' >= c_digit) {
        return c_digit - 'A' + 10;
    }
    return -1;
}

// A recursive-descent reader over one string.  The cursor is a byte offset, and
// every error names the offset, because a malformed pass is otherwise a blank
// failure.
class reader_c {
public:
    reader_c(const std::string &s_text, std::string &s_error)
        : s_text_(s_text), s_error_(s_error) {}

    bool read_value(value_c &o_value) {
        skip_space();
        if (u_at_ >= s_text_.size()) {
            return fail("unexpected end of input");
        }
        switch (s_text_[u_at_]) {
            case '{':
                return read_object(o_value);
            case '[':
                return read_array(o_value);
            case '"': {
                std::string s_text;
                if (!read_string(s_text)) {
                    return false;
                }
                o_value = value_c::make_string(s_text);
                return true;
            }
            case 't':
                return read_literal("true", value_c::make_boolean(true), o_value);
            case 'f':
                return read_literal("false", value_c::make_boolean(false), o_value);
            case 'n':
                return read_literal("null", value_c::make_null(), o_value);
            default:
                return read_number(o_value);
        }
    }

    bool at_end() {
        skip_space();
        return u_at_ >= s_text_.size();
    }

private:
    bool fail(const std::string &s_why) {
        std::ostringstream s_stream;
        s_stream << s_why << " at byte " << u_at_;
        s_error_ = s_stream.str();
        return false;
    }

    void skip_space() {
        while (u_at_ < s_text_.size()) {
            const char c_next = s_text_[u_at_];
            if (' ' == c_next || '\t' == c_next || '\n' == c_next || '\r' == c_next) {
                ++u_at_;
            } else {
                break;
            }
        }
    }

    bool read_literal(const char *s_literal, value_c o_value, value_c &o_result) {
        const std::size_t u_size = std::char_traits<char>::length(s_literal);
        if (s_text_.compare(u_at_, u_size, s_literal) != 0) {
            return fail("invalid literal");
        }
        u_at_ += u_size;
        o_result = o_value;
        return true;
    }

    bool read_hex4(std::uint32_t &u_code) {
        if (u_at_ + 4 > s_text_.size()) {
            return fail("truncated \\u escape");
        }
        u_code = 0;
        for (std::size_t u_digit = 0; u_digit < 4; ++u_digit) {
            const int i_value = hex_value(s_text_[u_at_ + u_digit]);
            if (0 > i_value) {
                u_at_ += u_digit;
                return fail("invalid \\u escape");
            }
            u_code = (u_code << 4) | static_cast<std::uint32_t>(i_value);
        }
        u_at_ += 4;
        return true;
    }

    bool read_string(std::string &s_value) {
        ++u_at_;  // the opening quote
        s_value.clear();
        for (;;) {
            if (u_at_ >= s_text_.size()) {
                return fail("unterminated string");
            }
            const char c_next = s_text_[u_at_];
            if ('"' == c_next) {
                ++u_at_;
                return true;
            }
            if ('\\' == c_next) {
                ++u_at_;
                if (u_at_ >= s_text_.size()) {
                    return fail("unterminated escape");
                }
                const char c_escape = s_text_[u_at_];
                ++u_at_;
                switch (c_escape) {
                    case '"':
                        s_value.push_back('"');
                        break;
                    case '\\':
                        s_value.push_back('\\');
                        break;
                    case '/':
                        s_value.push_back('/');
                        break;
                    case 'b':
                        s_value.push_back('\b');
                        break;
                    case 'f':
                        s_value.push_back('\f');
                        break;
                    case 'n':
                        s_value.push_back('\n');
                        break;
                    case 'r':
                        s_value.push_back('\r');
                        break;
                    case 't':
                        s_value.push_back('\t');
                        break;
                    case 'u': {
                        std::uint32_t u_code = 0;
                        if (!read_hex4(u_code)) {
                            return false;
                        }
                        // A surrogate pair is two escapes; combine them so a
                        // non-BMP character survives as one code point.
                        if (0xd800 <= u_code && 0xdc00 > u_code &&
                            u_at_ + 1 < s_text_.size() && '\\' == s_text_[u_at_] &&
                            'u' == s_text_[u_at_ + 1]) {
                            const std::size_t u_back = u_at_;
                            u_at_ += 2;
                            std::uint32_t u_low = 0;
                            if (!read_hex4(u_low)) {
                                return false;
                            }
                            if (0xdc00 <= u_low && 0xe000 > u_low) {
                                u_code = 0x10000 + ((u_code - 0xd800) << 10) + (u_low - 0xdc00);
                            } else {
                                u_at_ = u_back;
                            }
                        }
                        append_utf8(s_value, u_code);
                        break;
                    }
                    default:
                        return fail("invalid escape");
                }
                continue;
            }
            s_value.push_back(c_next);
            ++u_at_;
        }
    }

    bool read_number(value_c &o_value) {
        const std::size_t u_start = u_at_;
        if (u_at_ < s_text_.size() && ('-' == s_text_[u_at_] || '+' == s_text_[u_at_])) {
            ++u_at_;
        }
        bool b_digits = false;
        while (u_at_ < s_text_.size() && '0' <= s_text_[u_at_] && '9' >= s_text_[u_at_]) {
            ++u_at_;
            b_digits = true;
        }
        if (u_at_ < s_text_.size() && '.' == s_text_[u_at_]) {
            ++u_at_;
            while (u_at_ < s_text_.size() && '0' <= s_text_[u_at_] && '9' >= s_text_[u_at_]) {
                ++u_at_;
                b_digits = true;
            }
        }
        if (!b_digits) {
            return fail("invalid value");
        }
        if (u_at_ < s_text_.size() && ('e' == s_text_[u_at_] || 'E' == s_text_[u_at_])) {
            ++u_at_;
            if (u_at_ < s_text_.size() && ('-' == s_text_[u_at_] || '+' == s_text_[u_at_])) {
                ++u_at_;
            }
            while (u_at_ < s_text_.size() && '0' <= s_text_[u_at_] && '9' >= s_text_[u_at_]) {
                ++u_at_;
            }
        }

        const std::string s_number = s_text_.substr(u_start, u_at_ - u_start);
        o_value = value_c::make_number(std::strtod(s_number.c_str(), nullptr));
        return true;
    }

    bool read_array(value_c &o_value) {
        ++u_at_;  // the opening bracket
        o_value = value_c::make_array();
        skip_space();
        if (u_at_ < s_text_.size() && ']' == s_text_[u_at_]) {
            ++u_at_;
            return true;
        }
        for (;;) {
            value_c o_item;
            if (!read_value(o_item)) {
                return false;
            }
            o_value.append_item(o_item);
            skip_space();
            if (u_at_ >= s_text_.size()) {
                return fail("unterminated array");
            }
            const char c_next = s_text_[u_at_];
            ++u_at_;
            if (']' == c_next) {
                return true;
            }
            if (',' != c_next) {
                --u_at_;
                return fail("expected ',' or ']'");
            }
        }
    }

    bool read_object(value_c &o_value) {
        ++u_at_;  // the opening brace
        o_value = value_c::make_object();
        skip_space();
        if (u_at_ < s_text_.size() && '}' == s_text_[u_at_]) {
            ++u_at_;
            return true;
        }
        for (;;) {
            skip_space();
            if (u_at_ >= s_text_.size() || '"' != s_text_[u_at_]) {
                return fail("expected a member name");
            }
            std::string s_key;
            if (!read_string(s_key)) {
                return false;
            }
            skip_space();
            if (u_at_ >= s_text_.size() || ':' != s_text_[u_at_]) {
                return fail("expected ':'");
            }
            ++u_at_;
            value_c o_member;
            if (!read_value(o_member)) {
                return false;
            }
            o_value.set_member(s_key, o_member);
            skip_space();
            if (u_at_ >= s_text_.size()) {
                return fail("unterminated object");
            }
            const char c_next = s_text_[u_at_];
            ++u_at_;
            if ('}' == c_next) {
                return true;
            }
            if (',' != c_next) {
                --u_at_;
                return fail("expected ',' or '}'");
            }
        }
    }

    const std::string &s_text_;
    std::string &s_error_;
    std::size_t u_at_ = 0;
};

void write_escaped(std::ostringstream &s_stream, const std::string &s_text) {
    s_stream << '"';
    for (const char c_next : s_text) {
        const unsigned char u_next = static_cast<unsigned char>(c_next);
        switch (c_next) {
            case '"':
                s_stream << "\\\"";
                break;
            case '\\':
                s_stream << "\\\\";
                break;
            case '\b':
                s_stream << "\\b";
                break;
            case '\f':
                s_stream << "\\f";
                break;
            case '\n':
                s_stream << "\\n";
                break;
            case '\r':
                s_stream << "\\r";
                break;
            case '\t':
                s_stream << "\\t";
                break;
            default:
                if (0x20 > u_next) {
                    static const char *const s_hex = "0123456789abcdef";
                    s_stream << "\\u00" << s_hex[(u_next >> 4) & 0x0f] << s_hex[u_next & 0x0f];
                } else {
                    s_stream << c_next;
                }
                break;
        }
    }
    s_stream << '"';
}

void write_number(std::ostringstream &s_stream, double d_value) {
    // A whole number prints without a decimal point, so `serialNumber: 1` and
    // `formatVersion: 1` read back exactly as they were written.
    if (-1e15 < d_value && 1e15 > d_value &&
        d_value == static_cast<double>(static_cast<long long>(d_value))) {
        s_stream << static_cast<long long>(d_value);
    } else {
        s_stream << d_value;
    }
}

void write_indent(std::ostringstream &s_stream, int i_depth) {
    for (int i_step = 0; i_step < i_depth; ++i_step) {
        s_stream << "  ";
    }
}

void write_value(std::ostringstream &s_stream, const value_c &o_value, int i_depth) {
    switch (o_value.kind()) {
        case value_c::kind_e::null_value:
            s_stream << "null";
            break;
        case value_c::kind_e::boolean:
            s_stream << (o_value.as_boolean() ? "true" : "false");
            break;
        case value_c::kind_e::number:
            write_number(s_stream, o_value.as_number());
            break;
        case value_c::kind_e::string:
            write_escaped(s_stream, o_value.as_string());
            break;
        case value_c::kind_e::array: {
            const std::vector<value_c> &o_items = o_value.items();
            if (o_items.empty()) {
                s_stream << "[]";
                break;
            }
            s_stream << "[\n";
            for (std::size_t u_index = 0; u_index < o_items.size(); ++u_index) {
                write_indent(s_stream, i_depth + 1);
                write_value(s_stream, o_items[u_index], i_depth + 1);
                if (u_index + 1 < o_items.size()) {
                    s_stream << ',';
                }
                s_stream << '\n';
            }
            write_indent(s_stream, i_depth);
            s_stream << ']';
            break;
        }
        case value_c::kind_e::object: {
            const std::vector<std::pair<std::string, value_c>> &o_members = o_value.members();
            if (o_members.empty()) {
                s_stream << "{}";
                break;
            }
            s_stream << "{\n";
            for (std::size_t u_index = 0; u_index < o_members.size(); ++u_index) {
                write_indent(s_stream, i_depth + 1);
                write_escaped(s_stream, o_members[u_index].first);
                s_stream << ": ";
                write_value(s_stream, o_members[u_index].second, i_depth + 1);
                if (u_index + 1 < o_members.size()) {
                    s_stream << ',';
                }
                s_stream << '\n';
            }
            write_indent(s_stream, i_depth);
            s_stream << '}';
            break;
        }
    }
}

}  // namespace

bool value_c::parse(const std::string &s_text, value_c &o_value, std::string &s_error) {
    reader_c o_reader(s_text, s_error);
    value_c o_parsed;
    if (!o_reader.read_value(o_parsed)) {
        o_value = value_c();
        return false;
    }
    if (!o_reader.at_end()) {
        s_error = "trailing content after the document";
        o_value = value_c();
        return false;
    }
    o_value = o_parsed;
    return true;
}

value_c value_c::make_null() {
    return value_c();
}

value_c value_c::make_boolean(bool b_value) {
    value_c o_value;
    o_value.e_kind_ = kind_e::boolean;
    o_value.b_boolean_ = b_value;
    return o_value;
}

value_c value_c::make_number(double d_value) {
    value_c o_value;
    o_value.e_kind_ = kind_e::number;
    o_value.d_number_ = d_value;
    return o_value;
}

value_c value_c::make_string(const std::string &s_text) {
    value_c o_value;
    o_value.e_kind_ = kind_e::string;
    o_value.s_string_ = s_text;
    return o_value;
}

value_c value_c::make_array() {
    value_c o_value;
    o_value.e_kind_ = kind_e::array;
    return o_value;
}

value_c value_c::make_object() {
    value_c o_value;
    o_value.e_kind_ = kind_e::object;
    return o_value;
}

void value_c::append_item(value_c o_item) {
    if (is_array()) {
        o_items_.push_back(o_item);
    }
}

void value_c::set_member(const std::string &s_key, value_c o_value) {
    if (!is_object()) {
        return;
    }
    for (std::pair<std::string, value_c> &o_member : o_members_) {
        if (o_member.first == s_key) {
            o_member.second = o_value;
            return;
        }
    }
    o_members_.emplace_back(s_key, o_value);
}

value_c::kind_e value_c::kind() const {
    return e_kind_;
}

bool value_c::is_null() const {
    return kind_e::null_value == e_kind_;
}

bool value_c::is_boolean() const {
    return kind_e::boolean == e_kind_;
}

bool value_c::is_number() const {
    return kind_e::number == e_kind_;
}

bool value_c::is_string() const {
    return kind_e::string == e_kind_;
}

bool value_c::is_array() const {
    return kind_e::array == e_kind_;
}

bool value_c::is_object() const {
    return kind_e::object == e_kind_;
}

bool value_c::as_boolean(bool b_fallback) const {
    return is_boolean() ? b_boolean_ : b_fallback;
}

double value_c::as_number(double d_fallback) const {
    return is_number() ? d_number_ : d_fallback;
}

const std::string &value_c::as_string() const {
    return is_string() ? s_string_ : empty_string();
}

const std::vector<value_c> &value_c::items() const {
    return is_array() ? o_items_ : empty_items();
}

const value_c *value_c::member(const std::string &s_key) const {
    if (!is_object()) {
        return nullptr;
    }
    for (const std::pair<std::string, value_c> &o_member : o_members_) {
        if (o_member.first == s_key) {
            return &o_member.second;
        }
    }
    return nullptr;
}

const std::vector<std::pair<std::string, value_c>> &value_c::members() const {
    return is_object() ? o_members_ : empty_members();
}

std::string value_c::string_or(const std::string &s_key, const std::string &s_fallback) const {
    const value_c *p_member = member(s_key);
    if (nullptr == p_member) {
        return s_fallback;
    }
    if (p_member->is_string()) {
        return p_member->as_string();
    }
    if (p_member->is_number() || p_member->is_boolean()) {
        std::ostringstream s_stream;
        if (p_member->is_number()) {
            write_number(s_stream, p_member->as_number());
        } else {
            s_stream << (p_member->as_boolean() ? "true" : "false");
        }
        return s_stream.str();
    }
    return s_fallback;
}

bool value_c::boolean_or(const std::string &s_key, bool b_fallback) const {
    const value_c *p_member = member(s_key);
    return (nullptr != p_member) ? p_member->as_boolean(b_fallback) : b_fallback;
}

double value_c::number_or(const std::string &s_key, double d_fallback) const {
    const value_c *p_member = member(s_key);
    return (nullptr != p_member) ? p_member->as_number(d_fallback) : d_fallback;
}

std::vector<value_c> value_c::array_or_empty(const std::string &s_key) const {
    const value_c *p_member = member(s_key);
    if (nullptr == p_member || !p_member->is_array()) {
        return std::vector<value_c>();
    }
    return p_member->items();
}

std::string value_c::to_text() const {
    std::ostringstream s_stream;
    write_value(s_stream, *this, 0);
    return s_stream.str();
}

}  // namespace gnome_pkpass::json
