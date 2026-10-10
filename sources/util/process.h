#pragma once

#include <string>
#include <vector>

namespace gnome_pkpass::util {

// Whether a program is found on PATH and is executable.
bool command_exists(const std::string &s_program);

// Run a program with an argument vector, capturing its standard output and its
// standard error together.  No shell is involved, so an argument that carries
// spaces, quotes, or a semicolon is passed through unchanged; the desktop
// entries this project reads are full of such characters.
//
// On success b_exited is true and i_status is the exit status.  A program that
// cannot be started reports b_exited false, and s_error says why.
bool run_command(const std::vector<std::string> &o_arguments, std::string &s_output,
                 int &i_status, bool &b_exited, std::string &s_error);

}  // namespace gnome_pkpass::util
