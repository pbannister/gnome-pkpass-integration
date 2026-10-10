#include "pkpass/pkpass_verify.h"

#include "crypto/sha1.h"
#include "util/file_io.h"
#include "util/process.h"
#include "zip/zip_reader.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <unistd.h>

namespace gnome_pkpass::pkpass {
namespace {

std::string lower(const std::string &s_text) {
    std::string s_lowered = s_text;
    std::transform(s_lowered.begin(), s_lowered.end(), s_lowered.begin(),
                   [](unsigned char c_next) { return static_cast<char>(std::tolower(c_next)); });
    return s_lowered;
}

// The last non-empty line of a program's output; openssl prints progress before
// the line that says what went wrong.
std::string last_line(const std::string &s_text) {
    std::size_t u_end = s_text.size();
    while (0 < u_end && ('\n' == s_text[u_end - 1] || '\r' == s_text[u_end - 1])) {
        --u_end;
    }
    const std::size_t u_start = (0 < u_end) ? s_text.rfind('\n', u_end - 1) : std::string::npos;
    const std::size_t u_begin = (std::string::npos == u_start) ? 0 : u_start + 1;
    return s_text.substr(u_begin, u_end - u_begin);
}

// A temporary file that removes itself.  The signature check has to hand two
// buffers to `openssl` as files, and leaking them into /tmp on every inspect
// would be its own small mess.
class temporary_file_c {
public:
    bool create(std::string &s_error) {
        // mkstemp replaces the trailing six X's, so nothing may follow them.
        std::string s_template = "/tmp/gnome-pkpass-XXXXXX";
        std::vector<char> o_buffer(s_template.begin(), s_template.end());
        o_buffer.push_back('\0');
        const int i_descriptor = ::mkstemp(o_buffer.data());
        if (0 > i_descriptor) {
            s_error = "cannot create a temporary file";
            return false;
        }
        ::close(i_descriptor);
        s_path_ = o_buffer.data();
        return true;
    }

    ~temporary_file_c() {
        if (!s_path_.empty()) {
            std::remove(s_path_.c_str());
        }
    }

    temporary_file_c() = default;
    temporary_file_c(const temporary_file_c &) = delete;
    temporary_file_c &operator=(const temporary_file_c &) = delete;

    const std::string &path() const {
        return s_path_;
    }

private:
    std::string s_path_;
};

}  // namespace

bool verify_manifest(const std::string &s_path, const pass_o &o_pass, verify_result_o &o_result,
                     std::string &s_error) {
    o_result = verify_result_o();

    zip::archive_c o_archive;
    if (!o_archive.load_file(s_path, s_error)) {
        return false;
    }

    for (const manifest_entry_o &o_entry : o_pass.o_manifest) {
        manifest_check_o o_check;
        o_check.s_name = o_entry.s_name;
        o_check.s_expected = lower(o_entry.s_digest);

        const zip::entry_o *p_member = o_archive.find(o_entry.s_name);
        if (nullptr == p_member) {
            o_result.o_checks.push_back(o_check);
            ++o_result.i_missing;
            continue;
        }

        std::string s_contents;
        std::string s_member_error;
        if (!o_archive.extract(*p_member, s_contents, s_member_error)) {
            o_result.o_checks.push_back(o_check);
            ++o_result.i_missing;
            continue;
        }

        o_check.b_present = true;
        o_check.s_actual = crypto::sha1_hex(s_contents);
        o_check.b_match = (o_check.s_actual == o_check.s_expected);
        if (!o_check.b_match) {
            ++o_result.i_mismatch;
        }
        o_result.o_checks.push_back(o_check);
    }

    for (const zip::entry_o &o_member : o_archive.entries()) {
        if ("manifest.json" == o_member.s_name || "signature" == o_member.s_name) {
            continue;
        }
        bool b_listed = false;
        for (const manifest_entry_o &o_entry : o_pass.o_manifest) {
            if (o_entry.s_name == o_member.s_name) {
                b_listed = true;
                break;
            }
        }
        if (!b_listed) {
            o_result.o_extra_members.push_back(o_member.s_name);
        }
    }

    o_result.b_ok = (0 == o_result.i_missing && 0 == o_result.i_mismatch);
    return true;
}

bool verify_signature(const pass_o &o_pass, signature_check_o &o_check, std::string &s_error) {
    o_check = signature_check_o();

    if (!o_pass.b_has_signature) {
        o_check.s_status = "absent";
        o_check.s_detail = "the archive carries no signature";
        return true;
    }
    o_check.b_present = true;

    if (!util::command_exists("openssl")) {
        o_check.s_status = "unavailable";
        o_check.s_detail = "openssl is not installed, so the signature was not checked";
        return true;
    }
    if (!o_pass.b_has_manifest || o_pass.s_manifest_json_text.empty()) {
        o_check.s_status = "unavailable";
        o_check.s_detail = "there is no manifest.json to check the signature against";
        return true;
    }

    temporary_file_c o_manifest_file;
    temporary_file_c o_signature_file;
    if (!o_manifest_file.create(s_error) || !o_signature_file.create(s_error)) {
        return false;
    }
    if (!util::write_file(o_manifest_file.path(), o_pass.s_manifest_json_text, s_error) ||
        !util::write_file(o_signature_file.path(), o_pass.s_signature_data, s_error)) {
        return false;
    }

    // `-noverify` checks the CMS digest over the manifest without requiring
    // Apple's root certificate, which a Linux host does not carry: the content
    // check is what is useful here, and the chain is reported separately.
    std::string s_output;
    int i_status = 0;
    bool b_exited = false;
    std::string s_run_error;
    const std::vector<std::string> o_command = {
        "openssl",  "smime",   "-verify",           "-inform", "DER",
        "-in",      o_signature_file.path(),        "-content", o_manifest_file.path(),
        "-binary",  "-noverify"};

    if (!util::run_command(o_command, s_output, i_status, b_exited, s_run_error)) {
        o_check.s_status = "unavailable";
        o_check.s_detail = "cannot run openssl: " + s_run_error;
        return true;
    }

    if (b_exited && 0 == i_status) {
        o_check.s_status = "verified";
        o_check.s_detail = "the CMS digest over manifest.json is valid";
    } else {
        o_check.s_status = "invalid";
        o_check.s_detail = "openssl refused the signature";
        const std::string s_reason = last_line(s_output);
        if (!s_reason.empty()) {
            o_check.s_detail += ": " + s_reason;
        }
    }

    // Name the signer when the certificate can be read; it is Apple's, and the
    // subject says which pass type it belongs to.
    std::string s_certs;
    int i_certs_status = 0;
    bool b_certs_exited = false;
    std::string s_certs_error;
    if (util::run_command({"openssl", "pkcs7", "-inform", "DER", "-in", o_signature_file.path(),
                           "-print_certs", "-noout"},
                          s_certs, i_certs_status, b_certs_exited, s_certs_error) &&
        b_certs_exited && 0 == i_certs_status) {
        std::size_t u_at = 0;
        while (u_at < s_certs.size()) {
            const std::size_t u_end = s_certs.find('\n', u_at);
            std::string s_line = s_certs.substr(u_at, u_end - u_at);
            if (0 == s_line.compare(0, 8, "subject=")) {
                o_check.s_signer = s_line.substr(8);
                break;
            }
            if (std::string::npos == u_end) {
                break;
            }
            u_at = u_end + 1;
        }
    }

    return true;
}

}  // namespace gnome_pkpass::pkpass
