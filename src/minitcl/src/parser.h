// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Tcl command parser

#pragma once

#include <string>
#include <vector>

#include "tcl.h"

namespace minitcl {

// A parsed command is a list of words (before variable/command substitution).
// Each word tracks whether it was braced (no substitution) or not.
struct Word {
    std::string text;
    bool braced = false;   // {}-quoted: no substitution needed
    bool expand = false;   // {*} prefix: expand result as list into args
};

struct ParsedCommand {
    std::vector<Word> words;
};

// Parse a complete Tcl script into a sequence of commands.
// Commands are separated by newlines or semicolons.
// Handles comments (#), braced strings {}, double-quoted strings "",
// and backslash-newline continuation.
std::vector<ParsedCommand> parseScript(const char *script);

// Perform backslash substitution on a string.
// Handles: \n \t \r \\ \{ \} \" \$ \[ \] \a \b \f \v
// \ooo (octal) \xHH (hex) \uHHHH (unicode)
// \<newline> (line continuation - absorbed with following whitespace)
std::string backslashSubst(const std::string &str);

}  // namespace minitcl
