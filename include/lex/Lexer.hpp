#pragma once
#include "lex/Structurizer.hpp"
#include "core/TypesFWD.hpp"


struct LexerConfig {
    std::unordered_set<String> knownTypes;    // Int, List, Dict...
    std::unordered_set<String> keywords;      // if, else, return...
    std::unordered_set<String> nativeClasses; // if you want separate from types (optional)
    std::unordered_set<String> nativeFuncs;   // optional, if you want pre-seeded functions
    std::unordered_set<String> primitiveCtors;
};


class Lexer {
public:
    Lexer(LexerConfig);
    Lexer();

    Vector<Token> lex(const Vector<RawToken>& raw, Vector<Token>&);

private:
    LexerConfig cfg;

    // state flags (ported from Tokenizer)
    bool insideParams = false;
    bool insideArgs   = false;
    bool insideClass  = false;
    int  classIndentLevel = 0;
    int  currentIndent = 0;

    bool pendingClass = false;
    int classBodyIndent = 0; // e.g. 4

    Vector<int> indentStack{0};


    std::unordered_set<String> classes;
    std::unordered_set<String> functions;
    std::unordered_set<String> primitives;

    // tables (ported)
    std::unordered_set<String> keywords;
    std::unordered_set<String> knownTypes;

    // helpers
    TokenType classifyIdentifier(
        const String& value,
        const Vector<Token>& out,
        const Vector<RawToken>& raw,
        size_t rawIndex
    );

    bool nextNonTriviaIsLParen(const Vector<RawToken>& raw, size_t i) const;
    bool prevTokenWasDot(const Vector<Token>& out) const;

    bool tryEmitCompoundOp(
        const Vector<RawToken>& raw, size_t& i,
        Vector<Token>& out
    );

    void onIndent(int);
    void onDedent(int);
};
