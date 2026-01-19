#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <iomanip>

using std::uint8_t;
using String = std::string;

template <typename V>
using Vector = std::vector<V>;

enum class RawKind : uint8_t {
    SOF,
    EOF_,
    Newline,        // '\n'
    Tab,
    Space,
    Semicolon,      // ';'

    CommentLineStart,   // carries lexeme like "//"
    CommentBlockStart,  // carries lexeme like "/*"
    CommentBlockEnd,    // carries lexeme like "*/"
    Comment,
    Identifier,
    Number,
    String,
    Char,
    Text,

    Operator,       // single operators only, compoundOps will be determined in a later pass
    SpecialChar,    // Anything not clearly an identifier or Operator or Punctuation
    Punctuation,    // (), {}, [], :, ,, .
    Unknown,
    NoOp,
};

struct RawToken {
    RawKind kind {};
    String lexeme;
    int line;
    int column;
    int aux = -1;

    RawToken(): kind(RawKind::Unknown), line(-1), column(-1) {}

    explicit RawToken(RawKind t, char* v, int l, int c)
        : kind(t), line(l), column(c) {
            lexeme += v;
        }

    explicit RawToken(RawKind t, char v, int l, int c, int a = -1)
        : kind(t), line(l), column(c), aux(a) {
            lexeme += v;
        }

    explicit RawToken(RawKind k, String lx, int l, int c, int a = -1)
    : kind(k), lexeme(std::move(lx)), line(l), column(c), aux(a) {}

    
    RawToken(RawKind t): kind(t), line(-1), column(-1) {}
};

struct CommentPair {
    String start;   // "/*"
    String end;     // "*/"
    bool nestable = false; // C/C++ false, some langs true
};

struct CommentConfig {
    Vector<String> lineStarts;     // "#", "//", ";", "--", ...
    Vector<CommentPair> blockPairs; // { "/*","*/" }, { "{-","-}" }, ...
};



// If you don't already have this:
const char* rawKindToString(RawKind k);

// Escape lexeme so newlines/tabs are visible in debug output.
String escapeLexeme(const String& s);

void printRawTokens(const Vector<RawToken>& toks, std::ostream& os = std::cout);

// Optional: nice operator<< for RawToken
std::ostream& operator<<(std::ostream& os, const RawToken& t);






class Scanner {
    // Source state
public:
    const String source;
    size_t position = 0;
    size_t sourceLength = 0;
    char current = '\0';
    int line = 1;
    int column = 1;

    Vector<RawToken> rawTokens;
    RawToken noOpToken = RawToken(RawKind::NoOp);
    CommentConfig commentCfg;

    Scanner(const char* sourceFile, const CommentConfig& cfg);
    Scanner(String src, CommentConfig cfg);

    char next();
    bool hasNext();

    bool isWhiteSpace(char);
    bool isDigit(char);
    bool isTextBegin(char);
    bool isOperator(char);

    bool isCommentBegin(char);
    bool isPunctuation(char);
    bool isLetter(char);
    bool isSpecialChar(char);

    RawToken readIdentifier();
    RawToken readNumber();
    RawToken readText();

    RawToken readPunctuation();
    bool handleSpecialChar(char nextChar, char startChar, String& resultAccum);
    bool handleSpecialChars();
    void handleWhiteSpace();    
    RawToken readOperator();

    int matchBlockStartIndex() const;


    void readSource();


    // Call from your main scan loop *only when not inside a string/text literal*
    // Returns true if it consumed and emitted a comment delimiter token.
    bool tryScanCommentDelimiter();

    Vector<RawToken> scan();
private:
    // ---- helpers ----
    bool inBounds(size_t pos) const;

    bool matchAt(size_t pos, const String& s) const;

    void advanceN(size_t n);

    // ---- comment delimiter scanning ----

    bool tryScanCommentStart();

    bool tryScanCommentEnd();





    bool matchBlockEndIndex(int i) const;

    RawToken scanBlockComment(int pairIndex);

    RawToken scanLineComment(const String& startLexeme);
    
    bool tryScanComment();
};

using RunTimeError = std::runtime_error;


class ScannerError : public RunTimeError {
protected:
    String message;
    int line;
    int column;
    mutable String cache;

public:
    ScannerError() = default;
    ScannerError(const String& message, int line, int column);

    String errorString() const;

    const char* what() const noexcept override;

    ~ScannerError() = default;
};