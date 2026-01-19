#include "lex/Scanner.hpp"



Vector<RawToken> Scanner::scan() {
    rawTokens.emplace_back(RawToken(RawKind::SOF));
    while (position < sourceLength) {
        
        handleWhiteSpace();
        if (tryScanComment()) {
            continue;
        }
        

        if (isDigit(current)) {
             rawTokens.emplace_back(readNumber());
             continue;
        }

        if (isTextBegin(current)) {
            rawTokens.emplace_back(readText());
            continue;
        }


        if (isOperator(current)) {
            rawTokens.emplace_back(readOperator());
            continue;
        }

        if (isPunctuation(current)) {
            rawTokens.emplace_back(readPunctuation());
            continue;
        }

        if (isLetter(current)) {
            rawTokens.emplace_back(readIdentifier());
            continue;

        }

        if (!hasNext()) {
            break;
        }
        rawTokens.emplace_back( RawToken(RawKind::Unknown, String(1, current), line, column));
        advanceN(1);
        
    }

    String eof = "EOF";
    rawTokens.emplace_back(RawToken(RawKind::EOF_, eof, line, column));
    return rawTokens;
}

Scanner::Scanner(String src, CommentConfig cfg)
    : source(std::move(src)), commentCfg(std::move(cfg)) {
    sourceLength = source.size();
    current = (sourceLength > 0) ? source[0] : '\0';
}

Scanner::Scanner(const char* src, const CommentConfig& cfg) : Scanner(String(src), cfg) {}

bool Scanner::hasNext() { return position < sourceLength; }


char Scanner::next() {
    if (!hasNext()) return '\0';
    position++;
    column++;
    current = source[position];
    return current;
}

bool Scanner::isWhiteSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

bool Scanner::isDigit(char c) {
    return std::isdigit(static_cast<unsigned char>(c));
}


void Scanner::handleWhiteSpace() {
    while (isWhiteSpace(current) && hasNext()) {
        if (current == '\n') {
            int numNewLines = 0;
            while (current == '\n') {
                numNewLines += 1;
                line++;
                column = 0;
                next();
            }
            rawTokens.emplace_back(RawKind::Newline, "\n", line, column, numNewLines);            
        }
        if (current == '\t') {
            int numTabs = 0;
            while (current == '\t') {
                numTabs += 1;
                next();
            }
            rawTokens.emplace_back(RawToken(RawKind::Tab, "    ", line, column, numTabs));
        }
        
        if (current == ' ') {
            int numSpaces = 0;
            while (current == ' ') {
                numSpaces += 1;
                next();
            }
            
            rawTokens.emplace_back(RawToken(RawKind::Space, " ", line, column, numSpaces));
            
        }
    }
}


bool Scanner::isSpecialChar(char c) {
    switch (c) {
        case 'n':
        case 't':
        case 'r':
        case '\\':
        case '\'':
        case '"':
            return true;
        default:
            return false;
    }

}

bool Scanner::handleSpecialChar(char nextChar, char startChar, String& resultAccum) {
    if (nextChar == startChar) { return true; }
    if (isSpecialChar(nextChar)) {
        resultAccum += nextChar;
        return true;
    }
    return false;
}

bool Scanner::isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || 
        c == '%' || c == '=' || c == '<' || c == '>' || 
        c == '&' || c == '|' || c == '!';
}

bool Scanner::isPunctuation(char c) {
    return c == ':' || c == ';' || c == '.' || c == ',' || c == '$' || c == '@' || c == '?'
        || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}';
}

bool Scanner::isTextBegin(char c) {
    return c == '\'' || c == '"' || c == '`';
}

bool Scanner::isLetter(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool Scanner::isCommentBegin(char c) {
    for (auto& starts : commentCfg.lineStarts) {
        if (starts == std::to_string(c)) {
            return true;
        }
    }
    for (auto& pair : commentCfg.blockPairs) {
        if (pair.start == std::to_string(c)) {
            return true;
        }
    }

    return false;
}


RawToken Scanner::readOperator() {
    if (!isOperator(current)) {
        throw ScannerError("Not Operator in readOperator -> " + std::to_string(current), line, column);
    }

    auto t = RawToken(RawKind::Operator, current, line, column);
    next();
    return t;
}

RawToken Scanner::readPunctuation() {
    if (!isPunctuation(current)) {
        throw ScannerError("Not Punctuation in readPunctuation -> " + std::to_string(current), line, column);
    }
    auto t = RawToken(RawKind::Punctuation, current, line, column);
    next();
    return t;
}

RawToken Scanner::readText() {
    if (!isTextBegin(current)) {
        throw ScannerError("Not Text in readText -> " + std::to_string(current), line, column);
    }

    char startChar = current;
    int startColumn = column;
    int startLine = line;
    String result;
    RawKind type = current == '\'' ? RawKind::Char : current == '"' ? RawKind::String : RawKind::Text;
    next();

    while (hasNext() && current != startChar) {
        if (current == '\\') { // Handle escape sequences
            next();
            const char escaped = current;
            if (isSpecialChar(escaped)) {
                if (escaped == startChar) result += startChar;
                    else throw ScannerError("Unknown escape in string literal", line, column);
            }
            next();
            continue;

        } else {
            result += current;
        }

        next();
    }

    if (hasNext() && current != startChar) {
        throw ScannerError("Unmatched quote for " + std::to_string(startChar), line, column);
    }

    next();

    return RawToken(type, result, startLine, startColumn);
}

RawToken Scanner::readNumber() {
    if (!isDigit(current)) {
        throw ScannerError("Not Digit in readNumber -> " + std::to_string((int)current), line, column);
    }

    const int startLine = line;
    const int startCol  = column;

    String number;
    bool seenDot = false;
    while (position < sourceLength) {
        if (isDigit(current)) {
            number += current;
            next();
            continue;
        }

        if (!seenDot && current == '.' && (position + 1) < sourceLength && isDigit(source[position + 1]))
        {
            seenDot = true;
            number += current;
            next();
            continue;
        }

        break;
    }

    return RawToken(RawKind::Number, number, startLine, startCol);
}

RawToken Scanner::readIdentifier() {
    if (!isLetter(current)) throw ScannerError("Not Letter in readIdentifier -> " + std::to_string(current), line, column);

    int startCol = column;
    int startLine = line;
    String accum;

    while (true) {
        if (isLetter(current) || isDigit(current)) { accum += current; }
        else if (current == '_') { accum += current; }
        else break;

        if (!hasNext()) break;
        next();
    }

    return RawToken(RawKind::Identifier, accum, startLine, startCol);
}

bool Scanner::handleSpecialChars() {
    if (isSpecialChar(current)){
        String accum;
        accum += current;
        while (hasNext() && isSpecialChar(current)) {
            if (handleSpecialChar(source[position], current, accum)) {
                next();
            }
        }
        return true;
    }
    return false;
}

int Scanner::matchBlockStartIndex() const {
    int best = -1;
    size_t bestLen = 0;
    for (int i = 0; i < (int)commentCfg.blockPairs.size(); ++i) {
        const auto& st = commentCfg.blockPairs[i].start;
        if (matchAt(position, st) && st.size() > bestLen) {
            best = i;
            bestLen = st.size();
        }
    }
    return best;
}




bool Scanner::tryScanCommentDelimiter() {
    // Prefer starts over ends so "/*" is treated as start, not '/' then '*'
    if (tryScanCommentStart()) return true;
    if (tryScanCommentEnd())   return true;
    return false;
}
 
bool Scanner::inBounds(size_t pos) const { return pos < sourceLength; }

bool Scanner::matchAt(size_t pos, const String& s) const {
    if (s.empty()) return false;
    if (pos + s.size() > sourceLength) return false;
    // std::string::compare avoids allocations
    return source.compare(pos, s.size(), s) == 0;
}

void Scanner::advanceN(size_t n) {
    // Advance n characters, updating column (and line if you ever advance across '\n')
    // For comment delimiters, n is small and usually has no '\n'.
    for (size_t i = 0; i < n && position < sourceLength; ++i) {
        ++position;
        ++column;
    }
    current = (position < sourceLength) ? source[position] : '\0';
}

// ---- comment delimiter scanning ----

bool Scanner::tryScanCommentStart() {
    if (!inBounds(position)) return false;

    // Find the best (longest) match among ALL start delimiters at this position.
    enum class StartKind { None, Line, Block };
    StartKind bestKind = StartKind::None;
    size_t bestLen = 0;
    int bestBlockIndex = -1;
    String bestLexeme;

    // 1) Line starts
    for (const auto& ls : commentCfg.lineStarts) {
        if (ls.size() >= bestLen && matchAt(position, ls)) {
            // Longest match wins; if equal length, line vs block tie-break is arbitrary.
            bestKind = StartKind::Line;
            bestLen = ls.size();
            bestLexeme = ls;
            bestBlockIndex = -1;
        }
    }

    // 2) Block starts
    for (int i = 0; i < (int)commentCfg.blockPairs.size(); ++i) {
        const auto& bs = commentCfg.blockPairs[i].start;
        if (bs.size() >= bestLen && matchAt(position, bs)) {
            bestKind = StartKind::Block;
            bestLen = bs.size();
            bestLexeme = bs;
            bestBlockIndex = i;
        }
    }

    if (bestKind == StartKind::None) return false;

    const int startLine = line;
    const int startCol  = column;

    if (bestKind == StartKind::Line) {
        rawTokens.emplace_back(RawKind::CommentLineStart, bestLexeme, startLine, startCol, -1);
        advanceN(bestLen);
        return true;
    }

    // Block start
    rawTokens.emplace_back(RawKind::CommentBlockStart, bestLexeme, startLine, startCol, bestBlockIndex);
    advanceN(bestLen);
    return true;
}

bool Scanner::tryScanCommentEnd() {
    if (!inBounds(position)) return false;

    // Find the best (longest) matching block END delimiter at this position.
    size_t bestLen = 0;
    int bestBlockIndex = -1;
    String bestLexeme;

    for (int i = 0; i < (int)commentCfg.blockPairs.size(); ++i) {
        const auto& be = commentCfg.blockPairs[i].end;
        if (be.size() >= bestLen && matchAt(position, be)) {
            bestLen = be.size();
            bestLexeme = be;
            bestBlockIndex = i;
        }
    }

    if (bestBlockIndex < 0) return false;

    const int endLine = line;
    const int endCol  = column;

    rawTokens.emplace_back(RawKind::CommentBlockEnd, bestLexeme, endLine, endCol, bestBlockIndex);
    advanceN(bestLen);
    return true;
}

const char* rawKindToString(RawKind k) {
    switch (k) {
        case RawKind::SOF: return "SOF";
        case RawKind::EOF_: return "EOF_";
        case RawKind::Newline: return "Newline";
        case RawKind::Tab: return "Tab";
        case RawKind::Space: return "Space";
        case RawKind::Semicolon: return "Semicolon";

        case RawKind::CommentLineStart: return "CommentLineStart";
        case RawKind::CommentBlockStart: return "CommentBlockStart";
        case RawKind::CommentBlockEnd: return "CommentBlockEnd";

        case RawKind::Identifier: return "Identifier";
        case RawKind::Number: return "Number";
        case RawKind::String: return "String";
        case RawKind::Char: return "Char";
        case RawKind::Text: return "Text";

        case RawKind::Operator: return "Operator";
        case RawKind::Punctuation: return "Punctuation";
        case RawKind::SpecialChar: return "SpecialChar";
        case RawKind::Unknown: return "Unknown";
        case RawKind::NoOp: return "NoOp";
        case RawKind::Comment: return "Comment";
        default: return "<RawKind?>";
    }
}

bool Scanner::matchBlockEndIndex(int i) const {
    return matchAt(position, commentCfg.blockPairs[i].end);
}

RawToken Scanner::scanBlockComment(int pairIndex) {
    const int startLine = line;
    const int startCol  = column;

    const auto& pair = commentCfg.blockPairs[pairIndex];

    // consume start delimiter (e.g. "/*")
    advanceN(pair.start.size());

    String text;
    int depth = 1;

    while (position < sourceLength) {
        // If nestable, detect nested start
        if (pair.nestable) {
            int nested = matchBlockStartIndex();
            if (nested == pairIndex) {
                advanceN(commentCfg.blockPairs[nested].start.size());
                depth++;
                continue;
            }
        }

        // Detect end for this pair
        if (matchBlockEndIndex(pairIndex)) {
            advanceN(pair.end.size());
            depth--;
            if (depth == 0) {
                return RawToken(RawKind::Comment, text, startLine, startCol, pairIndex);
            }
            continue;
        }

        // Normal char in comment body
        text += current;
        next(); // use next() for newline tracking
    }

    throw ScannerError("Unterminated block comment", startLine, startCol);
}

RawToken Scanner::scanLineComment(const String& startLexeme) {
    const int startLine = line;
    const int startCol  = column;

    // consume delimiter (e.g. "#", "//")
    advanceN(startLexeme.size());

    String text;
    while (position < sourceLength && current != '\n') {
        text += current;
        next(); // IMPORTANT: use next() so line/col stays correct
    }
    if (!text.empty() && text[0] == ' ') text.erase(0, 1);

    // Don't consume '\n' here — let handleWhiteSpace() emit Newline token
    return RawToken(RawKind::Comment, text, startLine, startCol, -1);
}

bool Scanner::tryScanComment() {
    if (!inBounds(position)) return false;

    // Find best start match (longest) across line + block starts
    enum class StartKind { None, Line, Block };
    StartKind bestKind = StartKind::None;
    size_t bestLen = 0;
    int bestBlockIndex = -1;
    String bestLexeme;

    for (const auto& ls : commentCfg.lineStarts) {
        if (matchAt(position, ls) && ls.size() > bestLen) {
            bestKind = StartKind::Line;
            bestLen = ls.size();
            bestLexeme = ls;
            bestBlockIndex = -1;
        }
    }

    for (int i = 0; i < (int)commentCfg.blockPairs.size(); ++i) {
        const auto& bs = commentCfg.blockPairs[i].start;
        if (matchAt(position, bs) && bs.size() > bestLen) {
            bestKind = StartKind::Block;
            bestLen = bs.size();
            bestLexeme = bs;
            bestBlockIndex = i;
        }
    }

    if (bestKind == StartKind::None) return false;

    if (bestKind == StartKind::Line) {
        rawTokens.emplace_back(scanLineComment(bestLexeme));
        return true;
    }

    if (isWhiteSpace(current)) {
        handleWhiteSpace();
    }
    
    rawTokens.emplace_back(scanBlockComment(bestBlockIndex));
    return true;
}


String escapeLexeme(const String& s) {
    String out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            default:
                if (static_cast<unsigned char>(c) < 32) {
                    // Other control chars
                    out += "\\x";
                    const char* hex = "0123456789ABCDEF";
                    out += hex[(c >> 4) & 0xF];
                    out += hex[c & 0xF];
                } else {
                    out += c;
                }
        }
    }
    return out;
}

void printRawTokens(const Vector<RawToken>& toks, std::ostream& os) {
    os << "RawTokens (" << toks.size() << ")\n";
    os << "------------------------------------------------------------\n";
    for (size_t i = 0; i < toks.size(); ++i) {
        const auto& t = toks[i];
        os << std::setw(4) << i << "  "
           << std::setw(18) << rawKindToString(t.kind)
           << "  (" << t.line << ":" << t.column << ")";

        // aux is optional; print only if you use it
        if (t.aux != -1) {
            os << "  aux=" << t.aux;
        }

        if (!t.lexeme.empty()) {
            os << "  \"" << escapeLexeme(t.lexeme) << "\"";
        }

        os << "\n";
    }
    os << "------------------------------------------------------------\n";
}

std::ostream& operator<<(std::ostream& os, const RawToken& t) {
    os << rawKindToString(t.kind) << " (" << t.line << ":" << t.column << ")";
    if (t.aux != -1) os << " aux=" << t.aux;
    if (!t.lexeme.empty()) os << " \"" << escapeLexeme(t.lexeme) << "\"";
    return os;
}

ScannerError::ScannerError(const String& m, int l, int c): RunTimeError(m), message(m), line(l), column(c) {}

String ScannerError::errorString() const {
    return "ScannerError: " + message + " on Line: " + std::to_string(line) + ",  Column: " + std::to_string(column);
}

const char* ScannerError::what() const noexcept {
    cache = errorString();
    return cache.c_str();
};