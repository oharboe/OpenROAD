// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl Phase 2 - Parser tests (fine-grained, one aspect per test)

#include "tcl.h"

// Access parser internals for unit testing
#include "src/parser.h"

#include <gtest/gtest.h>

using minitcl::backslashSubst;
using minitcl::parseScript;
using minitcl::ParsedCommand;

// Helper: parse and return commands
static std::vector<ParsedCommand> parse(const char *script) {
    return parseScript(script);
}

// ============================================================
// Empty and trivial inputs
// ============================================================

TEST(ParserTest, EmptyString) {
    auto cmds = parse("");
    EXPECT_TRUE(cmds.empty());
}

TEST(ParserTest, WhitespaceOnly) {
    auto cmds = parse("   \t  ");
    EXPECT_TRUE(cmds.empty());
}

TEST(ParserTest, NewlineOnly) {
    auto cmds = parse("\n\n\n");
    EXPECT_TRUE(cmds.empty());
}

// ============================================================
// Single word commands
// ============================================================

TEST(ParserTest, SingleBareWord) {
    auto cmds = parse("hello");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 1u);
    EXPECT_EQ(cmds[0].words[0].text, "hello");
    EXPECT_FALSE(cmds[0].words[0].braced);
}

TEST(ParserTest, SingleBracedWord) {
    auto cmds = parse("{hello}");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 1u);
    EXPECT_EQ(cmds[0].words[0].text, "hello");
    EXPECT_TRUE(cmds[0].words[0].braced);
}

TEST(ParserTest, SingleQuotedWord) {
    auto cmds = parse("\"hello\"");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 1u);
    EXPECT_EQ(cmds[0].words[0].text, "hello");
    EXPECT_FALSE(cmds[0].words[0].braced);
}

// ============================================================
// Multiple words in one command
// ============================================================

TEST(ParserTest, TwoWords) {
    auto cmds = parse("set x");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 2u);
    EXPECT_EQ(cmds[0].words[0].text, "set");
    EXPECT_EQ(cmds[0].words[1].text, "x");
}

TEST(ParserTest, ThreeWordsWithValue) {
    auto cmds = parse("set x 42");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 3u);
    EXPECT_EQ(cmds[0].words[0].text, "set");
    EXPECT_EQ(cmds[0].words[1].text, "x");
    EXPECT_EQ(cmds[0].words[2].text, "42");
}

TEST(ParserTest, MultipleSpacesBetweenWords) {
    auto cmds = parse("set   x   42");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 3u);
    EXPECT_EQ(cmds[0].words[2].text, "42");
}

TEST(ParserTest, TabsBetweenWords) {
    auto cmds = parse("set\tx\t42");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 3u);
}

// ============================================================
// Command separators
// ============================================================

TEST(ParserTest, NewlineSeparatesTwoCommands) {
    auto cmds = parse("set x 1\nset y 2");
    ASSERT_EQ(cmds.size(), 2u);
    EXPECT_EQ(cmds[0].words[1].text, "x");
    EXPECT_EQ(cmds[1].words[1].text, "y");
}

TEST(ParserTest, SemicolonSeparatesTwoCommands) {
    auto cmds = parse("set x 1; set y 2");
    ASSERT_EQ(cmds.size(), 2u);
    EXPECT_EQ(cmds[0].words[1].text, "x");
    EXPECT_EQ(cmds[1].words[1].text, "y");
}

TEST(ParserTest, MultipleSemicolons) {
    auto cmds = parse("set x 1;;; set y 2");
    ASSERT_EQ(cmds.size(), 2u);
}

TEST(ParserTest, TrailingNewline) {
    auto cmds = parse("set x 1\n");
    ASSERT_EQ(cmds.size(), 1u);
}

TEST(ParserTest, TrailingSemicolon) {
    auto cmds = parse("set x 1;");
    ASSERT_EQ(cmds.size(), 1u);
}

// ============================================================
// Comments
// ============================================================

TEST(ParserTest, CommentLine) {
    auto cmds = parse("# this is a comment");
    EXPECT_TRUE(cmds.empty());
}

TEST(ParserTest, CommentBeforeCommand) {
    auto cmds = parse("# comment\nset x 1");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[0].text, "set");
}

TEST(ParserTest, CommentAfterCommand) {
    auto cmds = parse("set x 1\n# comment");
    ASSERT_EQ(cmds.size(), 1u);
}

TEST(ParserTest, CommentWithContinuation) {
    auto cmds = parse("# comment \\\ncontinued\nset x 1");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[0].text, "set");
}

// ============================================================
// Brace-quoted strings
// ============================================================

TEST(ParserTest, BracedStringPreservesSpaces) {
    auto cmds = parse("set x {hello world}");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 3u);
    EXPECT_EQ(cmds[0].words[2].text, "hello world");
    EXPECT_TRUE(cmds[0].words[2].braced);
}

TEST(ParserTest, BracedStringPreservesNewlines) {
    auto cmds = parse("set x {line1\nline2}");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[2].text, "line1\nline2");
}

TEST(ParserTest, BracedStringPreservesDollar) {
    auto cmds = parse("set x {$var}");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[2].text, "$var");
    EXPECT_TRUE(cmds[0].words[2].braced);
}

TEST(ParserTest, BracedStringPreservesBrackets) {
    auto cmds = parse("set x {[cmd]}");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[2].text, "[cmd]");
}

TEST(ParserTest, NestedBraces) {
    auto cmds = parse("set x {a {b c} d}");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[2].text, "a {b c} d");
}

TEST(ParserTest, DeeplyNestedBraces) {
    auto cmds = parse("set x {a {b {c}} d}");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[2].text, "a {b {c}} d");
}

TEST(ParserTest, EmptyBraces) {
    auto cmds = parse("set x {}");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[2].text, "");
    EXPECT_TRUE(cmds[0].words[2].braced);
}

// ============================================================
// Double-quoted strings
// ============================================================

TEST(ParserTest, QuotedStringPreservesSpaces) {
    auto cmds = parse("set x \"hello world\"");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[2].text, "hello world");
    EXPECT_FALSE(cmds[0].words[2].braced);
}

TEST(ParserTest, QuotedStringPreservesDollar) {
    // Dollar is preserved for later substitution during eval
    auto cmds = parse("set x \"$var\"");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[2].text, "$var");
    EXPECT_FALSE(cmds[0].words[2].braced);
}

TEST(ParserTest, QuotedStringWithBackslashEscape) {
    auto cmds = parse("set x \"hello\\nworld\"");
    ASSERT_EQ(cmds.size(), 1u);
    // Backslash sequences are preserved during parse, resolved during eval
    EXPECT_EQ(cmds[0].words[2].text, "hello\\nworld");
}

TEST(ParserTest, QuotedStringWithEscapedQuote) {
    auto cmds = parse("set x \"say \\\"hi\\\"\"");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[2].text, "say \\\"hi\\\"");
}

TEST(ParserTest, EmptyQuotedString) {
    auto cmds = parse("set x \"\"");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[2].text, "");
}

// ============================================================
// Bare words with special characters
// ============================================================

TEST(ParserTest, BareWordWithDollar) {
    auto cmds = parse("set x $y");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[2].text, "$y");
}

TEST(ParserTest, BareWordWithBrackets) {
    auto cmds = parse("set x [expr 1+2]");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 3u);
    EXPECT_EQ(cmds[0].words[2].text, "[expr 1+2]");
}

TEST(ParserTest, BareWordWithBackslashEscape) {
    auto cmds = parse("set x hello\\ world");
    ASSERT_EQ(cmds.size(), 1u);
    // "hello\ world" is a single word due to backslash-space
    EXPECT_EQ(cmds[0].words[2].text, "hello\\ world");
}

// ============================================================
// Line continuation (backslash-newline)
// ============================================================

TEST(ParserTest, LineContinuationInBareWord) {
    auto cmds = parse("set x hel\\\nlo");
    ASSERT_EQ(cmds.size(), 1u);
    // backslash-newline becomes a single space in bare word
    EXPECT_EQ(cmds[0].words[2].text, "hel lo");
}

TEST(ParserTest, LineContinuationBetweenWords) {
    auto cmds = parse("set \\\n  x 42");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 3u);
    EXPECT_EQ(cmds[0].words[1].text, "x");
}

// ============================================================
// Backslash substitution (unit tests for the function)
// ============================================================

TEST(BackslashSubstTest, NoEscapes) {
    EXPECT_EQ(backslashSubst("hello"), "hello");
}

TEST(BackslashSubstTest, Newline) {
    EXPECT_EQ(backslashSubst("a\\nb"), "a\nb");
}

TEST(BackslashSubstTest, Tab) {
    EXPECT_EQ(backslashSubst("a\\tb"), "a\tb");
}

TEST(BackslashSubstTest, CarriageReturn) {
    EXPECT_EQ(backslashSubst("a\\rb"), "a\rb");
}

TEST(BackslashSubstTest, Backslash) {
    EXPECT_EQ(backslashSubst("a\\\\b"), "a\\b");
}

TEST(BackslashSubstTest, OpenBrace) {
    EXPECT_EQ(backslashSubst("a\\{b"), "a{b");
}

TEST(BackslashSubstTest, CloseBrace) {
    EXPECT_EQ(backslashSubst("a\\}b"), "a}b");
}

TEST(BackslashSubstTest, DoubleQuote) {
    EXPECT_EQ(backslashSubst("a\\\"b"), "a\"b");
}

TEST(BackslashSubstTest, Dollar) {
    EXPECT_EQ(backslashSubst("a\\$b"), "a$b");
}

TEST(BackslashSubstTest, OpenBracket) {
    EXPECT_EQ(backslashSubst("a\\[b"), "a[b");
}

TEST(BackslashSubstTest, CloseBracket) {
    EXPECT_EQ(backslashSubst("a\\]b"), "a]b");
}

TEST(BackslashSubstTest, Bell) {
    EXPECT_EQ(backslashSubst("\\a"), "\a");
}

TEST(BackslashSubstTest, Backspace) {
    EXPECT_EQ(backslashSubst("\\b"), "\b");
}

TEST(BackslashSubstTest, FormFeed) {
    EXPECT_EQ(backslashSubst("\\f"), "\f");
}

TEST(BackslashSubstTest, VerticalTab) {
    EXPECT_EQ(backslashSubst("\\v"), "\v");
}

TEST(BackslashSubstTest, HexEscape) {
    EXPECT_EQ(backslashSubst("\\x41"), "A");
}

TEST(BackslashSubstTest, HexEscapeLowercase) {
    EXPECT_EQ(backslashSubst("\\x61"), "a");
}

TEST(BackslashSubstTest, OctalEscape) {
    EXPECT_EQ(backslashSubst("\\101"), "A");  // 0101 = 65 = 'A'
}

TEST(BackslashSubstTest, OctalEscapeThreeDigits) {
    EXPECT_EQ(backslashSubst("\\110"), "H");  // 0110 = 72 = 'H'
}

TEST(BackslashSubstTest, LineContinuation) {
    EXPECT_EQ(backslashSubst("hello\\\n   world"), "hello world");
}

TEST(BackslashSubstTest, LineContinuationNoTrailingSpace) {
    EXPECT_EQ(backslashSubst("hello\\\nworld"), "hello world");
}

TEST(BackslashSubstTest, MultipleEscapes) {
    EXPECT_EQ(backslashSubst("\\t\\n\\\\"), "\t\n\\");
}

TEST(BackslashSubstTest, UnknownEscapeConsumed) {
    // In Tcl, unknown escape sequences consume the backslash: \q -> q
    EXPECT_EQ(backslashSubst("\\q"), "q");
    EXPECT_EQ(backslashSubst("\\#"), "#");
}

// ============================================================
// Complex scripts
// ============================================================

TEST(ParserTest, MultiLineScript) {
    auto cmds = parse(
        "set x 1\n"
        "set y 2\n"
        "set z 3\n");
    ASSERT_EQ(cmds.size(), 3u);
    EXPECT_EQ(cmds[0].words[1].text, "x");
    EXPECT_EQ(cmds[1].words[1].text, "y");
    EXPECT_EQ(cmds[2].words[1].text, "z");
}

TEST(ParserTest, MixedQuotingStyles) {
    auto cmds = parse("cmd bare {braced} \"quoted\"");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 4u);
    EXPECT_EQ(cmds[0].words[1].text, "bare");
    EXPECT_FALSE(cmds[0].words[1].braced);
    EXPECT_EQ(cmds[0].words[2].text, "braced");
    EXPECT_TRUE(cmds[0].words[2].braced);
    EXPECT_EQ(cmds[0].words[3].text, "quoted");
    EXPECT_FALSE(cmds[0].words[3].braced);
}

TEST(ParserTest, IfLikeStructure) {
    auto cmds = parse("if {$x > 0} {\n  set y 1\n}");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 3u);
    EXPECT_EQ(cmds[0].words[0].text, "if");
    EXPECT_EQ(cmds[0].words[1].text, "$x > 0");
    EXPECT_TRUE(cmds[0].words[1].braced);
    EXPECT_EQ(cmds[0].words[2].text, "\n  set y 1\n");
    EXPECT_TRUE(cmds[0].words[2].braced);
}

TEST(ParserTest, ProcDefinition) {
    auto cmds = parse("proc add {a b} {\n  return [expr {$a + $b}]\n}");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 4u);
    EXPECT_EQ(cmds[0].words[0].text, "proc");
    EXPECT_EQ(cmds[0].words[1].text, "add");
    EXPECT_EQ(cmds[0].words[2].text, "a b");
    EXPECT_TRUE(cmds[0].words[2].braced);
}

TEST(ParserTest, CommentsInterleavedWithCommands) {
    auto cmds = parse(
        "# first comment\n"
        "set x 1\n"
        "# second comment\n"
        "set y 2\n");
    ASSERT_EQ(cmds.size(), 2u);
    EXPECT_EQ(cmds[0].words[1].text, "x");
    EXPECT_EQ(cmds[1].words[1].text, "y");
}

TEST(ParserTest, LeadingWhitespace) {
    auto cmds = parse("   set x 1");
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].words[0].text, "set");
}

TEST(ParserTest, BracedMultilineBody) {
    auto cmds = parse("foreach i {1 2 3} {\n  puts $i\n}");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 4u);  // foreach, i, {1 2 3}, {body}
    EXPECT_EQ(cmds[0].words[0].text, "foreach");
    EXPECT_EQ(cmds[0].words[1].text, "i");
    EXPECT_EQ(cmds[0].words[2].text, "1 2 3");
    EXPECT_TRUE(cmds[0].words[2].braced);
    EXPECT_EQ(cmds[0].words[3].text, "\n  puts $i\n");
    EXPECT_TRUE(cmds[0].words[3].braced);
}

TEST(ParserTest, QuotedStringWithBracketContainingQuotes) {
    // Quotes inside [...] inside "..." should not end the outer quote.
    // This is the ORFS log_cmd pattern: "$cmd[join [list "\"$arg\""]]"
    auto cmds = parse("set x \"hello[join [list \"world\"]]\"");
    ASSERT_EQ(cmds.size(), 1u);
    ASSERT_EQ(cmds[0].words.size(), 3u);
    EXPECT_EQ(cmds[0].words[0].text, "set");
    EXPECT_EQ(cmds[0].words[1].text, "x");
    // The quoted string should contain the full content including [...]
    EXPECT_FALSE(cmds[0].words[2].braced);
    EXPECT_NE(cmds[0].words[2].text.find("[join"), std::string::npos);
}
