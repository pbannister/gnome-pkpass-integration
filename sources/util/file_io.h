#pragma once

#include <string>

namespace gnome_pkpass::util {

// Read a whole file into memory.  A pass is small (Apple caps a bundle at
// 150 MB, and a single pass is a fraction of that), and the ZIP reader needs
// random access to it, so the whole file is held rather than streamed.
bool read_file(const std::string &s_path, std::string &s_data, std::string &s_error);

// Write a whole file, creating no directories.  Returns false and fills s_error
// on failure.
bool write_file(const std::string &s_path, const std::string &s_data, std::string &s_error);

// Whether a path names an existing regular file.
bool file_exists(const std::string &s_path);

// Standard base64 (RFC 4648, with padding), used to embed a pass image in the
// rendered SVG as a data URI.
std::string base64_encode(const std::string &s_data);

}  // namespace gnome_pkpass::util
