// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Virtual filesystem for embedded scripts

#pragma once

#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

// Register a virtual file in the VFS.
// When source or Tcl_EvalFile is called with this path, the content
// is used instead of reading from the real filesystem.
void minitcl_vfs_register(const char *path, const char *content, size_t len);

// Check if a path exists in the VFS.
int minitcl_vfs_exists(const char *path);

// Get content from VFS. Returns nullptr if not found.
const char *minitcl_vfs_get(const char *path, size_t *len);

#ifdef __cplusplus
}
#endif
