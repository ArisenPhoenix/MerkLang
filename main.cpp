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
    String content = readFile(filePath, 500);
    // Step 3: Initialize Global Scope
    const bool interpretMode = true;

        
    SharedPtr<Scope> globalScope = std::make_shared<Scope>(0, interpretMode);
    
    
    try {
        const bool byBlock = false;
        // Step 4: Initialize Tokenizer

        DEBUG_LOG(LogLevel::DEBUG, "Initializing tokenizer...");
        Tokenizer tokenizer(content);
        DEBUG_LOG(LogLevel::DEBUG, "Starting tokenization...");
        auto tokens = tokenizer.tokenize();
        DEBUG_LOG(LogLevel::DEBUG, "Tokenization complete.\n");

        // if (){
        tokenizer.printTokens((Debugger::getInstance().getLevel() > LogLevel::ERROR));
        // }

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
    globalScope.reset();
    
    return 0;
}



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