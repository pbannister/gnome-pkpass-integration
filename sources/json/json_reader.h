#pragma once

#include <string>
#include <utility>
#include <vector>

namespace gnome_pkpass::json {

// A minimal JSON value: enough to read `pass.json` and `manifest.json`, and to
// print a pass back as JSON for another program.  It is a value, not a
// general-purpose library; malformed input is reported through an error string
// with the byte offset that stopped the parse.
//
// Member order is preserved, so a printed pass keeps the field order the issuer
// wrote.
class value_c {
public:
    enum class kind_e { null_value, boolean, number, string, array, object };

    // Parse one document.  On failure the returned value is null and s_error
    // says why and where.
    static bool parse(const std::string &s_text, value_c &o_value, std::string &s_error);

    static value_c make_null();
    static value_c make_boolean(bool b_value);
    static value_c make_number(double d_value);
    static value_c make_string(const std::string &s_text);
    static value_c make_array();
    static value_c make_object();

    // Mutators used by the parser and by tests: an array grows with append_item,
    // an object with set_member.
    void append_item(value_c o_item);
    void set_member(const std::string &s_key, value_c o_value);

    kind_e kind() const;
    bool is_null() const;
    bool is_boolean() const;
    bool is_number() const;
    bool is_string() const;
    bool is_array() const;
    bool is_object() const;

    // Accessors for a value of the matching kind, otherwise a safe default.
    bool as_boolean(bool b_fallback = false) const;
    double as_number(double d_fallback = 0.0) const;
    const std::string &as_string() const;
    const std::vector<value_c> &items() const;

    // Members of an object, or nullptr when this value is not an object.
    const value_c *member(const std::string &s_key) const;
    const std::vector<std::pair<std::string, value_c>> &members() const;

    // The value a member would have, or the fallback when it is absent or has
    // another kind.  A number is rendered as text for a string reader, because
    // `serialNumber` is a string in the format but issuers write it as a number.
    std::string string_or(const std::string &s_key,
                          const std::string &s_fallback = std::string()) const;
    bool boolean_or(const std::string &s_key, bool b_fallback) const;
    double number_or(const std::string &s_key, double d_fallback = 0.0) const;
    std::vector<value_c> array_or_empty(const std::string &s_key) const;

    // Serialise with two-space indentation, preserving member order.
    std::string to_text() const;

private:
    kind_e e_kind_ = kind_e::null_value;
    bool b_boolean_ = false;
    double d_number_ = 0.0;
    std::string s_string_;
    std::vector<value_c> o_items_;
    std::vector<std::pair<std::string, value_c>> o_members_;
};

}  // namespace gnome_pkpass::json
