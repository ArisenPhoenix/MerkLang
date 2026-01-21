// #include "core/Tokenizer.hpp"
// #include "utilities/helper_functions.h"
// #include "utilities/debugging_functions.h"

// LexerConfig& handleLex(LexerConfig& lexCfg) {
//     if (lexCfg.keywords.size() == 0) {
//         lexCfg.keywords         = {"if","elif","else","from","as","import","while","continue","break","return","for","throw"};
//     }

//     if (lexCfg.nativeClasses.size() == 0) {
//         lexCfg.nativeClasses    = {"List","Dict","Set","Array", "Http","File"}; 
//     }

//     if (lexCfg.primitiveCtors.size() == 0) {
//         lexCfg.primitiveCtors   = {"Int","Float","Long","Bool","String"};
//     }

//     auto& primitives = lexCfg.primitiveCtors;
//     auto& classes = lexCfg.nativeClasses;

//     static const std::unordered_set<String> knownTypes = [primitives, classes] 
//     {
//         std::unordered_set<String> s;
//         s.insert(primitives.begin(), primitives.end());
//         s.insert(classes.begin(), classes.end());
//         return s;
//     }();

//     lexCfg.knownTypes = knownTypes;

//     return lexCfg;
// }


// CommentConfig handleScannerConfig() {
//     CommentConfig cfg{{"#"}, {}};
//     return cfg;
// }

// LayoutConfig handleLayoutConfig() {
//     LayoutConfig lcfg {};
//     return lcfg;
// }

// Vector<Token> Tokenizer::lex(LexerConfig& lexCfg) {
//     CommentConfig cfg       = handleScannerConfig();
//     Scanner scanner(source, cfg);
//     auto toks = scanner.scan();
//     // printRawTokens(toks, std::cout);


//     LayoutConfig lcfg       = handleLayoutConfig();
//     Structurizer structurizer(lcfg);
//     auto structs = structurizer.structurize(toks);
//     std::cout << std::endl;
//     // printRawTokens(structs, std::cout);
    
//     // LexerConfig lxCfg      = handleLex(lexCfg);
//     auto& v = handleLex(lexCfg);
//     Lexer lexer(v);
//     auto lexedTokens = lexer.lex(structs);


//     debugLog(true, "LEXER OUTPUT: ");
//     bool colored = true;
//     for (const auto& token : lexedTokens) {
//         String tok = colored ? highlight("Token", Colors::green) : "Token";
//         std::cout << tok + "(Type: " << tokenTypeToString(token.type, colored)
//                   << ", Value: " << token.value
//                   << ", Line: " << token.line
//                   << ", Column: " << token.column << ")" << std::endl;
//     }

    
//     auto tokenizer = Tokenizer(source);
//     tokenizer.tokenize();
//     debugLog(true, "TOKENIZER OUTPUT: ");
//     tokenizer.printTokens(colored);

//     return lexedTokens;

// }