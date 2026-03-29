// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Tcl command parser

#include "parser.h"

#include <cctype>
#include <cstdlib>

namespace minitcl {

// Skip whitespace (space and tab only, not newline)
static void skipWhitespace(const char *&p) {
    while (*p == ' ' || *p == '\t') p++;
}

// Parse a brace-quoted string: { ... }
// Braces nest. No substitution inside braces.
// Returns content between outer braces.
static bool parseBraced(const char *&p, std::string &out) {
    if (*p != '{') return false;
    p++;  // skip opening {
    int depth = 1;
    out.clear();
    while (*p && depth > 0) {
        if (*p == '\\') {
            if (*(p + 1) == '\\') {
                // Escaped backslash: consume both, don't affect brace counting
                out += *p++;
                out += *p++;
                continue;
            }
            if (*(p + 1) == '{' || *(p + 1) == '}') {
                // Escaped brace: include literally, don't count
                out += *p++;
                out += *p++;
                continue;
            }
            // Other backslash sequence: include literally
            out += *p++;
            if (*p) out += *p++;
            continue;
        }
        if (*p == '{') {
            depth++;
            out += *p;
        } else if (*p == '}') {
            depth--;
            if (depth > 0) out += *p;
        } else {
            out += *p;
        }
        p++;
    }
    return depth == 0;
}

// Parse a double-quoted string: " ... "
// Backslash substitution occurs inside quotes.
// Variable substitution ($) and command substitution ([]) are NOT
// handled here - they're handled during eval. We just collect the raw text.
static bool parseQuoted(const char *&p, std::string &out) {
    if (*p != '"') return false;
    p++;  // skip opening "
    out.clear();
    int bracketDepth = 0;
    while (*p && (*p != '"' || bracketDepth > 0)) {
        if (*p == '\\' && *(p + 1)) {
            // Keep the backslash sequence for later substitution
            out += *p++;
            out += *p++;
        } else if (*p == '[') {
            bracketDepth++;
            out += *p++;
        } else if (*p == ']' && bracketDepth > 0) {
            bracketDepth--;
            out += *p++;
        } else {
            out += *p++;
        }
    }
    if (*p == '"') p++;  // skip closing "
    return true;
}

// Parse a bare word (unquoted).
// Ends at whitespace, semicolon, newline, or end of string.
// Brackets [ ] are part of the word (command substitution handled in eval).
// Backslash sequences are preserved for later substitution.
static bool parseBareWord(const char *&p, std::string &out) {
    out.clear();
    while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' &&
           *p != ';') {
        if (*p == '\\' && *(p + 1) == '\n') {
            // Backslash-newline: line continuation
            p += 2;
            // Skip leading whitespace on next line
            while (*p == ' ' || *p == '\t') p++;
            out += ' ';
        } else if (*p == '\\' && *(p + 1)) {
            out += *p++;
            out += *p++;
        } else if (*p == '[') {
            // Command substitution brackets - consume until matching ]
            out += *p++;
            int depth = 1;
            while (*p && depth > 0) {
                if (*p == '[') depth++;
                else if (*p == ']') {
                    depth--;
                    if (depth == 0) { out += *p++; break; }
                } else if (*p == '\\' && *(p + 1)) {
                    out += *p++;
                    out += *p++;
                    continue;
                } else if (*p == '{') {
                    // Braces inside brackets
                    out += *p++;
                    int bd = 1;
                    while (*p && bd > 0) {
                        if (*p == '{') bd++;
                        else if (*p == '}') bd--;
                        if (bd > 0 || *p != '}') out += *p;
                        p++;
                    }
                    out += '}';
                    continue;
                }
                if (depth > 0) out += *p++;
            }
        } else if (*p == '{') {
            out += *p++;
        } else if (*p == '"') {
            out += *p++;
        } else {
            out += *p++;
        }
    }
    return !out.empty();
}

std::vector<ParsedCommand> parseScript(const char *script) {
    std::vector<ParsedCommand> commands;
    const char *p = script;

    while (*p) {
        skipWhitespace(p);

        // Skip empty lines
        if (*p == '\n' || *p == '\r') {
            p++;
            continue;
        }

        // End of input
        if (*p == '\0') break;

        // Comment: skip to end of line
        if (*p == '#') {
            while (*p && *p != '\n') {
                if (*p == '\\' && *(p + 1) == '\n') {
                    p += 2;  // continuation
                } else {
                    p++;
                }
            }
            continue;
        }

        // Semicolons between commands
        if (*p == ';') {
            p++;
            continue;
        }

        // Parse one command (sequence of words until newline/semicolon/eof)
        ParsedCommand cmd;
        while (*p && *p != ';') {
            skipWhitespace(p);
            // Handle backslash-newline continuation between words
            while (*p == '\\' && *(p + 1) == '\n') {
                p += 2;
                skipWhitespace(p);
            }
            if (*p == '\0' || *p == '\n' || *p == '\r' || *p == ';') break;

            // Comment after words
            if (*p == '#' && !cmd.words.empty()) {
                // # is only a comment at the start of a command
                // In Tcl, # is a comment only when it's the first
                // non-whitespace char after a command separator
                // But if we're mid-command, it's a literal
                // Actually in Tcl, # is a comment at the start of a command
                // and this position is after words, so it's literal.
                // Wait - we need to check: if we just consumed a separator
                // then # is a comment. If we're in the middle of args, it's not.
                // The code structure means if we reach here with words already
                // parsed, we're in args, so # is literal.
            }

            Word word;
            // Check for {*} expansion prefix
            if (p[0] == '{' && p[1] == '*' && p[2] == '}') {
                word.expand = true;
                p += 3;  // skip {*}
                // Parse the following word (no whitespace between {*} and word)
                if (*p == '{') {
                    std::string text;
                    if (parseBraced(p, text)) {
                        word.text = text;
                        word.braced = true;
                        cmd.words.push_back(word);
                    }
                } else if (*p == '"') {
                    std::string text;
                    if (parseQuoted(p, text)) {
                        word.text = text;
                        word.braced = false;
                        cmd.words.push_back(word);
                    }
                } else if (*p && *p != ' ' && *p != '\t' && *p != '\n' &&
                           *p != ';') {
                    std::string text;
                    if (parseBareWord(p, text)) {
                        word.text = text;
                        word.braced = false;
                        cmd.words.push_back(word);
                    }
                }
            } else if (*p == '{') {
                std::string text;
                if (parseBraced(p, text)) {
                    word.text = text;
                    word.braced = true;
                    cmd.words.push_back(word);
                }
            } else if (*p == '"') {
                std::string text;
                if (parseQuoted(p, text)) {
                    word.text = text;
                    word.braced = false;
                    cmd.words.push_back(word);
                }
            } else {
                std::string text;
                if (parseBareWord(p, text)) {
                    word.text = text;
                    word.braced = false;
                    cmd.words.push_back(word);
                }
            }
        }

        if (!cmd.words.empty()) {
            commands.push_back(std::move(cmd));
        }

        // Skip the newline
        if (*p == '\n' || *p == '\r') p++;
    }

    return commands;
}

std::string backslashSubst(const std::string &str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] != '\\' || i + 1 >= str.size()) {
            result += str[i];
            continue;
        }

        char next = str[i + 1];
        i++;  // consume backslash

        switch (next) {
            case 'a': result += '\a'; break;
            case 'b': result += '\b'; break;
            case 'f': result += '\f'; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            case 'v': result += '\v'; break;
            case '\\': result += '\\'; break;
            case '{': result += '{'; break;
            case '}': result += '}'; break;
            case '"': result += '"'; break;
            case '$': result += '$'; break;
            case '[': result += '['; break;
            case ']': result += ']'; break;
            case '\n':
                // Line continuation: skip following whitespace
                while (i + 1 < str.size() &&
                       (str[i + 1] == ' ' || str[i + 1] == '\t')) {
                    i++;
                }
                result += ' ';
                break;
            case 'x': {
                // Hex: \xHH
                std::string hex;
                while (i + 1 < str.size() && isxdigit(str[i + 1]) &&
                       hex.size() < 2) {
                    hex += str[++i];
                }
                if (!hex.empty()) {
                    result += static_cast<char>(strtol(hex.c_str(), nullptr, 16));
                } else {
                    result += 'x';
                }
                break;
            }
            case 'u': {
                // Unicode: \uHHHH
                std::string hex;
                while (i + 1 < str.size() && isxdigit(str[i + 1]) &&
                       hex.size() < 4) {
                    hex += str[++i];
                }
                if (!hex.empty()) {
                    long code = strtol(hex.c_str(), nullptr, 16);
                    // Simple ASCII range only for now
                    if (code < 128) {
                        result += static_cast<char>(code);
                    } else {
                        // UTF-8 encode
                        if (code < 0x80) {
                            result += static_cast<char>(code);
                        } else if (code < 0x800) {
                            result += static_cast<char>(0xC0 | (code >> 6));
                            result += static_cast<char>(0x80 | (code & 0x3F));
                        } else {
                            result += static_cast<char>(0xE0 | (code >> 12));
                            result += static_cast<char>(0x80 |
                                                        ((code >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (code & 0x3F));
                        }
                    }
                } else {
                    result += 'u';
                }
                break;
            }
            default:
                if (next >= '0' && next <= '7') {
                    // Octal: \ooo (up to 3 digits)
                    std::string oct;
                    oct += next;
                    while (i + 1 < str.size() && str[i + 1] >= '0' &&
                           str[i + 1] <= '7' && oct.size() < 3) {
                        oct += str[++i];
                    }
                    result += static_cast<char>(
                        strtol(oct.c_str(), nullptr, 8));
                } else {
                    // Unknown escape: in Tcl, \X becomes X for any
                    // unrecognized X (the backslash is consumed)
                    result += next;
                }
                break;
        }
    }
    return result;
}

}  // namespace minitcl
