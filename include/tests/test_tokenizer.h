#include <iostream>
#include <fstream>
#include "core/types.h"
#include "core/Tokenizer.hpp"

// very old test, I'll keep it for nostalgia

void testTokenizer(const std::string& filePath) {
    // Read file content
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filePath << "\n";
        return;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Tokenize content
    Tokenizer tokenizer(content);
    auto tokens = tokenizer.tokenize();

    // Display tokens
    std::cout << "Tokens for file: " << filePath << "\n";
    for (const auto& token : tokens) {
        std::cout << token.toString() << "\n";
    }
}


