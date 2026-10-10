//
// pkpass-integrate: manage the desktop handler for `.pkpass` files, and act as
// that handler.
//
// `handler install` registers `application/vnd.apple.pkpass` (and the
// `.pkpasses` bundle type) with this project, so double-clicking a pass opens
// the viewer and `*.pkpass` files carry this project's icon.  `handle` is what
// the desktop entry runs.
//
#include "integration/handler_integrator.h"
#include "util/file_io.h"
#include "util/process.h"
#include "version/version.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>

namespace {

namespace fs = std::filesystem;

constexpr int EXIT_OK = 0;
constexpr int EXIT_ERROR = 1;
constexpr int EXIT_USAGE = 2;

void print_usage(std::ostream &s_out) {
    s_out << "usage: pkpass-integrate handler status\n"
             "       pkpass-integrate handler install\n"
             "       pkpass-integrate handler uninstall\n"
             "       pkpass-integrate handle FILE.pkpass\n"
             "       pkpass-integrate --version\n"
             "\n"
             "  handler status     report whether this project owns the pkpass types\n"
             "  handler install    register the handler and the file-type icon\n"
             "  handler uninstall  restore the previous handler and remove the entry\n"
             "  handle             the desktop entry point: open a pass to view it\n";
}

// The directory this program was started from, which is where its sibling
// programs and its icon live.
std::string program_directory() {
    char o_buffer[4096];
    const ssize_t i_length = ::readlink("/proc/self/exe", o_buffer, sizeof(o_buffer) - 1);
    if (0 < i_length) {
        o_buffer[i_length] = '\0';
        return fs::path(o_buffer).parent_path().string();
    }
    return std::string();
}

std::string find_viewer() {
    const std::string s_directory = program_directory();
    if (!s_directory.empty()) {
        const std::string s_beside = s_directory + "/pkpass-viewer";
        if (gnome_pkpass::util::file_exists(s_beside)) {
            return s_beside;
        }
    }
    const char *s_path = ::getenv("PATH");
    if (nullptr == s_path) {
        return std::string();
    }
    std::string s_rest(s_path);
    while (!s_rest.empty()) {
        const std::size_t u_colon = s_rest.find(':');
        const std::string s_one = (std::string::npos == u_colon) ? s_rest : s_rest.substr(0, u_colon);
        if (!s_one.empty()) {
            const std::string s_candidate = s_one + "/pkpass-viewer";
            if (gnome_pkpass::util::file_exists(s_candidate)) {
                return s_candidate;
            }
        }
        if (std::string::npos == u_colon) {
            break;
        }
        s_rest = s_rest.substr(u_colon + 1);
    }
    return std::string();
}

bool has_display() {
    const char *s_display = ::getenv("DISPLAY");
    const char *s_wayland = ::getenv("WAYLAND_DISPLAY");
    return (nullptr != s_display && '\0' != s_display[0]) ||
           (nullptr != s_wayland && '\0' != s_wayland[0]);
}

// The desktop entry runs this with one file.  When a display and the graphical
// viewer are both available, become the viewer; otherwise print what the
// command line would do, so a double-click on a headless host still tells the
// person what happened and exits successfully.
int command_handle(const std::string &s_path) {
    if (s_path.empty()) {
        std::cerr << "pkpass-integrate: handle needs a file\n";
        return EXIT_USAGE;
    }
    if (!gnome_pkpass::util::file_exists(s_path)) {
        std::cerr << "pkpass-integrate: no such file: " << s_path << '\n';
        return EXIT_ERROR;
    }

    const std::string s_viewer = find_viewer();
    if (!s_viewer.empty() && has_display()) {
        const char *o_raw[] = {s_viewer.c_str(), s_path.c_str(), nullptr};
        ::execv(s_viewer.c_str(), const_cast<char *const *>(o_raw));
        // If exec fails the viewer was not runnable; fall through to the
        // command-line report rather than failing with nothing shown.
    }

    std::cout << "pass: " << s_path << '\n';
    std::cout << "  pkpass-inspect \"" << s_path << "\"\n";
    std::cout << "  pkpass-render \"" << s_path << "\"\n";
    if (s_viewer.empty()) {
        std::cout << "note: pkpass-viewer was not built or is not installed, so the pass was "
                     "not displayed\n";
    } else {
        std::cout << "note: no display is available, so the pass was not displayed\n";
    }
    return EXIT_OK;
}

void print_status(const gnome_pkpass::integration::handler_integrator_c &o_integrator) {
    gnome_pkpass::integration::status_o o_status;
    std::string s_error;
    if (!o_integrator.status(o_status, s_error)) {
        std::cerr << "pkpass-integrate: " << s_error << '\n';
        return;
    }

    for (const std::pair<std::string, std::string> &o_default : o_status.o_defaults) {
        std::cout << o_default.first << ": "
                  << (o_default.second.empty() ? "no default" : o_default.second) << '\n';
    }
    std::cout << "handler entry: " << o_status.s_desktop_path << " ("
              << (o_status.b_desktop_present ? "present" : "absent") << ")\n";
    std::cout << "mime definition: "
              << (o_status.b_mime_package_present
                      ? (o_status.b_mime_package_ours ? "present (installed by this project)"
                                                      : "present (not installed by this project)")
                      : "absent")
              << '\n';
    std::cout << "type icons: pkpass " << (o_status.b_pass_icon_present ? "present" : "absent")
              << ", pkpasses " << (o_status.b_passes_icon_present ? "present" : "absent") << '\n';
    std::cout << "record: " << (o_status.b_manifest_present ? "present" : "absent") << '\n';
}

int command_handler(const std::string &s_subcommand, bool &b_recognized) {
    using gnome_pkpass::integration::handler_integrator_c;

    b_recognized = true;
    gnome_pkpass::integration::handler_integrator_c o_integrator =
        gnome_pkpass::integration::handler_integrator_c::from_environment();

    if (s_subcommand.empty() || "status" == s_subcommand) {
        print_status(o_integrator);
        return EXIT_OK;
    }

    std::vector<std::string> o_log;
    std::string s_error;

    if ("install" == s_subcommand) {
        if (!o_integrator.install(o_log, s_error)) {
            std::cerr << "pkpass-integrate: " << s_error << '\n';
            return EXIT_ERROR;
        }
    } else if ("uninstall" == s_subcommand) {
        if (!o_integrator.uninstall(o_log, s_error)) {
            std::cerr << "pkpass-integrate: " << s_error << '\n';
            return EXIT_ERROR;
        }
    } else {
        b_recognized = false;
        return EXIT_USAGE;
    }

    for (const std::string &s_line : o_log) {
        std::cout << s_line << '\n';
    }
    print_status(o_integrator);
    return EXIT_OK;
}

}  // namespace

int main(int i_argument_count, char *o_arguments[]) {
    if (1 >= i_argument_count) {
        print_usage(std::cerr);
        return EXIT_USAGE;
    }

    const std::string s_command = o_arguments[1];
    if ("--help" == s_command || "-h" == s_command || "help" == s_command) {
        print_usage(std::cout);
        return EXIT_OK;
    }
    if ("--version" == s_command || "-V" == s_command) {
        std::cout << "pkpass-integrate " << gnome_pkpass::version::version_string() << " (build "
                  << gnome_pkpass::version::version_build_counter() << ")\n";
        return EXIT_OK;
    }

    if ("handler" == s_command) {
        const std::string s_subcommand = (2 < i_argument_count) ? o_arguments[2] : std::string();
        bool b_recognized = false;
        const int i_status = command_handler(s_subcommand, b_recognized);
        if (!b_recognized) {
            std::cerr << "pkpass-integrate: unknown handler subcommand: " << s_subcommand << '\n';
            print_usage(std::cerr);
            return EXIT_USAGE;
        }
        return i_status;
    }

    if ("handle" == s_command) {
        const std::string s_path = (2 < i_argument_count) ? o_arguments[2] : std::string();
        return command_handle(s_path);
    }

    std::cerr << "pkpass-integrate: unknown command: " << s_command << '\n';
    print_usage(std::cerr);
    return EXIT_USAGE;
}
