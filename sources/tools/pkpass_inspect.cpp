//
// pkpass-inspect: report what a `.pkpass` contains.
//
// The report is the platform-independent part of this project: it reads the
// container, the pass, and the manifest, and checks the digests, without
// touching the desktop.  `--json` prints the same facts for another program.
//
#include "json/json_reader.h"
#include "pkpass/pkpass_reader.h"
#include "pkpass/pkpass_verify.h"
#include "util/file_io.h"
#include "version/version.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int EXIT_OK = 0;
constexpr int EXIT_ERROR = 1;
constexpr int EXIT_USAGE = 2;

struct options_o {
    std::string s_path;
    bool b_json = false;
    bool b_signature = false;
};

void print_usage(std::ostream &s_out) {
    s_out << "usage: pkpass-inspect [--json] [--signature] FILE.pkpass\n"
             "\n"
             "  --json       print the pass as JSON instead of a report\n"
             "  --signature  also check the PKCS#7 signature (needs openssl)\n";
}

std::string color_text(const gnome_pkpass::pkpass::color_o &o_color) {
    if (!o_color.b_set) {
        return "unset";
    }
    std::ostringstream s_stream;
    s_stream << "rgb(" << o_color.i_red << ',' << o_color.i_green << ',' << o_color.i_blue << ')';
    if (255 != o_color.i_alpha) {
        s_stream << " alpha " << o_color.i_alpha;
    }
    return s_stream.str();
}

std::string format_coordinate(double d_value) {
    std::ostringstream s_stream;
    s_stream.precision(7);
    s_stream << d_value;
    return s_stream.str();
}

void print_field(const gnome_pkpass::pkpass::field_o &o_field, const std::string &s_indent) {
    std::cout << s_indent << o_field.s_key;
    if (!o_field.s_label.empty()) {
        std::cout << " [" << o_field.s_label << ']';
    }
    std::cout << ": ";
    const std::string s_value =
        o_field.s_attributed_value.empty() ? o_field.s_value : o_field.s_attributed_value;
    // A value with a newline would break the one-fact-per-line shape the report
    // is read with; the two spaces after a newline are kept as written.
    std::string s_flat = s_value;
    std::size_t u_at = 0;
    while ((u_at = s_flat.find('\n', u_at)) != std::string::npos) {
        s_flat.replace(u_at, 1, " \\n ");
        u_at += 4;
    }
    std::cout << s_flat << '\n';
    if (!o_field.s_date_style.empty() || !o_field.s_time_style.empty()) {
        std::cout << s_indent << "  style: date=" << o_field.s_date_style
                  << " time=" << o_field.s_time_style << '\n';
    }
    if (o_field.b_has_row) {
        std::cout << s_indent << "  row: " << o_field.i_row << '\n';
    }
}

void print_report(const std::string &s_path, const gnome_pkpass::pkpass::pass_o &o_pass,
                  const gnome_pkpass::pkpass::verify_result_o &o_verify,
                  const gnome_pkpass::pkpass::signature_check_o &o_signature,
                  bool b_did_signature) {
    using gnome_pkpass::pkpass::style_name;

    std::cout << "pass: " << s_path << '\n';
    std::cout << "size: " << o_pass.u_file_size << " bytes\n";
    std::cout << "members: " << o_pass.o_member_names.size() << '\n';
    for (const std::string &s_member : o_pass.o_member_names) {
        std::cout << "  " << s_member << '\n';
    }

    std::cout << "style: " << style_name(o_pass.e_style) << '\n';
    if (o_pass.b_has_format_version) {
        std::cout << "formatVersion: " << o_pass.i_format_version << '\n';
    }
    const auto print_line = [](const char *s_key, const std::string &s_value) {
        if (!s_value.empty()) {
            std::cout << s_key << ": " << s_value << '\n';
        }
    };
    print_line("passTypeIdentifier", o_pass.s_pass_type_identifier);
    print_line("serialNumber", o_pass.s_serial_number);
    print_line("teamIdentifier", o_pass.s_team_identifier);
    print_line("organizationName", o_pass.s_organization_name);
    print_line("description", o_pass.s_description);
    print_line("logoText", o_pass.s_logo_text);
    print_line("relevantDate", o_pass.s_relevant_date);
    print_line("expirationDate", o_pass.s_expiration_date);
    print_line("groupingIdentifier", o_pass.s_grouping_identifier);
    print_line("transitType", o_pass.s_transit_type);
    if (o_pass.b_voided) {
        std::cout << "voided: true\n";
    }
    if (o_pass.b_sharing_prohibited) {
        std::cout << "sharingProhibited: true\n";
    }
    if (!o_pass.s_web_service_url.empty()) {
        std::cout << "webServiceURL: " << o_pass.s_web_service_url << '\n';
        std::cout << "authenticationToken: "
                  << (o_pass.b_has_authentication_token ? "present" : "missing") << '\n';
    }
    if (o_pass.b_has_nfc) {
        std::cout << "nfc: present\n";
    }
    std::cout << "backgroundColor: " << color_text(o_pass.o_background_color) << '\n';
    std::cout << "foregroundColor: " << color_text(o_pass.o_foreground_color) << '\n';
    std::cout << "labelColor: " << color_text(o_pass.o_label_color) << '\n';

    for (const gnome_pkpass::pkpass::field_group_o &o_group : o_pass.o_field_groups) {
        if (o_group.o_fields.empty()) {
            continue;
        }
        std::cout << "fields " << o_group.s_name << ":\n";
        for (const gnome_pkpass::pkpass::field_o &o_field : o_group.o_fields) {
            print_field(o_field, "  ");
        }
    }

    for (std::size_t u_index = 0; u_index < o_pass.o_barcodes.size(); ++u_index) {
        const gnome_pkpass::pkpass::barcode_o &o_barcode = o_pass.o_barcodes[u_index];
        std::cout << "barcode" << (0 == u_index ? "" : " (alternate)")
                  << ": " << gnome_pkpass::pkpass::barcode_format_label(o_barcode.s_format);
        if (!o_barcode.s_message_encoding.empty()) {
            std::cout << " encoding=" << o_barcode.s_message_encoding;
        }
        std::cout << '\n';
        if (!o_barcode.s_alt_text.empty()) {
            std::cout << "  altText: " << o_barcode.s_alt_text << '\n';
        }
        std::cout << "  message: " << o_barcode.s_message << '\n';
    }

    for (const gnome_pkpass::pkpass::location_o &o_location : o_pass.o_locations) {
        std::cout << "location:";
        if (o_location.b_has_latitude) {
            std::cout << ' ' << format_coordinate(o_location.d_latitude);
        }
        if (o_location.b_has_longitude) {
            std::cout << ", " << format_coordinate(o_location.d_longitude);
        }
        if (!o_location.s_relevant_text.empty()) {
            std::cout << " (" << o_location.s_relevant_text << ')';
        }
        std::cout << '\n';
    }

    for (const gnome_pkpass::pkpass::image_o &o_image : o_pass.o_images) {
        std::cout << "image: " << o_image.s_name << " (" << o_image.u_size << " bytes)\n";
    }

    std::cout << "manifest: " << o_pass.o_manifest.size() << " entr"
              << (1 == o_pass.o_manifest.size() ? "y" : "ies");
    if (o_pass.o_manifest.empty()) {
        std::cout << " (none)\n";
    } else if (o_verify.b_ok) {
        std::cout << ", every SHA-1 digest matches\n";
    } else {
        std::cout << ", " << o_verify.i_missing << " missing and " << o_verify.i_mismatch
                  << " mismatched\n";
        for (const gnome_pkpass::pkpass::manifest_check_o &o_check : o_verify.o_checks) {
            if (o_check.b_match) {
                continue;
            }
            std::cout << "  " << o_check.s_name << ": ";
            if (!o_check.b_present) {
                std::cout << "listed but absent from the archive\n";
            } else {
                std::cout << "expected " << o_check.s_expected << " but found "
                          << o_check.s_actual << '\n';
            }
        }
    }
    for (const std::string &s_member : o_verify.o_extra_members) {
        std::cout << "unlisted member: " << s_member << '\n';
    }

    if (!o_pass.b_has_signature) {
        std::cout << "signature: absent\n";
    } else {
        std::cout << "signature: present (" << o_pass.u_signature_size << " bytes)";
        if (b_did_signature) {
            std::cout << "; " << o_signature.s_status;
            if (!o_signature.s_detail.empty()) {
                std::cout << " (" << o_signature.s_detail << ')';
            }
        }
        std::cout << '\n';
        if (b_did_signature && !o_signature.s_signer.empty()) {
            std::cout << "signer: " << o_signature.s_signer << '\n';
        }
    }

    for (const std::string &s_warning : o_pass.o_warnings) {
        std::cout << "warning: " << s_warning << '\n';
    }
}

gnome_pkpass::json::value_c to_json(const gnome_pkpass::pkpass::pass_o &o_pass,
                                    const gnome_pkpass::pkpass::verify_result_o &o_verify,
                                    const gnome_pkpass::pkpass::signature_check_o &o_signature,
                                    bool b_did_signature) {
    using gnome_pkpass::json::value_c;

    value_c o_root = value_c::make_object();
    o_root.set_member("path", value_c::make_string(o_pass.s_path));
    o_root.set_member("sizeBytes", value_c::make_number(static_cast<double>(o_pass.u_file_size)));

    value_c o_members = value_c::make_array();
    for (const std::string &s_member : o_pass.o_member_names) {
        o_members.append_item(value_c::make_string(s_member));
    }
    o_root.set_member("members", o_members);

    o_root.set_member("style",
                      value_c::make_string(gnome_pkpass::pkpass::style_key(o_pass.e_style)));
    o_root.set_member("formatVersion",
                      value_c::make_number(static_cast<double>(o_pass.i_format_version)));
    o_root.set_member("passTypeIdentifier", value_c::make_string(o_pass.s_pass_type_identifier));
    o_root.set_member("serialNumber", value_c::make_string(o_pass.s_serial_number));
    o_root.set_member("teamIdentifier", value_c::make_string(o_pass.s_team_identifier));
    o_root.set_member("organizationName", value_c::make_string(o_pass.s_organization_name));
    o_root.set_member("description", value_c::make_string(o_pass.s_description));
    o_root.set_member("logoText", value_c::make_string(o_pass.s_logo_text));
    o_root.set_member("relevantDate", value_c::make_string(o_pass.s_relevant_date));
    o_root.set_member("expirationDate", value_c::make_string(o_pass.s_expiration_date));
    o_root.set_member("voided", value_c::make_boolean(o_pass.b_voided));
    o_root.set_member("sharingProhibited", value_c::make_boolean(o_pass.b_sharing_prohibited));
    o_root.set_member("hasNfc", value_c::make_boolean(o_pass.b_has_nfc));
    if (!o_pass.s_web_service_url.empty()) {
        o_root.set_member("webServiceURL", value_c::make_string(o_pass.s_web_service_url));
        o_root.set_member("hasAuthenticationToken",
                          value_c::make_boolean(o_pass.b_has_authentication_token));
    }

    const auto color_json = [](const gnome_pkpass::pkpass::color_o &o_color) {
        if (!o_color.b_set) {
            return value_c::make_null();
        }
        value_c o_value = value_c::make_object();
        o_value.set_member("red", value_c::make_number(static_cast<double>(o_color.i_red)));
        o_value.set_member("green", value_c::make_number(static_cast<double>(o_color.i_green)));
        o_value.set_member("blue", value_c::make_number(static_cast<double>(o_color.i_blue)));
        o_value.set_member("alpha", value_c::make_number(static_cast<double>(o_color.i_alpha)));
        return o_value;
    };
    o_root.set_member("backgroundColor", color_json(o_pass.o_background_color));
    o_root.set_member("foregroundColor", color_json(o_pass.o_foreground_color));
    o_root.set_member("labelColor", color_json(o_pass.o_label_color));

    value_c o_groups = value_c::make_object();
    for (const gnome_pkpass::pkpass::field_group_o &o_group : o_pass.o_field_groups) {
        if (o_group.o_fields.empty()) {
            continue;
        }
        value_c o_fields = value_c::make_array();
        for (const gnome_pkpass::pkpass::field_o &o_field : o_group.o_fields) {
            value_c o_value = value_c::make_object();
            o_value.set_member("key", value_c::make_string(o_field.s_key));
            o_value.set_member("label", value_c::make_string(o_field.s_label));
            o_value.set_member("value", value_c::make_string(o_field.s_value));
            if (!o_field.s_attributed_value.empty()) {
                o_value.set_member("attributedValue",
                                   value_c::make_string(o_field.s_attributed_value));
            }
            if (!o_field.s_date_style.empty()) {
                o_value.set_member("dateStyle", value_c::make_string(o_field.s_date_style));
            }
            if (!o_field.s_time_style.empty()) {
                o_value.set_member("timeStyle", value_c::make_string(o_field.s_time_style));
            }
            o_fields.append_item(o_value);
        }
        o_groups.set_member(o_group.s_name, o_fields);
    }
    o_root.set_member("fields", o_groups);

    value_c o_barcodes = value_c::make_array();
    for (const gnome_pkpass::pkpass::barcode_o &o_barcode : o_pass.o_barcodes) {
        value_c o_value = value_c::make_object();
        o_value.set_member("format", value_c::make_string(o_barcode.s_format));
        o_value.set_member("message", value_c::make_string(o_barcode.s_message));
        o_value.set_member("messageEncoding",
                           value_c::make_string(o_barcode.s_message_encoding));
        o_value.set_member("altText", value_c::make_string(o_barcode.s_alt_text));
        o_barcodes.append_item(o_value);
    }
    o_root.set_member("barcodes", o_barcodes);

    value_c o_locations = value_c::make_array();
    for (const gnome_pkpass::pkpass::location_o &o_location : o_pass.o_locations) {
        value_c o_value = value_c::make_object();
        if (o_location.b_has_latitude) {
            o_value.set_member("latitude", value_c::make_number(o_location.d_latitude));
        }
        if (o_location.b_has_longitude) {
            o_value.set_member("longitude", value_c::make_number(o_location.d_longitude));
        }
        o_value.set_member("relevantText", value_c::make_string(o_location.s_relevant_text));
        o_locations.append_item(o_value);
    }
    o_root.set_member("locations", o_locations);

    value_c o_verification = value_c::make_object();
    o_verification.set_member("manifestOk", value_c::make_boolean(o_verify.b_ok));
    o_verification.set_member("manifestMissing",
                              value_c::make_number(static_cast<double>(o_verify.i_missing)));
    o_verification.set_member("manifestMismatched",
                              value_c::make_number(static_cast<double>(o_verify.i_mismatch)));
    value_c o_entries = value_c::make_array();
    for (const gnome_pkpass::pkpass::manifest_check_o &o_check : o_verify.o_checks) {
        value_c o_value = value_c::make_object();
        o_value.set_member("name", value_c::make_string(o_check.s_name));
        o_value.set_member("present", value_c::make_boolean(o_check.b_present));
        o_value.set_member("match", value_c::make_boolean(o_check.b_match));
        o_value.set_member("expected", value_c::make_string(o_check.s_expected));
        o_value.set_member("actual", value_c::make_string(o_check.s_actual));
        o_entries.append_item(o_value);
    }
    o_verification.set_member("entries", o_entries);
    o_verification.set_member("hasSignature", value_c::make_boolean(o_pass.b_has_signature));
    o_verification.set_member("signatureChecked", value_c::make_boolean(b_did_signature));
    o_verification.set_member("signatureStatus", value_c::make_string(o_signature.s_status));
    o_verification.set_member("signer", value_c::make_string(o_signature.s_signer));
    o_root.set_member("verification", o_verification);

    value_c o_warnings = value_c::make_array();
    for (const std::string &s_warning : o_pass.o_warnings) {
        o_warnings.append_item(value_c::make_string(s_warning));
    }
    o_root.set_member("warnings", o_warnings);

    return o_root;
}

}  // namespace

int main(int i_argument_count, char *o_arguments[]) {
    options_o o_options;
    for (int i_argument = 1; i_argument < i_argument_count; ++i_argument) {
        const std::string s_argument = o_arguments[i_argument];
        if ("--json" == s_argument) {
            o_options.b_json = true;
        } else if ("--signature" == s_argument) {
            o_options.b_signature = true;
        } else if ("--help" == s_argument || "-h" == s_argument) {
            print_usage(std::cout);
            return EXIT_OK;
        } else if ("--version" == s_argument) {
            std::cout << "pkpass-inspect " << gnome_pkpass::version::version_string() << " (build "
                      << gnome_pkpass::version::version_build_counter() << ")\n";
            return EXIT_OK;
        } else if ("--" == s_argument.substr(0, 2)) {
            std::cerr << "pkpass-inspect: unknown option: " << s_argument << '\n';
            print_usage(std::cerr);
            return EXIT_USAGE;
        } else if (o_options.s_path.empty()) {
            o_options.s_path = s_argument;
        } else {
            std::cerr << "pkpass-inspect: only one file may be given\n";
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
        std::cerr << "pkpass-inspect: " << s_error << '\n';
        return EXIT_ERROR;
    }

    gnome_pkpass::pkpass::verify_result_o o_verify;
    if (!gnome_pkpass::pkpass::verify_manifest(o_options.s_path, o_pass, o_verify, s_error)) {
        std::cerr << "pkpass-inspect: " << s_error << '\n';
        return EXIT_ERROR;
    }

    gnome_pkpass::pkpass::signature_check_o o_signature;
    if (o_options.b_signature) {
        if (!gnome_pkpass::pkpass::verify_signature(o_pass, o_signature, s_error)) {
            std::cerr << "pkpass-inspect: " << s_error << '\n';
            return EXIT_ERROR;
        }
    }

    if (o_options.b_json) {
        std::cout << to_json(o_pass, o_verify, o_signature, o_options.b_signature).to_text()
                  << '\n';
    } else {
        print_report(o_options.s_path, o_pass, o_verify, o_signature, o_options.b_signature);
    }

    // A mismatched digest is a finding, not a failure to read; the caller that
    // wants to refuse a pass checks `manifestOk` in the JSON, and the report
    // says so in words.  The command still exits 0 so a script can read the
    // report.
    return EXIT_OK;
}
