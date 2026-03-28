# MiniTcl Status

Minimal Tcl interpreter for OpenROAD — replaces the 250K-line Tcl 8.6/9.0
library with ~6700 lines of C++ that implement the Tcl subset OpenROAD
actually uses.

## Test Status

28 tests pass (18 C++ unit tests, 10 OpenROAD integration tests):

| Test | Time | What it exercises |
|------|------|------------------|
| `tcl_smoke` | 0.1s | set, expr, proc, namespace, string, list |
| `tcl_sta_commands` | 0.1s | STA init scripts loaded, is_keyword_arg |
| `tcl_read_design` | 0.3s | LEF + Liberty + DEF load, ODB SWIG queries |
| `tcl_floorplan` | 0.3s | read_verilog, link_design, initialize_floorplan |
| `tcl_placement` | 0.4s | check_placement, improve_placement |
| `tcl_timing` | 0.3s | report_checks, report_tns, report_wns, report_design_area |
| `tcl_cts` | 1.3s | clock_tree_synthesis |
| `tcl_route` | 7.7s | global_route + detailed_route |
| `tcl_full_flow` | 5.6s | End-to-end: CTS + GRT + DRT + write_def |
| `tcl_write_output` | 0.4s | improve_placement + write_def + write_verilog |

Run: `bazelisk test //src/minitcl/...`

## Working OpenROAD Flow

The complete physical implementation flow works end-to-end on Nangate45 GCD:

```
read_lef → read_liberty → read_verilog → link_design →
initialize_floorplan → improve_placement →
clock_tree_synthesis → global_route → detailed_route →
write_def / write_verilog
```

STA timing analysis works: `report_checks`, `report_tns`, `report_wns`,
`report_design_area` all produce correct output.

## Architecture

- `src/minitcl/include/tcl.h` — drop-in replacement for Tcl's public API
- `src/minitcl/src/` — implementation (~6700 lines total):
  - `parser.cc` — Tcl command parser (braces, quotes, backslash, {*})
  - `eval.cc` — eval engine, proc, set, subst, return -code
  - `expr.cc` — recursive descent expression parser
  - `interp.cc` — Tcl_Interp lifecycle, Tcl_Eval, Tcl_StringMatch, env
  - `control.cc` — if, while, for, foreach, switch, catch, break, continue
  - `string_cmds.cc` — string, list, format, scan, split, join, sort
  - `dict_array.cc` — dict, array, info, glob
  - `namespace.cc` — namespace eval/import/export, upvar, uplevel, variable
  - `regexp.cc` — regexp/regsub using std::regex
  - `file_cmds.cc` — file, source, open/close/gets/read
  - `channel.cc` — Tcl channel system for ReportTcl.cc (StackChannel etc.)
  - `swig_stubs.cc` — SWIG runtime: hash tables, Tcl_GetCommandInfo, etc.
  - `misc_cmds.cc` — clock, exec, exit, trace, interp, package, etc.
  - `vfs.cc` — virtual filesystem for embedded Tcl scripts
- `third-party/tcl_lang/` — Bazel overlay that redirects @tcl_lang to minitcl
  headers, so src/sta BUILD file stays unpatched

## Build Integration

- All BUILD files use `//src/minitcl` instead of `@tcl_lang//:tcl`
- `@tcl_lang` bazel_dep is redirected via `local_path_override` to a
  header-only shim at `third-party/tcl_lang/`
- `src/sta` submodule stays at upstream commit, unpatched
- `evalTclInit` warns instead of aborting on partial init failures

## Known Issues / What's Left

### Blocking the gcd_nangate45 regression flow

**`read_sdc` fails with `set_input_delay` argument error.** The SDC file
calls `set_all_input_output_delays` which calls `set_input_delay`. The
STA proc `set_port_delay` passes 10 args to the SWIG `set_input_delay_cmd`,
but the `$min_max` variable isn't being set to a default value. Root cause:
`parse_early_late_all_flags` (defined in Sdc.tcl) sets `$min_max` via upvar
but the variable resolution through nested upvar chains + namespace context
fails in this specific call pattern. Likely needs debugging of how
`parse_early_late_all_flags` interacts with the caller's frame.

### Language gaps (diminishing returns)

- **`-exit script.tcl` flag**: `sourceTclFile` calls `sta::include_file`
  which works but some error context formatting differs from real Tcl.
  Workaround: pipe `source script.tcl` to stdin instead.
- **`info commands` glob in procs**: glob matching works but `info procs`
  doesn't support glob patterns yet.
- **Variable tracing**: `trace` is a no-op stub. STA Variables.tcl uses
  trace to sync Tcl vars with C++ state; the C++ side handles this
  directly so the stub is sufficient for now.
- **`interp` command**: only `interp alias` is implemented (enough for
  dbSta.tcl). Other subcommands are no-ops.

### Edge cases in Tcl compatibility

- `namespace eval` + `set` at namespace scope qualifies variable names
  with the namespace, but `$var` substitution in non-proc namespace
  eval context doesn't auto-qualify. This means `set x 5` inside
  `namespace eval sta` creates `sta::x`, but `$x` in the same block
  looks for global `x`. Workaround: use `$sta::x` or `variable x`.
- Some STA init scripts partially fail (warnings printed to stderr).
  The most impactful failures are in scripts that use complex upvar
  chains through proc_redirect wrappers + catch + namespace contexts.
- `commands_without_load` test crashes — some SWIG commands segfault
  when called with wrong argument types (SWIG doesn't validate all
  error paths with minitcl's simplified Tcl_Obj).

### Not implemented (not needed by OpenROAD)

- Safe interpreters (`interp create -safe`)
- Package loading (`package require` is a no-op)
- Event loop (`vwait`, `fileevent`)
- Socket/HTTP networking
- Tk (GUI)
- Thread/coroutine support
- Binary data (`binary format/scan`)
- Encoding conversion (UTF-8 pass-through only)
