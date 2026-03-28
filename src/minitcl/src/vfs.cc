// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Virtual filesystem for embedded scripts

#include "vfs.h"

#include <map>
#include <string>

namespace {

struct VfsEntry {
    std::string content;
};

std::map<std::string, VfsEntry> &vfsMap() {
    static std::map<std::string, VfsEntry> m;
    return m;
}

}  // namespace

void minitcl_vfs_register(const char *path, const char *content, size_t len) {
    vfsMap()[path] = {std::string(content, len)};
}

int minitcl_vfs_exists(const char *path) {
    return vfsMap().count(path) ? 1 : 0;
}

const char *minitcl_vfs_get(const char *path, size_t *len) {
    auto &m = vfsMap();
    auto it = m.find(path);
    if (it == m.end()) {
        if (len) *len = 0;
        return nullptr;
    }
    if (len) *len = it->second.content.size();
    return it->second.content.c_str();
}
