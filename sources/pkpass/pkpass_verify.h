#pragma once

#include <string>
#include <vector>

#include "pkpass/pass_model.h"

namespace gnome_pkpass::pkpass {

// The result of checking one manifest entry against the member it names.
struct manifest_check_o {
    std::string s_name;
    bool b_present = false;
    bool b_match = false;
    std::string s_expected;
    std::string s_actual;
};

struct verify_result_o {
    std::vector<manifest_check_o> o_checks;
    // Members that are in the container but in no manifest entry.  The format
    // excludes only manifest.json and signature, so anything else is worth
    // naming; an issuer that adds an image without resigning leaves this behind.
    std::vector<std::string> o_extra_members;
    int i_missing = 0;
    int i_mismatch = 0;
    bool b_ok = false;

    bool ok() const {
        return b_ok;
    }
};

// Check every manifest entry against the file it names, using SHA-1 as the
// format does.  A digest is compared case-insensitively, because the hex case is
// not part of the format.
bool verify_manifest(const std::string &s_path, const pass_o &o_pass, verify_result_o &o_result,
                     std::string &s_error);

struct signature_check_o {
    bool b_present = false;
    // absent | verified | invalid | unavailable
    std::string s_status = "absent";
    std::string s_detail;
    std::string s_signer;
};

// Check the detached PKCS#7 signature over `manifest.json`.  This needs
// `openssl`; without it the status is `unavailable` rather than a guess.  The
// signer chain is Apple's, so a pass verifies its content here even when the
// chain cannot be completed offline; the check is therefore of the content, and
// says so.
bool verify_signature(const pass_o &o_pass, signature_check_o &o_check, std::string &s_error);

}  // namespace gnome_pkpass::pkpass
