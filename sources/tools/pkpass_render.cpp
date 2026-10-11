//
// pkpass-render: draw a pass as an SVG card.
//
// The card is what the graphical viewer shows and what a file manager could use
// for a thumbnail; it is a plain SVG so it needs no image library.
//
#include "pkpass/pkpass_reader.h"
#include "qr/qr_encoder.h"
#include "render/pass_render.h"
#include "util/file_io.h"
#include "version/version.h"

#include <iostream>
#include <string>

namespace {

constexpr int EXIT_OK = 0;
constexpr int EXIT_ERROR = 1;
constexpr int EXIT_USAGE = 2;

struct options_o {
    std::string s_path;
    std::string s_output;
    std::string s_qr_png;
    bool b_back = false;
    bool b_no_images = false;
};

void print_usage(std::ostream &s_out) {
    s_out << "usage: pkpass-render [--back] [--no-images] [--output FILE]\n"
             "                      [--qr-png FILE] FILE.pkpass\n"
             "\n"
             "  --back       draw the reverse of the pass (the back fields)\n"
             "  --no-images  do not embed the pass's own images\n"
             "  --output     write the SVG to FILE instead of standard output\n"
             "  --qr-png     also write the barcode as a PNG, for a scanner to read\n";
}

}  // namespace

int main(int i_argument_count, char *o_arguments[]) {
    options_o o_options;
    for (int i_argument = 1; i_argument < i_argument_count; ++i_argument) {
        const std::string s_argument = o_arguments[i_argument];
        if ("--back" == s_argument) {
            o_options.b_back = true;
        } else if ("--no-images" == s_argument) {
            o_options.b_no_images = true;
        } else if ("--qr-png" == s_argument) {
            if (i_argument + 1 >= i_argument_count) {
                std::cerr << "pkpass-render: --qr-png needs a file name\n";
                return EXIT_USAGE;
            }
            o_options.s_qr_png = o_arguments[++i_argument];
        } else if ("--output" == s_argument || "-o" == s_argument) {
            if (i_argument + 1 >= i_argument_count) {
                std::cerr << "pkpass-render: --output needs a file name\n";
                return EXIT_USAGE;
            }
            o_options.s_output = o_arguments[++i_argument];
        } else if ("--help" == s_argument || "-h" == s_argument) {
            print_usage(std::cout);
            return EXIT_OK;
        } else if ("--version" == s_argument) {
            std::cout << "pkpass-render " << gnome_pkpass::version::version_string() << " (build "
                      << gnome_pkpass::version::version_build_counter() << ")\n";
            return EXIT_OK;
        } else if (2 < s_argument.size() && 0 == s_argument.compare(0, 2, "--")) {
            std::cerr << "pkpass-render: unknown option: " << s_argument << '\n';
            print_usage(std::cerr);
            return EXIT_USAGE;
        } else if (o_options.s_path.empty()) {
            o_options.s_path = s_argument;
        } else {
            std::cerr << "pkpass-render: only one file may be given\n";
            return EXIT_USAGE;
        }
    }

    if (o_options.s_path.empty()) {
        print_usage(std::cerr);
        return EXIT_USAGE;
    }

    gnome_pkpass::pkpass::pass_o o_pass;
    std::string s_error;
    if (!gnome_pkpass::pkpass::read_pass(o_options.s_path, o_pass, s_error)) {
        std::cerr << "pkpass-render: " << s_error << '\n';
        return EXIT_ERROR;
    }

    // The barcode as a PNG, when one was asked for: the same symbol the card
    // draws, so it can be scanned from a printed card or another screen.
    if (!o_options.s_qr_png.empty()) {
        if (o_pass.o_barcodes.empty()) {
            std::cerr << "pkpass-render: the pass carries no barcode\n";
            return EXIT_ERROR;
        }
        gnome_pkpass::qr::matrix_o o_matrix;
        if (!gnome_pkpass::render::encode_barcode(o_pass.o_barcodes.front(),
                                                  gnome_pkpass::qr::level_e::medium, o_matrix,
                                                  s_error)) {
            std::cerr << "pkpass-render: " << s_error << '\n';
            return EXIT_ERROR;
        }
        if (!gnome_pkpass::util::write_file(o_options.s_qr_png,
                                            gnome_pkpass::qr::to_png(o_matrix, 6, 4), s_error)) {
            std::cerr << "pkpass-render: " << s_error << '\n';
            return EXIT_ERROR;
        }
    }

    gnome_pkpass::render::options_o o_render;
    o_render.b_render_back = o_options.b_back;
    o_render.b_embed_images = !o_options.b_no_images;

    const std::string s_svg =
        gnome_pkpass::render::render_svg(o_options.s_path, o_pass, o_render, s_error);
    if (!s_error.empty()) {
        std::cerr << "pkpass-render: warning: " << s_error << '\n';
    }

    if (o_options.s_output.empty()) {
        std::cout << s_svg;
        return EXIT_OK;
    }

    if (!gnome_pkpass::util::write_file(o_options.s_output, s_svg, s_error)) {
        std::cerr << "pkpass-render: " << s_error << '\n';
        return EXIT_ERROR;
    }
    return EXIT_OK;
}
