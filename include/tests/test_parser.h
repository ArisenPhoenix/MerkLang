// #include "parser.h"
// #include <cassert>

// void test_parser() {
//     std::string code = "let x = 10;";
//     auto tokens = tokenize(code);
//     auto ast = parse(tokens);

//     assert(dynamic_cast<DeclarationNode*>(ast.get())->name == "x");
//     assert(dynamic_cast<DeclarationNode*>(ast.get())->value == "10");
// }

// int main() {
//     test_parser();
//     return 0;
// }
