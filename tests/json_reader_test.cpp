//
// json_reader_test.cpp: the JSON reader the pass and manifest readers depend on.
//
#include "json/json_reader.h"

#include "test_harness.h"

int main() {
    using gnome_pkpass::json::value_c;

    // A document with the shapes pass.json uses.
    const std::string s_pass =
        "{\"formatVersion\":1,\"serialNumber\":\"0001\",\"voided\":false,"
        "\"nested\":{\"a\":[1,2,3],\"b\":null},\"text\":\"line\\nbreak \\u00e9 \\ud83d\\ude00\","
        "\"number\":-2.5e3}";
    value_c o_root;
    std::string s_error;
    EXPECT_TRUE(value_c::parse(s_pass, o_root, s_error));
    EXPECT_TRUE(o_root.is_object());
    EXPECT_EQ(1.0, o_root.number_or("formatVersion"));
    EXPECT_EQ(std::string("0001"), o_root.string_or("serialNumber"));
    EXPECT_FALSE(o_root.boolean_or("voided", true));
    EXPECT_EQ(std::string("fallback"), o_root.string_or("absent", "fallback"));
    EXPECT_EQ(std::string("1"), o_root.string_or("formatVersion"));
    EXPECT_EQ(std::string("false"), o_root.string_or("voided"));

    const value_c *p_nested = o_root.member("nested");
    EXPECT_TRUE(nullptr != p_nested);
    EXPECT_TRUE(nullptr != p_nested->member("a"));
    EXPECT_EQ(static_cast<std::size_t>(3), p_nested->member("a")->items().size());
    EXPECT_EQ(2.0, p_nested->member("a")->items()[1].as_number());
    EXPECT_TRUE(p_nested->member("b")->is_null());
    EXPECT_TRUE(nullptr == o_root.member("b"));

    // Escapes, a Unicode code point, and a surrogate pair that must become one
    // code point (emoji, four UTF-8 bytes).
    const std::string s_text = o_root.string_or("text");
    EXPECT_CONTAINS(s_text, "line\nbreak");
    EXPECT_CONTAINS(s_text, "\xc3\xa9");
    EXPECT_CONTAINS(s_text, "\xf0\x9f\x98\x80");
    EXPECT_EQ(-2500.0, o_root.number_or("number"));

    // Whole numbers must print back without a decimal point, because
    // `formatVersion` and `serialNumber` are read as numbers by some issuers.
    EXPECT_CONTAINS(o_root.to_text(), "\"formatVersion\": 1");
    EXPECT_NOT_CONTAINS(o_root.to_text(), "1.0");

    // An array at the top level, and the empty containers.
    value_c o_array;
    EXPECT_TRUE(value_c::parse("[ \"a\", 2, true, null ]", o_array, s_error));
    EXPECT_TRUE(o_array.is_array());
    EXPECT_EQ(static_cast<std::size_t>(4), o_array.items().size());
    EXPECT_EQ(std::string("a"), o_array.items()[0].as_string());

    value_c o_empty_object;
    EXPECT_TRUE(value_c::parse("{}", o_empty_object, s_error));
    EXPECT_TRUE(o_empty_object.is_object());
    EXPECT_TRUE(o_empty_object.members().empty());

    value_c o_empty_array;
    EXPECT_TRUE(value_c::parse("[]", o_empty_array, s_error));
    EXPECT_TRUE(o_empty_array.items().empty());

    // Malformed input is refused, not guessed at, and the reason names where.
    const char *const o_malformed[] = {"{",      "{\"a\":}", "[1,]", "\"unterminated",
                                       "truex",  "{\"a\":1}{\"b\":2}", "", "nul"};
    for (const char *s_bad : o_malformed) {
        value_c o_value;
        std::string s_reason;
        EXPECT_FALSE(value_c::parse(s_bad, o_value, s_reason));
        EXPECT_FALSE(s_reason.empty());
    }

    // Round-tripping a document keeps member order, so a printed pass reads the
    // way the issuer wrote it.
    const std::string s_ordered = "{\"z\":1,\"a\":2,\"m\":3}";
    value_c o_ordered;
    EXPECT_TRUE(value_c::parse(s_ordered, o_ordered, s_error));
    const std::string s_printed = o_ordered.to_text();
    EXPECT_TRUE(s_printed.find("\"z\"") < s_printed.find("\"a\""));
    EXPECT_TRUE(s_printed.find("\"a\"") < s_printed.find("\"m\""));

    return pkpass_test::finish("json-reader-test");
}
