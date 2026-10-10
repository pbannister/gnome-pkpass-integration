//
// handler_integrator_test.cpp: the text this project hands to the desktop.
//
// The filesystem side of an install is exercised end to end by
// tests/90-handler-mime-definition.sh in an XDG sandbox; this test covers the
// pure text, which is what the desktop actually parses.
//
#include "integration/handler_integrator.h"

#include "test_harness.h"

int main() {
    using gnome_pkpass::integration::exec_quote;
    using gnome_pkpass::integration::handler_desktop_text;
    using gnome_pkpass::integration::handler_integrator_c;
    using gnome_pkpass::integration::mime_definition_xml;

    // A path without a reserved character is passed through; one with a space is
    // quoted, because the desktop splits `Exec=` on spaces.
    EXPECT_EQ(std::string("/usr/local/bin/pkpass-integrate"), exec_quote("/usr/local/bin/pkpass-integrate"));
    EXPECT_EQ(std::string("\"/opt/tool dir/pkpass-integrate\""), exec_quote("/opt/tool dir/pkpass-integrate"));
    EXPECT_EQ(std::string("\"/opt/quote\\\"d/x\""), exec_quote("/opt/quote\"d/x"));
    EXPECT_EQ(std::string("\"/opt/dollar\\$x\""), exec_quote("/opt/dollar$x"));

    const std::string s_desktop = handler_desktop_text("/usr/local/bin/pkpass-integrate", "v1");
    EXPECT_CONTAINS(s_desktop, "[Desktop Entry]");
    EXPECT_CONTAINS(s_desktop, "Type=Application");
    EXPECT_CONTAINS(s_desktop, "Name=PKPASS Viewer");
    EXPECT_CONTAINS(s_desktop, "Exec=/usr/local/bin/pkpass-integrate handle %f");
    EXPECT_CONTAINS(s_desktop, "Icon=pkpass-handler");
    EXPECT_CONTAINS(s_desktop, "StartupWMClass=pkpass-viewer");
    EXPECT_CONTAINS(s_desktop, "NoDisplay=true");
    EXPECT_CONTAINS(s_desktop,
                    "MimeType=application/vnd.apple.pkpass;application/vnd.apple.pkpasses;");
    EXPECT_CONTAINS(s_desktop, "X-Integrated-By=gnome-pkpass-integration");
    EXPECT_CONTAINS(s_desktop, "X-Integrated-At=v1");

    // A tool path with a space is quoted in the entry.
    const std::string s_spaced = handler_desktop_text("/opt/tool dir/pkpass-integrate", "v1");
    EXPECT_CONTAINS(s_spaced, "Exec=\"/opt/tool dir/pkpass-integrate\" handle %f");

    const std::string s_xml = mime_definition_xml();
    EXPECT_CONTAINS(s_xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    EXPECT_CONTAINS(s_xml, "xmlns=\"http://www.freedesktop.org/standards/shared-mime-info\"");
    EXPECT_CONTAINS(s_xml, "<mime-type type=\"application/vnd.apple.pkpass\">");
    EXPECT_CONTAINS(s_xml, "<mime-type type=\"application/vnd.apple.pkpasses\">");
    EXPECT_CONTAINS(s_xml, "<glob pattern=\"*.pkpass\"/>");
    EXPECT_CONTAINS(s_xml, "<glob pattern=\"*.pkpasses\"/>");
    EXPECT_CONTAINS(s_xml, "<generic-icon name=\"pkpass-handler\"/>");
    EXPECT_CONTAINS(s_xml, "<sub-class-of type=\"application/zip\"/>");
    EXPECT_CONTAINS(s_xml, "PK\\003\\004");
    EXPECT_CONTAINS(s_xml, "pass.json");
    // Every mime-type element is closed: a MIME definition is XML, and
    // update-mime-database refuses a malformed one.
    EXPECT_CONTAINS(s_xml, "</mime-type>");
    EXPECT_CONTAINS(s_xml, "</mime-info>");

    // The resolved paths are the ones the desktop reads; a test asserts on the
    // absolute names under an XDG sandbox, so only the suffixes belong here.
    gnome_pkpass::integration::handler_integrator_c o_integrator =
        handler_integrator_c::from_environment("/tmp/pkpass-integrate");
    const gnome_pkpass::integration::paths_o &o_paths = o_integrator.paths();
    EXPECT_TRUE(0 < o_paths.s_applications_directory.size());
    const std::string s_suffix_entry = "/applications/pkpass-handler.desktop";
    EXPECT_EQ(s_suffix_entry,
              o_paths.s_handler_desktop.substr(o_paths.s_handler_desktop.size() -
                                               s_suffix_entry.size()));
    const std::string s_suffix_manifest = "/gnome-pkpass-integration/pkpass-handler.manifest";
    EXPECT_EQ(s_suffix_manifest,
              o_paths.s_handler_manifest.substr(o_paths.s_handler_manifest.size() -
                                                s_suffix_manifest.size()));
    const std::string s_suffix_mime = "/mime/packages/pkpass.xml";
    EXPECT_EQ(s_suffix_mime,
              o_paths.s_mime_package.substr(o_paths.s_mime_package.size() - s_suffix_mime.size()));

    return pkpass_test::finish("handler-integrator-test");
}
