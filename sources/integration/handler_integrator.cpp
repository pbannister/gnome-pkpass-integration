#include "integration/handler_integrator.h"

#include "util/file_io.h"
#include "util/process.h"
#include "version/version.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>

namespace gnome_pkpass::integration {
namespace {

namespace fs = std::filesystem;

std::string environment_value(const char *s_name) {
    const char *s_value = ::getenv(s_name);
    return (nullptr == s_value) ? std::string() : std::string(s_value);
}

std::string trim(const std::string &s_text) {
    std::size_t u_begin = 0;
    std::size_t u_end = s_text.size();
    while (u_begin < u_end && (' ' == s_text[u_begin] || '\t' == s_text[u_begin] ||
                               '\n' == s_text[u_begin] || '\r' == s_text[u_begin])) {
        ++u_begin;
    }
    while (u_end > u_begin && (' ' == s_text[u_end - 1] || '\t' == s_text[u_end - 1] ||
                               '\n' == s_text[u_end - 1] || '\r' == s_text[u_end - 1])) {
        --u_end;
    }
    return s_text.substr(u_begin, u_end - u_begin);
}

// Run a helper program and return its first line of output, trimmed.  A helper
// that is absent, or that fails, yields an empty string: the callers treat that
// as "the desktop did not answer", never as "the default is empty".
std::string query_tool(const std::vector<std::string> &o_arguments) {
    std::string s_output;
    int i_status = 0;
    bool b_exited = false;
    std::string s_error;
    if (!util::run_command(o_arguments, s_output, i_status, b_exited, s_error)) {
        return std::string();
    }
    if (!b_exited || 0 != i_status) {
        return std::string();
    }
    const std::size_t u_break = s_output.find('\n');
    return trim((std::string::npos == u_break) ? s_output : s_output.substr(0, u_break));
}

bool run_helper(const std::vector<std::string> &o_arguments, std::string &s_error) {
    std::string s_output;
    int i_status = 0;
    bool b_exited = false;
    if (!util::run_command(o_arguments, s_output, i_status, b_exited, s_error)) {
        return false;
    }
    if (!b_exited || 0 != i_status) {
        s_error = o_arguments.front() + " failed";
        return false;
    }
    return true;
}

// Refresh the GTK icon cache.  GTK trusts a cache that is not older than the
// theme directory and then never rescans it, so a stale cache hides every newly
// installed icon; a missing cache tool is not an error.
void refresh_icon_cache(const std::string &s_theme_directory) {
    if (!fs::is_directory(s_theme_directory)) {
        return;
    }
    for (const char *s_program : {"gtk4-update-icon-cache", "gtk-update-icon-cache"}) {
        if (!util::command_exists(s_program)) {
            continue;
        }
        std::string s_error;
        run_helper({s_program, "-f", "-t", s_theme_directory}, s_error);
        return;
    }
}

std::vector<std::string> handled_types() {
    return {MIME_TYPE_PASS, MIME_TYPE_PASSES};
}

// Create a directory and every parent, reporting a failure instead of throwing.
bool ensure_directory(const std::string &s_path, std::string &s_error) {
    std::error_code o_error;
    fs::create_directories(s_path, o_error);
    if (o_error && !fs::is_directory(s_path)) {
        s_error = "cannot create " + s_path + ": " + o_error.message();
        return false;
    }
    return true;
}

// The value recorded for one MIME type in a `previous_default=<type>\t<value>`
// line.
std::string previous_default_of(
    const std::vector<std::pair<std::string, std::string>> &o_entries,
    const std::string &s_type) {
    for (const std::pair<std::string, std::string> &o_entry : o_entries) {
        if ("previous_default" != o_entry.first) {
            continue;
        }
        const std::size_t u_tab = o_entry.second.find('\t');
        if (std::string::npos == u_tab) {
            continue;
        }
        if (o_entry.second.substr(0, u_tab) == s_type) {
            return o_entry.second.substr(u_tab + 1);
        }
    }
    return std::string();
}

}  // namespace

std::string exec_quote(const std::string &s_path) {
    bool b_needs_quotes = s_path.empty();
    for (const char c_next : s_path) {
        if (' ' == c_next || '\t' == c_next || '"' == c_next || '\'' == c_next ||
            '\\' == c_next || '$' == c_next || '`' == c_next) {
            b_needs_quotes = true;
            break;
        }
    }
    if (!b_needs_quotes) {
        return s_path;
    }

    std::string s_quoted = "\"";
    for (const char c_next : s_path) {
        if ('"' == c_next || '\\' == c_next || '$' == c_next || '`' == c_next) {
            s_quoted.push_back('\\');
        }
        s_quoted.push_back(c_next);
    }
    s_quoted.push_back('"');
    return s_quoted;
}

std::string mime_definition_xml() {
    return R"(<?xml version="1.0" encoding="UTF-8"?>

<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">

  <mime-type type="application/vnd.apple.pkpass">
    <comment>Apple Wallet pass</comment>
    <sub-class-of type="application/zip"/>
    <generic-icon name="pkpass-handler"/>
    <magic priority="65">
      <match type="string" value="PK\003\004" offset="0">
        <match type="string" value="pass.json" offset="30"/>
      </match>
    </magic>
    <glob pattern="*.pkpass"/>
  </mime-type>

  <mime-type type="application/vnd.apple.pkpasses">
    <comment>Apple Wallet passes bundle</comment>
    <sub-class-of type="application/zip"/>
    <generic-icon name="pkpass-handler"/>
    <magic priority="65">
      <match type="string" value="PK\003\004" offset="0">
        <match type="string" value=".pkpass" offset="30:256"/>
      </match>
    </magic>
    <glob pattern="*.pkpasses"/>
  </mime-type>

</mime-info>
)";
}

std::string handler_desktop_text(const std::string &s_tool_path, const std::string &s_version) {
    std::ostringstream s_text;
    s_text << "[Desktop Entry]\n"
           << "Type=Application\n"
           << "Name=" << HANDLER_NAME << "\n"
           << "Comment=Inspect and display an Apple Wallet pass\n"
           << "Exec=" << exec_quote(s_tool_path) << " handle %f\n"
           << "Icon=" << HANDLER_ICON_NAME << "\n"
           << "StartupWMClass=" << HANDLER_WM_CLASS << "\n"
           << "Terminal=false\n"
           << "NoDisplay=true\n"
           << "MimeType=" << MIME_TYPE_PASS << ';' << MIME_TYPE_PASSES << ";\n"
           << "X-Integrated-By=" << STATE_DIRECTORY_NAME << "\n"
           << "X-Integrated-At=" << s_version << "\n";
    return s_text.str();
}

handler_integrator_c handler_integrator_c::from_environment(const std::string &s_tool_path) {
    handler_integrator_c o_integrator;

    const std::string s_home = environment_value("HOME");
    const std::string s_data_home = environment_value("XDG_DATA_HOME").empty()
                                        ? s_home + "/.local/share"
                                        : environment_value("XDG_DATA_HOME");
    const std::string s_config_home = environment_value("XDG_CONFIG_HOME").empty()
                                          ? s_home + "/.config"
                                          : environment_value("XDG_CONFIG_HOME");

    paths_o &o_paths = o_integrator.o_paths_;
    o_paths.s_home = s_home;
    o_paths.s_data_home = s_data_home;
    o_paths.s_config_home = s_config_home;
    o_paths.s_state_directory = s_data_home + "/" + STATE_DIRECTORY_NAME;
    o_paths.s_applications_directory = s_data_home + "/applications";
    o_paths.s_mime_directory = s_data_home + "/mime";
    o_paths.s_mime_packages = o_paths.s_mime_directory + "/packages";
    o_paths.s_icons_directory = s_data_home + "/icons/hicolor";
    o_paths.s_handler_desktop = o_paths.s_applications_directory + "/" + HANDLER_DESKTOP_ID;
    o_paths.s_handler_manifest = o_paths.s_state_directory + "/" + HANDLER_MANIFEST;
    o_paths.s_mime_package = o_paths.s_mime_packages + "/" + MIME_PACKAGE_NAME;
    o_paths.s_mime_backup = o_paths.s_state_directory + "/backup/" + MIME_PACKAGE_NAME;

    o_integrator.s_tool_path_ = s_tool_path;
    return o_integrator;
}

std::string handler_integrator_c::tool_path() const {
    if (!s_tool_path_.empty()) {
        return s_tool_path_;
    }
    // The path the desktop entry must run: the installed copy when there is one,
    // otherwise this very program, resolved through /proc.
    const std::string s_installed = o_paths_.s_home + "/.local/bin/pkpass-integrate";
    if (util::file_exists(s_installed)) {
        return s_installed;
    }
    char o_buffer[4096];
    const ssize_t i_length = ::readlink("/proc/self/exe", o_buffer, sizeof(o_buffer) - 1);
    if (0 < i_length) {
        o_buffer[i_length] = '\0';
        return std::string(o_buffer);
    }
    return "pkpass-integrate";
}

std::string handler_integrator_c::icon_source() const {
    const std::string s_tool = tool_path();
    const fs::path o_tool(s_tool);
    const std::string s_beside_tool = o_tool.parent_path().string();

    std::vector<std::string> o_candidates;
    o_candidates.push_back(s_beside_tool + "/icons/" + HANDLER_ICON_NAME + ".svg");
    o_candidates.push_back(s_beside_tool + "/" + HANDLER_ICON_NAME + ".svg");
    const fs::path o_prefix = o_tool.parent_path().parent_path();
    o_candidates.push_back(o_prefix.string() + "/share/icons/hicolor/scalable/apps/" +
                           HANDLER_ICON_NAME + ".svg");

    for (const std::string &s_candidate : o_candidates) {
        if (util::file_exists(s_candidate)) {
            return s_candidate;
        }
    }
    return std::string();
}

const paths_o &handler_integrator_c::paths() const {
    return o_paths_;
}

bool handler_integrator_c::read_manifest(
    std::vector<std::pair<std::string, std::string>> &o_entries) const {
    o_entries.clear();
    std::string s_text;
    std::string s_error;
    if (!util::read_file(o_paths_.s_handler_manifest, s_text, s_error)) {
        return false;
    }

    std::istringstream s_stream(s_text);
    std::string s_line;
    while (std::getline(s_stream, s_line)) {
        if (s_line.empty()) {
            continue;
        }
        const std::size_t u_equals = s_line.find('=');
        if (std::string::npos == u_equals) {
            continue;
        }
        o_entries.emplace_back(s_line.substr(0, u_equals), s_line.substr(u_equals + 1));
    }
    return true;
}

std::string handler_integrator_c::manifest_value(
    const std::vector<std::pair<std::string, std::string>> &o_entries,
    const std::string &s_key) const {
    for (const std::pair<std::string, std::string> &o_entry : o_entries) {
        if (o_entry.first == s_key) {
            return o_entry.second;
        }
    }
    return std::string();
}

bool handler_integrator_c::has_record() const {
    return util::file_exists(o_paths_.s_handler_manifest);
}

bool handler_integrator_c::status(status_o &o_status, std::string &s_error) const {
    o_status = status_o();

    o_status.s_desktop_path = o_paths_.s_handler_desktop;
    o_status.b_desktop_present = util::file_exists(o_paths_.s_handler_desktop);
    o_status.b_mime_package_present = util::file_exists(o_paths_.s_mime_package);
    o_status.b_pass_icon_present =
        util::file_exists(o_paths_.s_icons_directory + "/scalable/mimetypes/application-vnd.apple.pkpass.svg");
    o_status.b_passes_icon_present =
        util::file_exists(o_paths_.s_icons_directory + "/scalable/mimetypes/application-vnd.apple.pkpasses.svg");
    o_status.b_manifest_present = util::file_exists(o_paths_.s_handler_manifest);

    for (const std::string &s_type : handled_types()) {
        std::string s_default;
        if (util::command_exists("xdg-mime")) {
            s_default = query_tool({"xdg-mime", "query", "default", s_type});
        }
        o_status.o_defaults.emplace_back(s_type, s_default);
    }

    if (o_status.b_manifest_present) {
        std::vector<std::pair<std::string, std::string>> o_entries;
        read_manifest(o_entries);
        o_status.b_mime_package_ours =
            (manifest_value(o_entries, "mime_package") == o_paths_.s_mime_package);
        o_status.b_mime_package_created_by_us =
            ("1" == manifest_value(o_entries, "mime_created"));
    }

    s_error.clear();
    return true;
}

bool handler_integrator_c::install(std::vector<std::string> &o_log, std::string &s_error) {
    o_log.clear();

    if (!util::command_exists("xdg-mime")) {
        s_error = "xdg-mime is required to register a MIME default; install xdg-utils and retry";
        return false;
    }

    const std::string s_version = version::version_string();
    const std::string s_tool = tool_path();

    std::vector<std::pair<std::string, std::string>> o_old_entries;
    read_manifest(o_old_entries);

    if (!ensure_directory(o_paths_.s_applications_directory, s_error) ||
        !ensure_directory(o_paths_.s_mime_packages, s_error) ||
        !ensure_directory(o_paths_.s_state_directory, s_error) ||
        !ensure_directory(o_paths_.s_icons_directory + "/scalable/apps", s_error) ||
        !ensure_directory(o_paths_.s_icons_directory + "/scalable/mimetypes", s_error)) {
        return false;
    }

    // Remember what opened each type before this tool did, so uninstall can hand
    // it back.  A previous run's recorded value wins over this tool's own ID, so
    // reinstalling does not lose the real previous handler.
    std::vector<std::pair<std::string, std::string>> o_previous;
    for (const std::string &s_type : handled_types()) {
        std::string s_previous = query_tool({"xdg-mime", "query", "default", s_type});
        if (HANDLER_DESKTOP_ID == s_previous) {
            const std::string s_recorded = previous_default_of(o_old_entries, s_type);
            if (!s_recorded.empty()) {
                s_previous = s_recorded;
            }
        }
        o_previous.emplace_back(s_type, s_previous);
    }

    // The MIME definition.  This project installs its own definition at the user
    // level even where shared-mime-info already knows the type, because that is
    // the only way `generic-icon` can name this project's icon; a definition the
    // project did not write is backed up first and restored by uninstall.
    const bool b_existed = fs::exists(o_paths_.s_mime_package);
    const bool b_ours_before =
        (manifest_value(o_old_entries, "mime_package") == o_paths_.s_mime_package);
    const bool b_old_created = ("1" == manifest_value(o_old_entries, "mime_created"));

    std::string s_backup;
    if (fs::exists(o_paths_.s_mime_backup)) {
        s_backup = o_paths_.s_mime_backup;
    } else if (b_existed && !b_ours_before) {
        if (!ensure_directory(o_paths_.s_state_directory + "/backup", s_error)) {
            return false;
        }
        std::error_code o_copy_error;
        fs::copy_file(o_paths_.s_mime_package, o_paths_.s_mime_backup,
                      fs::copy_options::overwrite_existing, o_copy_error);
        if (o_copy_error) {
            s_error = "cannot back up " + o_paths_.s_mime_package;
            return false;
        }
        s_backup = o_paths_.s_mime_backup;
        o_log.push_back("backed up the MIME definition to " + s_backup);
    }

    const bool b_created = s_backup.empty() && (!b_existed || b_ours_before || b_old_created);

    if (!util::write_file(o_paths_.s_mime_package, mime_definition_xml(), s_error)) {
        return false;
    }
    o_log.push_back("wrote the MIME definition " + o_paths_.s_mime_package);
    if (util::command_exists("update-mime-database")) {
        std::string s_helper_error;
        run_helper({"update-mime-database", o_paths_.s_mime_directory}, s_helper_error);
        o_log.push_back("updated the MIME database " + o_paths_.s_mime_directory);
    }

    // The desktop entry that opens a pass.
    if (!util::write_file(o_paths_.s_handler_desktop, handler_desktop_text(s_tool, s_version),
                          s_error)) {
        return false;
    }
    o_log.push_back("wrote the handler entry " + o_paths_.s_handler_desktop);

    // The icon, both for the handler entry and for the two file types.  A file
    // icon is resolved from the MIME type's own name, so the type icons are
    // copies under the name GIO looks for.
    std::vector<std::string> o_icons_written;
    const std::string s_source = icon_source();
    if (s_source.empty()) {
        o_log.push_back("no icon source was found; the entry keeps a generic icon");
    } else {
        std::string s_icon_data;
        if (!util::read_file(s_source, s_icon_data, s_error)) {
            return false;
        }
        const std::string s_app_icon =
            o_paths_.s_icons_directory + "/scalable/apps/" + HANDLER_ICON_NAME + ".svg";
        if (!util::write_file(s_app_icon, s_icon_data, s_error)) {
            return false;
        }
        o_icons_written.push_back(s_app_icon);

        for (const std::string &s_type : handled_types()) {
            std::string s_type_name = s_type;
            std::replace(s_type_name.begin(), s_type_name.end(), '/', '-');
            const std::string s_type_icon =
                o_paths_.s_icons_directory + "/scalable/mimetypes/" + s_type_name + ".svg";
            if (!util::write_file(s_type_icon, s_icon_data, s_error)) {
                return false;
            }
            o_icons_written.push_back(s_type_icon);
        }
        o_log.push_back("installed " + std::to_string(o_icons_written.size()) + " icon(s)");
        refresh_icon_cache(o_paths_.s_icons_directory);
    }

    // Make this tool the default for both types.
    std::vector<std::string> o_command = {"xdg-mime", "default", HANDLER_DESKTOP_ID};
    for (const std::string &s_type : handled_types()) {
        o_command.push_back(s_type);
    }
    std::string s_output;
    int i_status = 0;
    bool b_exited = false;
    std::string s_run_error;
    if (!util::run_command(o_command, s_output, i_status, b_exited, s_run_error) || !b_exited ||
        0 != i_status) {
        s_error = "xdg-mime default failed" +
                  (s_run_error.empty() ? std::string() : ": " + s_run_error);
        return false;
    }
    for (const std::string &s_type : handled_types()) {
        o_log.push_back("set " + std::string(HANDLER_DESKTOP_ID) + " as the default for " + s_type);
    }

    if (util::command_exists("update-desktop-database")) {
        std::string s_helper_error;
        run_helper({"update-desktop-database", o_paths_.s_applications_directory}, s_helper_error);
        o_log.push_back("updated the desktop database " + o_paths_.s_applications_directory);
    }

    // The record of what this install owns.
    std::ostringstream s_manifest;
    s_manifest << "handler_desktop=" << o_paths_.s_handler_desktop << "\n";
    for (const std::string &s_icon : o_icons_written) {
        s_manifest << "icon=" << s_icon << "\n";
    }
    s_manifest << "mime_package=" << o_paths_.s_mime_package << "\n";
    s_manifest << "mime_backup=" << s_backup << "\n";
    if (b_created) {
        s_manifest << "mime_created=1\n";
    }
    for (const std::pair<std::string, std::string> &o_entry : o_previous) {
        s_manifest << "previous_default=" << o_entry.first << '\t' << o_entry.second << "\n";
    }
    if (!util::write_file(o_paths_.s_handler_manifest, s_manifest.str(), s_error)) {
        return false;
    }
    o_log.push_back("wrote the record " + o_paths_.s_handler_manifest);

    return true;
}

bool handler_integrator_c::uninstall(std::vector<std::string> &o_log, std::string &s_error) {
    o_log.clear();

    std::vector<std::pair<std::string, std::string>> o_entries;
    if (!read_manifest(o_entries)) {
        s_error = "there is no record to undo: " + o_paths_.s_handler_manifest;
        return false;
    }

    // Hand each type back to whatever opened it before.
    if (util::command_exists("xdg-mime")) {
        for (const std::string &s_type : handled_types()) {
            const std::string s_previous = previous_default_of(o_entries, s_type);
            if (s_previous.empty()) {
                continue;
            }
            std::string s_output;
            int i_status = 0;
            bool b_exited = false;
            std::string s_run_error;
            util::run_command({"xdg-mime", "default", s_previous, s_type}, s_output, i_status,
                              b_exited, s_run_error);
            o_log.push_back("restored " + s_type + " to " + s_previous);
        }
    }

    // Remove the recorded icons, and the well-known names even when the record is
    // from an older run that named fewer.
    std::vector<std::string> o_icons;
    for (const std::pair<std::string, std::string> &o_entry : o_entries) {
        if ("icon" == o_entry.first && !o_entry.second.empty()) {
            o_icons.push_back(o_entry.second);
        }
    }
    o_icons.push_back(o_paths_.s_icons_directory + "/scalable/apps/" + HANDLER_ICON_NAME + ".svg");
    for (const std::string &s_type : handled_types()) {
        std::string s_type_name = s_type;
        std::replace(s_type_name.begin(), s_type_name.end(), '/', '-');
        o_icons.push_back(o_paths_.s_icons_directory + "/scalable/mimetypes/" + s_type_name + ".svg");
    }
    for (const std::string &s_icon : o_icons) {
        std::error_code o_remove_error;
        if (fs::remove(s_icon, o_remove_error) && !o_remove_error) {
            o_log.push_back("removed " + s_icon);
        }
    }
    refresh_icon_cache(o_paths_.s_icons_directory);

    // The MIME definition: restore a displaced one, otherwise remove our own.
    const std::string s_backup = manifest_value(o_entries, "mime_backup");
    const bool b_created = ("1" == manifest_value(o_entries, "mime_created"));
    bool b_mime_changed = false;
    if (!s_backup.empty() && util::file_exists(s_backup)) {
        std::error_code o_copy_error;
        fs::copy_file(s_backup, o_paths_.s_mime_package, fs::copy_options::overwrite_existing,
                      o_copy_error);
        if (o_copy_error) {
            s_error = "cannot restore " + o_paths_.s_mime_package + " from " + s_backup;
            return false;
        }
        o_log.push_back("restored the MIME definition from " + s_backup);
        b_mime_changed = true;
    } else if (b_created) {
        std::error_code o_remove_error;
        if (fs::remove(o_paths_.s_mime_package, o_remove_error) && !o_remove_error) {
            o_log.push_back("removed the MIME definition " + o_paths_.s_mime_package);
            b_mime_changed = true;
        }
    }
    if (b_mime_changed && util::command_exists("update-mime-database")) {
        std::string s_helper_error;
        run_helper({"update-mime-database", o_paths_.s_mime_directory}, s_helper_error);
        o_log.push_back("updated the MIME database " + o_paths_.s_mime_directory);
    }

    std::error_code o_remove_error;
    if (fs::remove(o_paths_.s_handler_desktop, o_remove_error) && !o_remove_error) {
        o_log.push_back("removed the handler entry " + o_paths_.s_handler_desktop);
    }
    if (fs::remove(o_paths_.s_handler_manifest, o_remove_error) && !o_remove_error) {
        o_log.push_back("removed the record " + o_paths_.s_handler_manifest);
    }

    return true;
}

}  // namespace gnome_pkpass::integration
