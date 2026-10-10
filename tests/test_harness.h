#pragma once

//
// test_harness.h: the tiny check-and-count harness the C++ tests share.
//
// The shell tests in tests/*.sh build and run these programs; a program exits
// nonzero when any check failed, which is what the shell test reports.
//
#include <iostream>
#include <string>

namespace pkpass_test {

inline int &failures() {
    static int i_failures = 0;
    return i_failures;
}

inline void fail(const char *s_file, int i_line, const std::string &s_message) {
    std::cerr << s_file << ':' << i_line << ": " << s_message << '\n';
    ++failures();
}

inline void expect_true(const char *s_file, int i_line, const char *s_expression, bool b_value) {
    if (!b_value) {
        fail(s_file, i_line, std::string("expected true: ") + s_expression);
    }
}

inline void expect_false(const char *s_file, int i_line, const char *s_expression, bool b_value) {
    if (b_value) {
        fail(s_file, i_line, std::string("expected false: ") + s_expression);
    }
}

// Compare two values that print themselves.  The expressions are named in the
// failure so a failing check says what was compared.
template <typename type_expected, typename type_actual>
inline void expect_equal(const char *s_file, int i_line, const char *s_expression,
                         const type_expected &o_expected, const type_actual &o_actual) {
    if (!(o_expected == o_actual)) {
        fail(s_file, i_line, std::string("expected ") + s_expression);
    }
}

inline void expect_contains(const char *s_file, int i_line, const char *s_expression,
                            const std::string &s_haystack, const std::string &s_needle) {
    if (std::string::npos == s_haystack.find(s_needle)) {
        fail(s_file, i_line,
             std::string("expected ") + s_expression + " to contain \"" + s_needle + "\"");
    }
}

inline void expect_not_contains(const char *s_file, int i_line, const char *s_expression,
                                const std::string &s_haystack, const std::string &s_needle) {
    if (std::string::npos != s_haystack.find(s_needle)) {
        fail(s_file, i_line,
             std::string("expected ") + s_expression + " not to contain \"" + s_needle + "\"");
    }
}

inline int finish(const char *s_name) {
    if (0 == failures()) {
        std::cout << s_name << ": ok\n";
        return 0;
    }
    std::cerr << s_name << ": " << failures() << " failure(s)\n";
    return 1;
}

}  // namespace pkpass_test

#define EXPECT_TRUE(expression) \
    ::pkpass_test::expect_true(__FILE__, __LINE__, #expression, (expression))
#define EXPECT_FALSE(expression) \
    ::pkpass_test::expect_false(__FILE__, __LINE__, #expression, (expression))
#define EXPECT_EQ(expected, actual)                                                     \
    ::pkpass_test::expect_equal(__FILE__, __LINE__, #expected " == " #actual, (expected), \
                                (actual))
#define EXPECT_CONTAINS(haystack, needle) \
    ::pkpass_test::expect_contains(__FILE__, __LINE__, #haystack, (haystack), (needle))
#define EXPECT_NOT_CONTAINS(haystack, needle) \
    ::pkpass_test::expect_not_contains(__FILE__, __LINE__, #haystack, (haystack), (needle))
