#include "lex/Lexer.hpp"
#include "core/types.h"

static bool isCompoundPair(char a, char b) {
    switch (a) {
        case '<': return b == '=' || b == '<';
        case '>': return b == '=' || b == '>';
        case '=': return b == '=';
        case '!': return b == '=';
        case '+': return b == '=' || b == '+';
        case '-': return b == '=' || b == '-' || b == '>';
        case '*': return b == '=' || b == '*';
        case '/': return b == '=' || b == '/';
        case '&': return b == '&' || b == '=';
        case '|': return b == '|' || b == '=';
        case ':': return b == ':' || b == '=';
        default: return false;
    }
}

static bool isOpLike(const RawToken& t) {
    // In your raw design, both RawKind::Operator and some punctuation (:) belong in compound pairs.
    return t.kind == RawKind::Operator || t.kind == RawKind::Punctuation;
}

bool Lexer::tryEmitCompoundOp(const Vector<RawToken>& raw, size_t& i, Vector<Token>& out) {
    if (i + 1 >= raw.size()) return false;
    const RawToken& a = raw[i];
    const RawToken& b = raw[i + 1];

    if (!isOpLike(a) || !isOpLike(b)) return false;
    if (a.lexeme.empty() || b.lexeme.empty()) return false;

    const char ca = a.lexeme[0];
    const char cb = b.lexeme[0];
    if (!isCompoundPair(ca, cb)) return false;

    String two;
    two += ca;
    two += cb;

    // special case: ":=" becomes VarAssignment
    if (two == ":=") {
        out.emplace_back(TokenType::VarAssignment, two, a.line, a.column);
        i += 2;
        return true;
    }

    // else: treat as Operator (your old tokenizer did)
    out.emplace_back(TokenType::Operator, two, a.line, a.column);
    i += 2;
    return true;
}


static bool isTriviaRaw(const RawToken& t) {
    return t.kind == RawKind::Space || t.kind == RawKind::Tab;
}

bool Lexer::nextNonTriviaIsLParen(const Vector<RawToken>& raw, size_t i) const {
    for (size_t j = i + 1; j < raw.size(); ++j) {
        if (isTriviaRaw(raw[j])) continue;
        return raw[j].kind == RawKind::Punctuation && raw[j].lexeme == "(";
    }
    return false;
}

static bool nextNonTriviaIsDotOrColonColon(const Vector<RawToken>& raw, size_t i) {
    // '.' is punctuation, '::' would be merged into Operator token by tryEmitCompoundOp,
    // but the ChainEntryPoint check happens BEFORE emitting identifier token, so we must look raw-side too.
    for (size_t j = i + 1; j < raw.size(); ++j) {
        if (isTriviaRaw(raw[j])) continue;
        if (raw[j].kind == RawKind::Punctuation && raw[j].lexeme == ".") return true;
        // raw ':' ':' (before merge) case:
        if (raw[j].kind == RawKind::Punctuation && raw[j].lexeme == ":" &&
            (j + 1 < raw.size()) && raw[j + 1].kind == RawKind::Punctuation && raw[j + 1].lexeme == ":")
            return true;
        return false;
    }
    return false;
}

bool Lexer::prevTokenWasDot(const Vector<Token>& out) const {
    if (out.empty()) return false;
    return out.back().value == ".";
}


TokenType Lexer::classifyIdentifier(const String& value, const Vector<Token>& out, const Vector<RawToken>& raw, size_t rawIndex) {
    TokenType type = TokenType::Variable;

    auto lastType = [&]() -> TokenType { return out.empty() ? TokenType::Unknown : out.back().type; };
    auto lastVal  = [&]() -> String    { return out.empty() ? "" : out.back().value; };

    const bool nextIsCall = nextNonTriviaIsLParen(raw, rawIndex);

    // 0) Dot always wins: .name( => method call
    if (prevTokenWasDot(out) && nextIsCall) {
        insideArgs = true;
        return TokenType::ClassMethodCall;
    }

    // 1) Class definition keyword
    if (value == "Class") {
        return TokenType::ClassDef;
    }

    // 2) After ClassDef: class name (also arms "pendingClass")
    if (!out.empty() && lastType() == TokenType::ClassDef) {
        classes.insert(value);
        pendingClass = true;              // <- arm it HERE
        return TokenType::ClassRef;
    }

    // 3) def/function keywords
    if (value == "function" || value == "def") {
        insideParams = true;
        return insideClass ? TokenType::ClassMethodDef : TokenType::FunctionDef;
    }

    // 4) name after def
    if (!out.empty() && (lastType() == TokenType::FunctionDef || lastType() == TokenType::ClassMethodDef)) {
        functions.insert(value);
        return insideClass ? TokenType::ClassMethodRef : TokenType::FunctionRef;
    }

    // 5) literals / word-ops
    if (value == "true" || value == "false") return TokenType::Bool;
    if (value == "and" || value == "or" || value == "not") return TokenType::Operator;

    // 6) var/const
    if (value == "var" || value == "const") return TokenType::VarDeclaration;

    // 7) keyword bucket
    if (keywords.count(value)) return TokenType::Keyword;

    // 8) Primitive ctor call: String(1) => FunctionCall
    if (primitives.count(value) && nextIsCall) {
        insideArgs = true;
        return TokenType::FunctionCall;
    }

    // 9) Type token for annotations: x: String
    // Only classify as Type when NOT immediately being called.
    if (knownTypes.count(value) && !nextIsCall) {
        return TokenType::Type;
    }

    // 10) Class ctor call: List(1), Square(5), Dict()
    if ((cfg.nativeClasses.count(value) || classes.count(value)) && nextIsCall) {
        insideArgs = true;
        return TokenType::ClassCall;
    }

    // 11) Otherwise, any identifier followed by '(' is a normal function call
    if (nextIsCall) {
        insideArgs = true;
        return TokenType::FunctionCall;
    }

    // 12) Argument / Parameter modes (must run before generic call classification)
    if (insideArgs && !out.empty() && out.back().type == TokenType::Punctuation && !nextIsCall)
        return TokenType::Argument;
    if (insideParams && !nextIsCall)
        return TokenType::Parameter;


    // 13) function ref / class ref if known
    if (functions.count(value)) type = TokenType::FunctionRef;
    if (classes.count(value))   type = TokenType::ClassRef;


    if (type == TokenType::Variable && value == "null") {
        return TokenType::String;
    }
    // 14) ChainEntryPoint
    if (type == TokenType::Variable && lastVal() != "." && nextNonTriviaIsDotOrColonColon(raw, rawIndex)) {
        return TokenType::ChainEntryPoint;
    }

    // 15) demotions (same as before)
    if (type == TokenType::FunctionRef || type == TokenType::ClassMethodRef) {
        if (nextNonTriviaIsDotOrColonColon(raw, rawIndex) ||
            (!out.empty() && out.back().type == TokenType::VarDeclaration) ||
            (!out.empty() && out.back().type == TokenType::VarAssignment && nextNonTriviaIsDotOrColonColon(raw, rawIndex))) {
            return TokenType::Variable;
        }
    }

    return type;
}





Vector<Token> Lexer::lex(const Vector<RawToken>& raw, Vector<Token>& out) {
    
    out.reserve(raw.size());
    String s = "SOF";
    String e = "EOF";
    out.emplace_back(TokenType::SOF_Token, s, 0, 0);
    classes = cfg.nativeClasses;   // seed from runtime/builtins
    functions = cfg.nativeFuncs; 
    keywords = cfg.keywords;
    primitives = cfg.primitiveCtors;
    knownTypes = cfg.knownTypes;
    auto eofToken = RawToken(RawKind::EOF_, e, 0, 0);
    
    for (size_t i = 0; i < raw.size(); ) {
        const RawToken& t = raw[i];
        if (t.kind == RawKind::SOF) {
            ++i;
            continue;
        }
        // drop trivia
        if (t.kind == RawKind::Space || t.kind == RawKind::Tab) { ++i; continue; }

        // comments: Merk previously skipped them
        if (t.kind == RawKind::Comment || t.kind == RawKind::CommentLineStart ||
            t.kind == RawKind::CommentBlockStart || t.kind == RawKind::CommentBlockEnd) {
            ++i;
            continue;
        }

        if (t.kind == RawKind::Newline) {
            out.emplace_back(TokenType::Newline, "NewLine", t.line, t.column);
            ++i;
            continue;
        }


        // structure tokens
        // if (t.kind == RawKind::Indent) {
        //     out.emplace_back(TokenType::Indent, "->", t.line, t.column);
        //     currentIndent = (int)t.aux;

        //     if (pendingClass) {
        //         insideClass = true;
        //         classBodyIndent = currentIndent; // typically 4
        //         pendingClass = false;
        //     }

        //     ++i;
        //     continue;
        // }

        // if (t.kind == RawKind::Dedent) {
        //     out.emplace_back(TokenType::Dedent, "<-", t.line, t.column);
        //     currentIndent = (int)t.aux;

        //     // Exit class only if we dedent below class body indent
        //     if (insideClass && currentIndent < classBodyIndent) {
        //         insideClass = false;
        //         classBodyIndent = 0;
        //     }

        //     ++i;
        //     continue;
        // }


        if (t.kind == RawKind::Indent) {
            out.emplace_back(TokenType::Indent, "->", t.line, t.column);

            const int newIndent = (int)t.aux;

            // AUX is the truth:
            currentIndent = newIndent;

            // Validate that aux represents an increase
            // (Structurizer should only emit Indent when indent increases)
            if (!indentStack.empty() && newIndent <= indentStack.back()) {
                throw RunTimeError(
                    "Lexer: Indent aux not increasing. aux=" + std::to_string(newIndent) +
                    " prev=" + std::to_string(indentStack.back()) +
                    " at " + std::to_string(t.line) + ":" + std::to_string(t.column)
                );
            }

            indentStack.push_back(newIndent);

            // If we just parsed "Class Name" earlier, the first indent begins the class body.
            if (pendingClass) {
                insideClass = true;
                classBodyIndent = newIndent;
                pendingClass = false;
            }

            ++i;
            continue;
        }

        if (t.kind == RawKind::Dedent) {
            out.emplace_back(TokenType::Dedent, "<-", t.line, t.column);

            const int newIndent = (int)t.aux;

            // AUX is the truth:
            currentIndent = newIndent;

            // Validate: aux must not be greater than current stack top
            if (indentStack.empty() || newIndent > indentStack.back()) {
                throw RunTimeError(
                    "Lexer: Dedent aux larger than current indent. aux=" + std::to_string(newIndent) +
                    " prev=" + (indentStack.empty() ? String("EMPTY") : std::to_string(indentStack.back())) +
                    " at " + std::to_string(t.line) + ":" + std::to_string(t.column)
                );
            }

            // Pop until stack matches aux (or we reach base)
            while (indentStack.size() > 1 && indentStack.back() > newIndent) {
                indentStack.pop_back();
            }

            // Validate: aux must match a previous indent level exactly
            if (indentStack.back() != newIndent) {
                throw RunTimeError(
                    "Lexer: Dedent aux does not match any prior indent level. aux=" + std::to_string(newIndent) +
                    " top=" + std::to_string(indentStack.back()) +
                    " at " + std::to_string(t.line) + ":" + std::to_string(t.column)
                );
            }

            // Exit class only if we dedent BELOW the class body indent
            if (insideClass && newIndent < classBodyIndent) {
                insideClass = false;
                classBodyIndent = 0;
            }

            ++i;
            continue;
        }


        // try compound op merge
        if (tryEmitCompoundOp(raw, i, out)) continue;

        // numbers/strings
        if (t.kind == RawKind::Number) {
            out.emplace_back(TokenType::Number, t.lexeme, t.line, t.column);
            ++i;
            continue;
        }
        if (t.kind == RawKind::String) {
            out.emplace_back(TokenType::String, t.lexeme, t.line, t.column);
            ++i;
            continue;
        }
        if (t.kind == RawKind::Char) {
            out.emplace_back(TokenType::Char, t.lexeme, t.line, t.column);
            ++i;
            continue;
        }
        if (t.kind == RawKind::Text) {
            out.emplace_back(TokenType::Text, t.lexeme, t.line, t.column);
            ++i;
            continue;
        }

        // identifiers
        if (t.kind == RawKind::Identifier) {
            TokenType tt = classifyIdentifier(t.lexeme, out, raw, i);
            out.emplace_back(tt, t.lexeme, t.line, t.column);
            ++i;
            continue;
        }

        // single '=' becomes VarAssignment (ported)
        if (t.kind == RawKind::Operator && t.lexeme == "=") {
            out.emplace_back(TokenType::VarAssignment, "=", t.line, t.column);
            ++i;
            continue;
        }

        // operator / punctuation passthrough
        if (t.kind == RawKind::Operator) {
            out.emplace_back(TokenType::Operator, t.lexeme, t.line, t.column);
            ++i;
            continue;
        }

        if (t.kind == RawKind::Punctuation) {
            // preserve your special bracket tokens if you still want them:
            if (t.lexeme == "[") out.emplace_back(TokenType::LeftBracket, "[", t.line, t.column);
            else if (t.lexeme == "]") out.emplace_back(TokenType::RightBracket, "]", t.line, t.column);
            else out.emplace_back(TokenType::Punctuation, t.lexeme, t.line, t.column);

            // close paren resets args/params like your old tokenizer
            if (t.lexeme == ")") { insideArgs = false; insideParams = false; }
            ++i;
            continue;
        }

        

        // raw EOF
        if (t.kind == RawKind::EOF_) break;

        // unknown
        out.emplace_back(TokenType::Unknown, t.lexeme, t.line, t.column);
        ++i;
    }
    
    // EOF    
    const RawToken& last = raw.empty() ? RawToken(RawKind::EOF_, e, 0, 0) : raw.back();
    out.emplace_back(TokenType::EOF_Token, e, last.line, last.column);
    return out;
}

Lexer::Lexer(LexerConfig lxCfg) { cfg = lxCfg; }

Lexer::Lexer() {}