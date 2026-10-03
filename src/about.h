#ifndef EDITOR_ABOUT_H
#define EDITOR_ABOUT_H

#include <string>
#include <vector>

namespace editor {

namespace about {

const char* name();
const char* version();

std::vector<std::string> lines();
// The release record: every program in `directory` - and in `more`, under names not yet listed - by CRC-32, size and
// name, written into directory/release.crc as the installer's last step. About compares its own program with it.
int writeReleaseRecord(const std::string& directory, const std::vector<std::string>& more = std::vector<std::string>());
// Help > Environment: every tool, header directory and library in force, the file each resolved to, and why.
std::vector<std::string> environment();

}
}

#endif
