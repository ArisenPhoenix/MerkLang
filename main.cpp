#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cassert>

#include "utilities/streaming.h"

#include "core/types.h"
#include "core/node.h"
#include "core/tokenizer.h"
#include "core/parser.h"
#include "core/context.h"

#include "ast/ast_base.h"
#include "ast/ast_control.h"
#include "ast/ast.h"

#include "utilities/helper_functions.h"
#include "utilities/debugging_functions.h"
#include "utilities/debugger.h"

#include "core/scope.h"


int main(int argc, char* argv[]) {
    // Debug::configureDebugger();
    Debug::configureDebugger();

    const String codeDir = "code/";
    const String defaultFile = "test1.merk";

    // Step 1: Determine the file path
    String filePath = getFilePath(argc, argv, codeDir, defaultFile);
    DEBUG_LOG(LogLevel::DEBUG, "Using File: ", filePath);
   
    // Step 2: Read file content
    String content = readFile(filePath);
    outputFileContents(content, 500);

    // Step 3: Initialize Global Scope
    const bool interpretMode = true;
    const bool byBlock = true;
    const bool isRoot = true;

        
    SharedPtr<Scope> globalScope = std::make_shared<Scope>(0, interpretMode, isRoot);
    globalScope->owner = "GLOBAL";
    
    try {
        // Step 4: Initialize Tokenizer

        DEBUG_LOG(LogLevel::DEBUG, "Initializing tokenizer...");
        Tokenizer tokenizer(content);
        DEBUG_LOG(LogLevel::DEBUG, "Starting tokenization...");
        auto tokens = tokenizer.tokenize();
        DEBUG_LOG(LogLevel::DEBUG, "Tokenization complete.\n");

        tokenizer.printTokens((Debugger::getInstance().getLevel() >= LogLevel::ERROR));

        // Step 5: Parse tokens into an AST
        DEBUG_LOG(LogLevel::DEBUG, "\nInitializing parser...");
        Parser parser(tokens, globalScope, interpretMode, byBlock);
        DEBUG_LOG(LogLevel::DEBUG, "\nStarting parser...");
        auto ast = parser.parse();
        debugLog("Parsing complete. \n");

        DEBUG_LOG(LogLevel::DEBUG, "Terminating Program...");
        DEBUG_LOG(LogLevel::DEBUG, "==================== PRINTING GLOBAL SCOPE ====================");
        
        if (!interpretMode) {
            ast->evaluate(globalScope); // Explicitly evaluate the AST in deferred mode
        }

        debugLog(true, highlight("============================== FINAL OUTPUT ==============================", Colors::green));
        ast->printAST(std::cout);
        globalScope->debugPrint();
        globalScope->printChildScopes();

        DEBUG_LOG(LogLevel::DEBUG, "");
        DEBUG_LOG(LogLevel::DEBUG, "==================== TRY TERMINATION ====================");

    } catch (MerkError& e){
        std::cerr << e.errorString() << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Error during execution: " << ex.what() << std::endl;
        return 1;
    } 

    // Print the final state of the Global Scope right before exiting
    DEBUG_LOG(LogLevel::DEBUG, "");
    // DEBUG_LOG(LogLevel::DEBUG, "==================== FINAL GLOBAL SCOPE ====================");
    // globalScope->printChildScopes();

    // DEBUG_LOG(LogLevel::DEBUG, "==================== PROGRAM TERMINATION ====================");
    // nativeFunctions.clear();
    globalScope->clear();
    globalScope.reset();
    
    return 0;
}










// #include <chrono>
// #include <iostream>
// #include <fstream>
// #include "core/tokenizer.h"


// String generatedCode;

// String generateTestTokenSource(int argc, char* argv[], size_t repeatCount) {
//     const String codeDir = "code/";
//     const String defaultFile = "test1.merk";

//     // Step 1: Determine the file path
//     String filePath = getFilePath(argc, argv, codeDir, defaultFile);
   
//     // Step 2: Read file content
//     String sourceCode = readFile(filePath);
//     String result;

//     for (size_t i = 0; i < repeatCount; ++i) {
//         result += sourceCode;
//     }

//     generatedCode = sourceCode;

//     return result;
// }

// String generateSafeMerkBlocks(size_t repeatCount) {
//     String block;
//     for (size_t i = 0; i < repeatCount; ++i) {
//         String idx = std::to_string(i);
//         block += "def func" + idx + "():\n";
//         block += "    var x" + idx + " = 0\n";
//         block += "    var y" + idx + " = 3\n";
//         block += "    var z" + idx + " = x" + idx + " + y" + idx + "\n";
//         block += "    if z" + idx + " == 13:\n";
//         block += "        z" + idx + " = 10\n";
//         block += "    else:\n";
//         block += "        z" + idx + " = 4\n";
//         block += "    var count" + idx + " = 0\n";
//         block += "    while x" + idx + " < 10:\n";
//         block += "        x" + idx + " = x" + idx + " + 1\n";
//         block += "        if x" + idx + " == 5:\n";
//         block += "            count" + idx + " = count" + idx + " + 1\n";
//         block += "        elif x" + idx + " == 8:\n";
//         block += "            break\n";
//         block += "        else:\n";
//         block += "            x" + idx + " = x" + idx + " + 1\n";
//         block += "\n";
//         block += "func" + idx + "()\n";  // <== include the call
//     }
//     return block;
// }


// String generateTestTokenSource(size_t repeatCount) {
//     return generateSafeMerkBlocks(repeatCount);
// }



// Vector<Token> profileTokenizer(int argc, char* argv[]){
//     for (size_t repeat : {10, 100, 1000, 5000}) {
//         auto code = generateTestTokenSource(argc, argv, repeat);
//         Tokenizer tokenizer(code);
//         auto start = std::chrono::high_resolution_clock::now();
//         auto tokens = tokenizer.tokenize();
//         auto end = std::chrono::high_resolution_clock::now();
//         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//         std::cout << repeat << " blocks: " << duration << "ms for " << tokens.size() << " tokens\n";
//     }
//     auto parserCode = generateTestTokenSource(argc, argv, 0);
//     outputFileContents(generatedCode, 500);

//     Tokenizer tokenizer(parserCode);
//     auto tokens = tokenizer.tokenize();
//     return tokens;
// }

// Vector<Token> profileTokenizer(size_t repeatCount) {
//     auto code = generateSafeMerkBlocks(repeatCount);
//     Tokenizer tokenizer(code);

//     auto start = std::chrono::high_resolution_clock::now();
//     auto tokens = tokenizer.tokenize();
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

//     std::cout << repeatCount << " blocks: " << duration << "ms for " << tokens.size() << " tokens\n";
//     return tokens;
// }

// Vector<Token> profileTokenizer() {
//     Vector<Token> allTokens;

//     for (size_t repeat : {10, 100, 1000, 5000}) {
//         String fileContent = generateTestTokenSource(repeat); // <-- Generate new content per block size

//         Tokenizer tokenizer(fileContent);
//         auto start = std::chrono::high_resolution_clock::now();
//         auto tokens = tokenizer.tokenize();
//         auto end = std::chrono::high_resolution_clock::now();

//         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//         std::cout << repeat << " blocks: " << duration << "ms for " << tokens.size() << " tokens\n";

//         allTokens.insert(allTokens.end(), tokens.begin(), tokens.end());
//     }

//     return allTokens;
// }




// void profileParser(Vector<Token>& tokens, bool byBlock) {
//     const bool interpretMode = true;
//     SharedPtr<Scope> globalScope = std::make_shared<Scope>(0, interpretMode);

//     std::cout << "byBlock: " << (byBlock ? "true" : "false") << "\n";

//     auto start = std::chrono::high_resolution_clock::now();
//     Parser parser(tokens, globalScope, interpretMode, byBlock);
//     parser.parse();
//     auto end = std::chrono::high_resolution_clock::now();

//     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
//     std::cout << "Parse time: " << duration << " microseconds\n\n";
// }

// void profileEvaluation(size_t repeatCount, bool byBlock = true) {
//     String code = generateSafeMerkBlocks(repeatCount);

//     Tokenizer tokenizer(code);
//     auto tokens = tokenizer.tokenize();

//     SharedPtr<Scope> globalScope = std::make_shared<Scope>(0, true);

//     Parser parser(tokens, globalScope, true, byBlock);
//     auto ast = parser.parse();  // This should return a CallableBody or CodeBlockNode

//     auto start = std::chrono::high_resolution_clock::now();
//     ast->evaluate(globalScope);  // Evaluate all parsed AST nodes
//     auto end = std::chrono::high_resolution_clock::now();

//     auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

//     std::cout << "=== Evaluation Test ===\n";
//     std::cout << "Blocks: " << repeatCount << "\n";
//     std::cout << "Evaluation Time: " << microseconds << " μs\n\n";
// }


// void profileBlockParsing() {
//     std::vector<size_t> blockCounts = {10, 100, 1000, 5000};

//     for (bool byBlock : {true, false}) {
//         std::cout << "===== Testing Parser (byBlock = " << (byBlock ? "true" : "false") << ") =====\n";

//         for (size_t blocks : blockCounts) {
//             Vector<Token> tokens = profileTokenizer(blocks);
//             profileParser(tokens, byBlock);
//         }

//         std::cout << "\n";
//     }
// }


// void profileASTEval() {
//     for (size_t count : {10, 100, 1000, 5000}) {
//         profileEvaluation(count);
//     }

// }



// int evaluateAndCallFunctions(const SharedPtr<Scope>& globalScope, size_t repeatCount) {
//     auto start = std::chrono::high_resolution_clock::now();

//     for (size_t i = 0; i < repeatCount; ++i) {
//         String funcName = "func" + std::to_string(i);

//         auto funcNode = globalScope->getFunction(funcName);
//         if (funcNode) {
//             auto func = funcNode->get();
//             func.call({}, globalScope);
//             // funcNode->call({}, globalScope);  // No args, reuses global scope
//         } else {
//             std::cerr << "Function not found: " << funcName << std::endl;
//         }
//     }

//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
//     std::cout << "Function execution time: " << duration << " μs" << std::endl;

//     return 0;
// }


// #include <chrono>
// #include <iostream>
// // #include "core/tokenizer.h"
// // #include "core/parser/parser_main.h"
// // #include "core/scope/scope_main.h"
// // #include "core/evaluator.h"

// using Clock = std::chrono::high_resolution_clock;


// void benchmark(size_t blockCount) {
//     std::cout << "\n=== Benchmark: " << blockCount << " blocks ===\n";

//     String code = generateSafeMerkBlocks(blockCount);

//     // --- Tokenizer ---
//     auto t0 = Clock::now();
//     Tokenizer tokenizer(code);
//     Vector<Token> tokens = tokenizer.tokenize();
//     auto t1 = Clock::now();
//     auto tokenizeTime = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

//     // --- Parser ---
//     const bool interpretMode = false;
//     const bool byBlock = false;
//     SharedPtr<Scope> globalScope = std::make_shared<Scope>(0, interpretMode);

//     t0 = Clock::now();
//     Parser parser(tokens, globalScope, interpretMode, byBlock);
//     auto ast = parser.parse();
//     // parser.parse();
//     auto t2 = Clock::now();
//     auto parseTime = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t0).count();

//     // --- Evaluation ---
//     t0 = Clock::now();
//     ast->evaluate();  // Evaluate full AST
//     auto t3 = Clock::now();
//     auto evalTime = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t0).count();

//     // --- Print Results ---
//     std::cout << "Tokens: " << tokens.size() << "\n";
//     std::cout << "Tokenizer: " << tokenizeTime << " μs\n";
//     std::cout << "Parser:    " << parseTime << " μs\n";
//     std::cout << "Evaluator: " << evalTime << " μs\n";
// }

// int main() {
//     for (size_t blockCount : {10, 100, 1000, 5000}) {
//         benchmark(blockCount);
//     }
//     return 0;
// }




// int main() {
//     profileASTEval();

//     return 0;
// }








































// int profileEvaluation(const String& sourceCode, size_t repeatCount) {
//     Tokenizer tokenizer(sourceCode);
//     auto tokens = tokenizer.tokenize();

//     const bool interpretMode = true;
//     const bool byBlock = true;
//     SharedPtr<Scope> globalScope = std::make_shared<Scope>(0, interpretMode);

//     // Parsing and registering all functions
//     Parser parser(tokens, globalScope, interpretMode, byBlock);
//     parser.parse();

//     // Evaluate full AST
//     auto evalStart = std::chrono::high_resolution_clock::now();
//     parser.evaluate(); // If this is your main entrypoint to run AST
//     auto evalEnd = std::chrono::high_resolution_clock::now();
//     std::cout << "Initial Evaluation Time: "
//               << std::chrono::duration_cast<std::chrono::microseconds>(evalEnd - evalStart).count()
//               << " μs\n";

//     // Call all the defined functions
//     evaluateAndCallFunctions(globalScope, repeatCount);

//     return 0;
// }





// int run_test(int argc, char* argv[]) {
    

//     const String codeDir = "code/";
//     const String defaultFile = "time_test.merk";

//     // Step 1: Determine the file path
//     String filePath = getFilePath(argc, argv, codeDir, defaultFile);
//     DEBUG_LOG(LogLevel::DEBUG, "Using File: ", filePath);
   
//     // Step 2: Read file content
//     String content = readFile(filePath, 500);
//     // Step 3: Initialize Global Scope
//     const bool interpretMode = true;

        
//     SharedPtr<Scope> globalScope = std::make_shared<Scope>(0, interpretMode);
    
    
//     try {
//         const bool byBlock = true;
//         // Step 4: Initialize Tokenizer

//         DEBUG_LOG(LogLevel::DEBUG, "Initializing tokenizer...");
//         Tokenizer tokenizer(content);
//         DEBUG_LOG(LogLevel::DEBUG, "Starting tokenization...");
//         auto tokens = tokenizer.tokenize();
//         tokenizer.printTokens();
//         DEBUG_LOG(LogLevel::DEBUG, "Tokenization complete.\n");

//         // tokenizer.printTokens((Debugger::getInstance().getLevel() >= LogLevel::ERROR));

//         // Step 5: Parse tokens into an AST
//         DEBUG_LOG(LogLevel::DEBUG, "\nInitializing parser...");
//         Parser parser(tokens, globalScope, interpretMode, byBlock);
//         DEBUG_LOG(LogLevel::DEBUG, "\nStarting parser...");
//         auto ast = parser.parse();
//         debugLog("Parsing complete. \n");

//         DEBUG_LOG(LogLevel::DEBUG, "Terminating Program...");
//         DEBUG_LOG(LogLevel::DEBUG, "==================== PRINTING GLOBAL SCOPE ====================");
        
//         if (!interpretMode) {
//             ast->evaluate(globalScope); // Explicitly evaluate the AST in deferred mode
//         }

//         debugLog(true, highlight("============================== FINAL OUTPUT ==============================", Colors::green));
//         ast->printAST(std::cout);
//         // globalScope->debugPrint();
//         // globalScope->printChildScopes();

//         DEBUG_LOG(LogLevel::DEBUG, "");
//         DEBUG_LOG(LogLevel::DEBUG, "==================== TRY TERMINATION ====================");

//     } catch (MerkError& e){
//         std::cerr << e.errorString() << std::endl;
//     } catch (const std::exception& ex) {
//         std::cerr << "Error during execution: " << ex.what() << std::endl;
//         return 1;
//     } 

//     // Print the final state of the Global Scope right before exiting
//     // DEBUG_LOG(LogLevel::DEBUG, "");
//     // globalScope->debugPrint();
//     globalScope->getContext().debugPrint();
//     globalScope.reset();
    
//     return 0;
// }


// int main(int argc, char* argv[]) {
//     #include <chrono>
//     using namespace std::chrono;
//     Debug::configureDebugger();
//     auto start = high_resolution_clock::now();

//     const String codeDir = "code/";
//     const String defaultFile = "time_test.merk";

//     run_test(argc, argv);

//     auto end = high_resolution_clock::now();
//     auto duration = duration_cast<microseconds>(end - start);
//     std::cout << "Execution time: " << duration.count() << " microseconds\n";
// }



// #include "tests/test_params.h"
// int main() {
//     runAllParamNodeTests();
//     return 0;
// }


// #include "tests/test_node.h"
// int main() {
//     testNode();
//     testLitNode();
//     testVarNode();
//     std::cout << "All tests completed successfully.\n";
//     return 0;
// }






































// int main() {
//     // Create root scope
//     std::shared_ptr<Scope> rootScope = std::make_shared<Scope>(0, true);
//     debugLog(true, "Created root scope at address: ", rootScope.get());

//     // Create BlockNode with root scope
//     auto block = std::make_unique<BlockNode>(rootScope);
//     debugLog(true, "Created BlockNode at address: ", block.get(), 
//              ", with scope: ", rootScope.get());

//     // Create child BlockNode with a body
//     auto childBody = std::make_unique<BlockNode>(rootScope);
//     auto childNode = std::make_unique<BlockNode>(rootScope, std::move(childBody));
//     debugLog(true, "Created child BlockNode with body at address: ", childNode.get());

//     // Add child to parent BlockNode
//     block->addChild(std::move(childNode));
//     debugLog(true, "Added child BlockNode. Children count: ", block->getChildren().size());

//     debugLog(true, "Test complete. Scope use_count: ", rootScope.use_count());

//     return 0;
// }



// int main(){
//     auto globalScope = std::make_shared<Scope>(0, true); // Root scope
//     auto childScope1 = globalScope->createChildScope(); // Level 1
//     auto childScope2 = childScope1->createChildScope(); // Level 2

//     // Print debug information
//     globalScope->printChildScopes();

// }


// int main(){
//     try {
//         Node node(42, false, true, false, "Number"); // Static int Node
//         node.setValue(42.0);
//         debugLog(true, node);
//     } catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << "\n";
//     }
// }