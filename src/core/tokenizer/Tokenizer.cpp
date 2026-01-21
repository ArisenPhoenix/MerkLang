#include "core/Tokenizer.hpp"
#include "core/TypesFWD.hpp"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"
#include "core/errors.h"
#include <filesystem>
#include <fstream>
#include <iostream>


void Tokenizer::read() {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Cannot open file '" + filePath + "'. Please ensure the file exists and is accessible.");
    }
    std::ostringstream content;
    content << file.rdbuf();
    source = content.str();
}


Tokenizer::Tokenizer(const String& source, bool readFile): source(readFile ? "" : source), filePath(readFile ? source : "") {
    if (readFile) {
        read();
    }
    
}



void Tokenizer::printTokens(bool colored) const {
    for (const auto& token : tokens) {
        String tok = colored ? highlight("Token", Colors::green) : "Token";
        std::cout << tok + "(Type: " << tokenTypeToString(token.type, colored)
                  << ", Value: " << token.value
                  << ", Line: " << token.line
                  << ", Column: " << token.column << ")" << std::endl;
    }
}



Vector<Token> Tokenizer::tokenize(LexerConfig& lexCfg) {
    return lex(lexCfg);
}

LexerConfig& handleLex(LexerConfig& lexCfg) {
    if (lexCfg.keywords.size() == 0) {
        lexCfg.keywords         = {"if","elif","else","from","as","import","while","continue","break","return","for","throw"};
    }

    if (lexCfg.nativeClasses.size() == 0) {
        lexCfg.nativeClasses    = {"List","Dict","Set","Array", "Http","File"}; 
    }

    if (lexCfg.primitiveCtors.size() == 0) {
        lexCfg.primitiveCtors   = {"Int","Float","Long","Bool","String"};
    }

    auto& primitives = lexCfg.primitiveCtors;
    auto& classes = lexCfg.nativeClasses;

    static const std::unordered_set<String> knownTypes = [primitives, classes] 
    {
        std::unordered_set<String> s;
        s.insert(primitives.begin(), primitives.end());
        s.insert(classes.begin(), classes.end());
        return s;
    }();

    lexCfg.knownTypes = knownTypes;

    return lexCfg;
}


CommentConfig handleScannerConfig() {
    CommentConfig cfg{{"#"}, {}};
    return cfg;
}

LayoutConfig handleLayoutConfig() {
    LayoutConfig lcfg {};
    return lcfg;
}

Vector<Token> Tokenizer::lex(LexerConfig& lexCfg) {
    CommentConfig cfg       = handleScannerConfig();
    Scanner scanner(source, cfg);
    auto toks = scanner.scan();

    LayoutConfig lcfg       = handleLayoutConfig();
    Structurizer structurizer(lcfg);
    auto structs = structurizer.structurize(toks);
    std::cout << std::endl;

    auto& v = handleLex(lexCfg);
    Lexer lexer(v);
    lexer.lex(structs, tokens);


    debugLog(true, "LEXER OUTPUT: ");
    bool colored = true;

    debugLog(true, "TOKENIZER OUTPUT: ");
    if (tokens.size() < 1) {
        debugLog(true, "No Tokens: ");
    }
    return tokens;

}