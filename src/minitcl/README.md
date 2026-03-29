# MiniTcl

Minimal Tcl interpreter for OpenROAD — ~6700 lines of C++ replacing the
250K-line Tcl 8.6/9.0 library.  Implements only the Tcl subset that
OpenROAD actually uses: enough for STA init scripts, SWIG-generated
bindings, and user Tcl flows through the complete physical
implementation pipeline (LEF → Liberty → Verilog → floorplan → placement
→ CTS → routing → output).

No external Tcl dependency.  Bazel-native.  Self-contained.

See [STATUS.md](STATUS.md) for the test matrix, working flow details,
and known issues.

---

## Design: Keeping `src/sta` Untouched

STA is an upstream submodule.  Patching its BUILD files to reference
minitcl would create a permanent merge burden against upstream — every
STA update would conflict.

MiniTcl avoids this entirely:

1. `third-party/tcl_lang/` contains a header-only Bazel shim that
   re-exports minitcl's `tcl.h` under the `@tcl_lang//:tcl` target.

2. `MODULE.bazel` uses `local_path_override` to redirect the
   `@tcl_lang` dependency to this shim:

   ```bzl
   bazel_dep(name = "tcl_lang", version = "8.6.16.bcr.1")
   local_path_override(
       module_name = "tcl_lang",
       path = "third-party/tcl_lang",
   )
   ```

3. STA's BUILD file depends on `@tcl_lang//:tcl` — unchanged from
   upstream.  It gets minitcl headers without knowing it.

4. `tcl.h` advertises Tcl 9.0 version constants
   (`TCL_MAJOR_VERSION = 9`) so version-gated code takes modern paths.

5. `evalTclInit` warns instead of aborting on partial init failures,
   since minitcl may not handle every edge case in STA's init scripts.

The result: `src/sta` stays at upstream commit, unpatched.  Merging
upstream STA updates is a clean fast-forward.

---

## Build Integration: Making Tcl a Bazel Config Option

**Current state:** The `local_path_override` in MODULE.bazel is
hardcoded — you get minitcl or nothing.  CMake doesn't support minitcl
and continues to use system Tcl.

**Proposed:** Make the Tcl backend a command-line flag:

```bzl
# In src/minitcl/BUILD or a central config:
string_flag(name = "tcl_backend", build_setting_default = "mini")

config_setting(
    name = "use_minitcl",
    flag_values = {":tcl_backend": "mini"},
)

config_setting(
    name = "use_system_tcl",
    flag_values = {":tcl_backend": "system"},
)
```

Dependent BUILD files would use `select()`:

```bzl
deps = select({
    "//src/minitcl:use_minitcl": ["//src/minitcl"],
    "//src/minitcl:use_system_tcl": ["@tcl_lang//:tcl"],
})
```

Usage: `bazelisk build //... --//src/minitcl:tcl_backend=system`

Benefits: CI can test both backends.  Users can opt in or out with a
single flag.  The `local_path_override` shim only applies when
`tcl_backend=mini`.

---

## Accidental Complexity

### Everything is a string internally

Tcl's motto is "everything is a string," but standard Tcl maintains
dual representations (string + cached internal type) to avoid constant
re-parsing.  MiniTcl does not — there is no typed internal
representation.

`Tcl_NewIntObj` converts an integer to a string via `snprintf`.
Downstream code calls `strtol` to get the integer back.  Every list
operation follows the same cycle:

```
parse string → vector<string> → manipulate → rebuild string
```

`lappend` parses the entire list, appends one element, and rebuilds.
Dict operations search a flat string list linearly.  This is fine for
OpenROAD's workload (the bottleneck is C++ routing/CTS, not Tcl
dispatch), but it means list-heavy scripts pay a quadratic tax that
real Tcl avoids.

### Monolithic command handlers

`stringCmd` is 270 lines handling 20+ subcommands through an if/else
`strcmp` chain.  `dictCmd` is 130 lines of the same structure.
`parsePrimary()` in the expression parser is 195 lines covering
parentheses, strings, variables, commands, functions, and numbers in
one function.

### Copy-paste boilerplate

The "wrong # args" error pattern appears 50+ times across all command
files, each a hand-written:

```cpp
if (objc != N) {
    getImpl(interp)->result = "wrong # args: should be \"cmd ...\"";
    return TCL_ERROR;
}
```

Index parsing (`"end"`, `"end-N"`, integer) is duplicated in 5+ places.
List element quoting logic (check for spaces → add braces) appears in
6+ locations.

### No dispatch tables

Subcommand routing uses linear `strcmp` chains instead of a lookup
table.  A `std::map<std::string_view, handler>` or even a sorted array
with binary search would be both faster and more readable.

---

## Antipatterns and Recommended Refactorings

### Helper extractions

| Helper | Replaces | Occurrences |
|--------|----------|-------------|
| `parseIndex(const char*, int len) → int` | Duplicated "end"/"end-N"/atoi logic | 5+ |
| `setArgError(interp, cmd, usage)` | Inline "wrong # args" patterns | 50+ |
| `quoteListElement(string_view) → string` | Inline quoting-if-spaces blocks | 6+ |

### Dispatch tables

Replace the if/else `strcmp` chains in `stringCmd`, `dictCmd`,
`arrayCmd`, and `fileCmd` with dispatch tables:

```cpp
using Handler = int (*)(Tcl_Interp *, int, Tcl_Obj *const[]);
static const std::map<std::string_view, Handler> kStringCmds = {
    {"length", stringLengthCmd},
    {"index",  stringIndexCmd},
    // ...
};
```

Each handler becomes a focused, testable function.

### File splits

| Current file | Lines | Split into |
|-------------|-------|------------|
| `string_cmds.cc` | 787 | `string_cmds.cc` + `list_cmds.cc` |
| `dict_array.cc` | 507 | `dict.cc` + `array.cc` + `info_cmd.cc` |

### Memory leaks

- **`regexp.cc`**: global `vector<CompiledRegexp*>` grows indefinitely.
  Use `vector<unique_ptr<CompiledRegexp>>` or clean up on interp
  deletion.
- **`channel.cc`**: stacked channels stored as raw pointers in a global
  vector.  Use `unique_ptr`.

### Consider a TclList type

A thin wrapper around `vector<string>` that caches the string
representation would eliminate the repeated parse → modify → rebuild
cycle in `lappend`, `lrange`, `lsort`, `lsearch`, etc.

---

## Moving Commands from C++ to Tcl

Standard Tcl implements many commands in Tcl itself (tcllib).  MiniTcl
already has a VFS mechanism (`vfs.cc`) for embedded scripts — a
bootstrap path exists.  A `stdlib.tcl` registered at interpreter startup
could host pure-Tcl command implementations, reducing C++ surface area
without runtime cost.

### Phase 1 — Stubs (trivial, zero risk)

Already no-ops in C++, cleaner as Tcl procs:

```tcl
proc package {args} {}
proc encoding {args} { return "utf-8" }
proc history {args} {}
proc update {args} {}
proc trace {args} {}
```

### Phase 2 — Simple commands (low risk)

Logic that needs no C++ API access:

- **String:** `string reverse`, `string repeat`, `string cat`
- **List:** `lreverse`, `lrepeat`, `lmap`, `join`, `concat`
- **Lambda:** `apply` (already internally creates a proc and calls it)

Example — `lreverse` today is 8 lines of C++ (parse list, reverse
vector, rebuild string).  As Tcl:

```tcl
proc lreverse {list} {
    set result {}
    for {set i [expr {[llength $list] - 1}]} {$i >= 0} {incr i -1} {
        lappend result [lindex $list $i]
    }
    return $result
}
```

### Phase 3 — Dict operations (medium risk)

All `dict` subcommands (`create`, `get`, `set`, `keys`, `values`,
`exists`, `size`, `for`) are list-based string manipulation.  `array`
commands need a variable-introspection primitive exposed from C++, but
the command logic itself is pure Tcl.

### Keep in C++

| Category | Why |
|----------|-----|
| `break`, `continue` | Need `TCL_BREAK`/`TCL_CONTINUE` return codes |
| `expr` | Performance-sensitive recursive descent parser |
| I/O: `open`, `close`, `gets`, `read`, `puts` | Require `FILE*` management |
| `exec` | Requires `popen()`/`pclose()` |
| `clock seconds`/`milliseconds` | Requires `<chrono>` |
| `source` | File reading + VFS integration |
| `file exists`, `file mkdir`, etc. | Require `<filesystem>` |
| `pid`, `cd`, `pwd`, `exit` | System calls |
| Parsing infrastructure | Core engine (`parseScript`, `parseList`) |

### Performance note

OpenROAD's runtime is dominated by C++ (detailed routing, CTS, STA
graph traversal).  Tcl command dispatch is not on the critical path.
Moving ~30 simple commands to Tcl has negligible runtime impact.

---

## IWYU (Include What You Use)

Following hzeller's IWYU discipline in OpenROAD (see commits
`ecdc4846eb`, `bbbcf495aa`, `323c31c96d`), minitcl has several
include hygiene issues:

### Unused includes to remove

| File | Unused include | Reason |
|------|---------------|--------|
| `file_cmds.cc` | `<fstream>` | Only uses `<cstdio>` / `FILE*` |
| `misc_cmds.cc` | `<sstream>` | No `ostringstream` usage |
| `dict_array.cc` | `<algorithm>` | No `std::sort`, `std::find`, etc. |
| `dict_array.cc` | `<unistd.h>` | No POSIX calls |
| `expr.cc` | `<cstring>` | No `strcmp`, `strlen`, etc. |
| `eval.cc` | `<iostream>` | Only uses `<cstdio>` for `fprintf` |
| `string_cmds.cc` | `<sstream>` | No `ostringstream` usage |

### Missing includes to add

| File | Missing include | Used symbol |
|------|----------------|-------------|
| `channel.cc` | `<cstring>` | `memset` |

### Include order

The OpenROAD convention (matching Google style): own header first, then
C system headers, then C++ standard headers, then project headers, each
group separated by a blank line.  MiniTcl files mostly follow this but
should be audited for consistency.

---

## Aligning with OpenROAD / hzeller Style

hzeller's contributions to OpenROAD establish clear principles: strict
IWYU, no `using std::` in headers, meaningful typedefs for complex
container types, factory patterns returning `unique_ptr`, and
trailing-underscore member naming.  His own projects (timg,
rpi-rgb-led-matrix) reinforce these: minimal focused includes, concise
inline documentation, const correctness, and smart-pointer ownership.

MiniTcl was written quickly for correctness.  Here is where it diverges
from these standards and what to do about it.

### Smart pointers over raw new/delete

| Location | Current | Recommended |
|----------|---------|-------------|
| `interp.cc` `Tcl_CreateInterp` | `new InterpImpl()` | `unique_ptr<InterpImpl>` stored internally, raw ptr exposed to C API via `.get()` |
| `regexp.cc` compiled regexp cache | `vector<CompiledRegexp*>` | `vector<unique_ptr<CompiledRegexp>>` |
| `channel.cc` stacked channels | `vector<ChannelImpl*>` | `vector<unique_ptr<ChannelImpl>>` |

### Naming consistency

OpenROAD convention: member variables use trailing underscore
(`grid_`, `pdn_`, `cells_`).  MiniTcl's `InterpImpl` uses bare names
(`result`, `commands`, `globals`).  Rename to `result_`, `commands_`,
`globals_`, etc.

### No `using std::` in headers

Per hzeller's commit `2d349c12d1` ("Shortcuts like `using std::vector`
pollute the namespace for everyone including the header").  Audit
minitcl headers and replace any `using` declarations with explicit
`std::` qualification.

### Replace C-isms with idiomatic C++

| C pattern | C++ replacement | Where |
|-----------|----------------|-------|
| `strcmp(a, b) == 0` | `a == b` (on `std::string`) | All command files |
| `atoi(s)` | `std::stoi(s)` | Index parsing, expr |
| `snprintf(buf, ..., val)` | `std::to_string(val)` | `interp.cc`, `expr.cc` |
| `fprintf(stderr, ...)` | Structured error helper | Diagnostic output |

Note: `strcmp` on `const char*` from `Tcl_GetString()` is unavoidable
at the C API boundary.  The goal is to convert to `std::string` or
`std::string_view` early and use C++ comparisons from that point on.

### Meaningful typedefs

Per hzeller's `frOrderedIdMap` pattern — give complex container types a
name that communicates intent:

```cpp
using CmdMap = std::map<std::string, CmdEntry>;
using VarMap = std::map<std::string, std::string>;
using ProcMap = std::map<std::string, ProcDef>;
```

### License headers

Add SPDX headers to all minitcl source files:

```cpp
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, OpenROAD Contributors
```
