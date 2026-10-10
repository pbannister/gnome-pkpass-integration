#include "version/version.h"

#include "version_info.h"

namespace gnome_pkpass::version {

const char *version_string() {
    return VERSION_STRING_VALUE;
}

int version_build_counter() {
    return VERSION_BUILD_COUNTER_VALUE;
}

}  // namespace gnome_pkpass::version
