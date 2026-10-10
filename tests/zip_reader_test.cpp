//
// zip_reader_test.cpp: the ZIP reader over a real pkpass container.
//
// Usage: zip-reader-test FILE.pkpass
//
// The container is built by tests/30-zip-reader.sh, which stores the PNG members
// and deflates the JSON members: the two methods the format actually uses, and
// the reason the reader honours the per-entry method instead of assuming one.
//
#include "util/file_io.h"
#include "zip/zip_reader.h"

#include "test_harness.h"

#include <cstdio>

int main(int i_argument_count, char *o_arguments[]) {
    using gnome_pkpass::zip::archive_c;

    if (2 > i_argument_count) {
        std::fprintf(stderr, "usage: zip-reader-test FILE.pkpass\n");
        return 2;
    }
    const std::string s_path = o_arguments[1];

    archive_c o_archive;
    std::string s_error;
    EXPECT_TRUE(o_archive.load_file(s_path, s_error));
    EXPECT_TRUE(s_error.empty() || !o_archive.entries().empty());

    EXPECT_TRUE(o_archive.contains("pass.json"));
    EXPECT_TRUE(o_archive.contains("manifest.json"));
    EXPECT_TRUE(o_archive.contains("icon.png"));
    EXPECT_FALSE(o_archive.contains("missing.png"));
    EXPECT_TRUE(nullptr == o_archive.find("missing.png"));

    const gnome_pkpass::zip::entry_o *p_pass = o_archive.find("pass.json");
    const gnome_pkpass::zip::entry_o *p_icon = o_archive.find("icon.png");
    EXPECT_TRUE(nullptr != p_pass);
    EXPECT_TRUE(nullptr != p_icon);
    if (nullptr != p_pass && nullptr != p_icon) {
        // 8 is deflate, 0 is stored; the fixture mixes them on purpose.
        EXPECT_EQ(static_cast<std::uint16_t>(8), p_pass->u_method);
        EXPECT_EQ(static_cast<std::uint16_t>(0), p_icon->u_method);
        EXPECT_TRUE(0 < p_pass->u_size_uncompressed);
    }

    std::string s_pass_json;
    EXPECT_TRUE(o_archive.extract("pass.json", s_pass_json, s_error));
    EXPECT_TRUE(0 < s_pass_json.size());
    EXPECT_EQ('{', s_pass_json[0]);

    std::string s_icon;
    EXPECT_TRUE(o_archive.extract("icon.png", s_icon, s_error));
    EXPECT_EQ(static_cast<std::size_t>(100), s_icon.size());
    EXPECT_EQ(std::string("\x89PNG\r\n\x1a\n", 8), s_icon.substr(0, 8));

    // An absent member is refused rather than returning empty bytes.
    std::string s_missing;
    std::string s_missing_error;
    EXPECT_FALSE(o_archive.extract("missing.png", s_missing, s_missing_error));
    EXPECT_FALSE(s_missing_error.empty());

    // Not-a-ZIP input fails with a reason.
    archive_c o_bad;
    std::string s_bad_error;
    EXPECT_FALSE(o_bad.load_bytes("this is not a ZIP archive at all", s_bad_error));
    EXPECT_FALSE(s_bad_error.empty());

    // The same bytes load through the in-memory path.
    std::string s_data;
    EXPECT_TRUE(gnome_pkpass::util::read_file(s_path, s_data, s_error));
    archive_c o_memory;
    EXPECT_TRUE(o_memory.load_bytes(s_data, s_error));
    EXPECT_EQ(o_archive.entries().size(), o_memory.entries().size());

    return pkpass_test::finish("zip-reader-test");
}
