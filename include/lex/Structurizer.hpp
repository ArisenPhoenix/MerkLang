#pragma once

#include <cstdint>
#include "lex/Scanner.hpp"

enum class TokKind : uint16_t {
    SOF, EOF_,
    Identifier, Number, String, Char, Text,
    Operator, Punctuation,

    Newline,
    Indent,
    Dedent,

    CommentLineStart,
    CommentBlockStart,
    CommentBlockEnd,
    Comment,

    Space, 
    Tab,
};


struct LayoutConfig {
    int tabWidth = 4;
    bool tabsAllowed = true;
    bool spacesAllowed = true;

    bool keepWhitespaceTokens = false;   // drop Space/Tab tokens unless debugging
    bool keepComments = true;            // keep comment tokens or drop them
    bool commentsAreLineContent = false; // for indentation: whether comment-only lines count (usually false)

    // “line continuation” / newline suppression (optional for now)
    bool backslashContinuation = false;
    bool parenContinuation = true;       // ignore newline inside (), [], {}
};


class Structurizer {
public:
    Structurizer(LayoutConfig cfg) : cfg_(cfg) {}

    Vector<RawToken> structurize(const Vector<RawToken>& in);

private:
    LayoutConfig cfg_;
    Vector<int> indentStack_;

    bool isCommentToken(const RawToken& t) const;

    void applyIndent(int indent, const RawToken& atToken, Vector<RawToken>& out);
};
