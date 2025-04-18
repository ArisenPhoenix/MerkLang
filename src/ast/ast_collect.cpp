#include "utilities/debugger.h"
#include "core/types.h"
#include "utilities/debugging_functions.h"
#include "ast/ast_base.h"
#include "ast/ast.h"
#include "ast/ast_control.h"
#include "ast/ast_function.h"
#include "ast/ast_callable.h"
// #include "ast/ast_function.h"

// Vector<UniquePtr<BaseAST>> CodeBlock::getChildrenOfType(AstType astType, bool recurse) {
//     Vector<UniquePtr<BaseAST>> matches;
//     for (auto& child : children){
//         if (child->getAstType() == astType){
//             matches.emplace_back(std::move(child->clone()));
//         }

//         if (recurse){
//             if (child->getAstType() == AstType::CodeBlock){
//                 auto* block = static_cast<CodeBlock*>(child.get());
//                 for (auto& subChild : block->getChildrenOfType(astType)) {
//                     matches.emplace_back(std::move(subChild));
//                 }
//             }

//             else if (child->getAstType() == AstType::FunctionDefinition) {
//                 auto* block = static_cast<FunctionDef*>(child.get());
//                 for (auto& subChild : block->getBody()->getChildrenOfType(astType)) {
//                     matches.emplace_back(std::move(subChild));
//                 }

//             }
//         }
//     }
//     DEBUG_LOG(LogLevel::ERROR, highlight("NUMBER OF IFStatments Found", Colors::pink), matches.size());
//     return matches;
// }

FreeVars CodeBlock::collectFreeVariables() const {
    DEBUG_FLOW();
    FreeVars localDecls;
    
    // Helper lambda to process a single child node.
    auto processNode = [&](const BaseAST* node) {
        AstType type = node->getAstType();
        DEBUG_LOG(LogLevel::DEBUG, "[Free Variables] Current Node Being Processed", highlight(node->getAstTypeAsString(), Colors::blue));

        if (type == AstType::VariableDeclaration) {
            DEBUG_LOG(LogLevel::DEBUG, highlight("[Free Variables] Checking", Colors::yellow), highlight(node->getAstTypeAsString(), Colors::blue));
            // Cast to VariableDeclaration to retrieve the name.
            const auto* decl = static_cast<const VariableDeclaration*>(node);
            if (decl) {
                DEBUG_LOG(LogLevel::DEBUG, highlight("Found Var " + decl->getName(), Colors::orange));
        
                localDecls.insert(decl->getName());  // Track declared variables
            } else {
                DEBUG_LOG(LogLevel::DEBUG, "VariableDeclaration cast failed.");
            }
        }

        else if (type == AstType::VariableReference) {
            DEBUG_LOG(LogLevel::DEBUG, highlight("[Free Variables] Checking", Colors::yellow), highlight("VariableReference", Colors::blue));

            const auto* ref = static_cast<const VariableReference*>(node);
            // If this variable wasn’t declared locally, it is free.
            if (localDecls.find(ref->getName()) == localDecls.end()) {
                DEBUG_LOG(LogLevel::DEBUG, highlight("Getting Vars From VariableReference", Colors::orange));
                freeVars.insert(ref->getName());
            }
        }

        else if (type == AstType::VariableAssignment) {
            DEBUG_LOG(LogLevel::DEBUG, highlight("[Free Variables] Checking", Colors::yellow), highlight("VariableAssignment", Colors::blue));

            const auto* ref = static_cast<const VariableAssignment*>(node);
            localAssign.insert(ref->getName());
            if (localDecls.find(ref->getName()) == localDecls.end()) {
                DEBUG_LOG(LogLevel::DEBUG, highlight("Getting Variable Assignment " + ref->getName(), Colors::orange));
        
                freeVars.insert(ref->getName());
            }
        }
    }; 


    // // Process each child in the current CodeBlock.
    for (const auto& child : children) {

        AstType type = child->getAstType();
        DEBUG_LOG(LogLevel::DEBUG, highlight("[Free Variables] Current Child", Colors::yellow), highlight(child->getAstTypeAsString(), Colors::blue));

        if (type == AstType::CodeBlock || type == AstType::FunctionBlock) {
            DEBUG_LOG(LogLevel::DEBUG, highlight("Getting Vars From CodeBlock|FunctionBlock", Colors::orange));

            // For a nested block, recurse.
            const auto* block = static_cast<const CodeBlock*>(child.get());

            // Any free variable in the nested block that isn’t declared in the localDecls is free here.
            for (const auto& varName : block->collectFreeVariables()) {
                if (localDecls.find(varName) == localDecls.end()) {
                    freeVars.insert(varName);
                }
            }
        } 
        
        else if (type == AstType::IfStatement){

            DEBUG_LOG(LogLevel::DEBUG, highlight("[Free Variables] Checking", Colors::red), highlight("IfStatement", Colors::blue));
            if (!child->getScope()){
                throw ScopeError("Scope Not Found When Collecting Free Variables From IfStatment");
            }
            
            const auto* block = static_cast<const IfStatement*>(child.get());
            DEBUG_LOG(LogLevel::DEBUG, highlight("[Free Variables] IfStatement casted", Colors::red), highlight("IfStatement", Colors::blue));

            DEBUG_LOG(LogLevel::DEBUG, highlight("[Free Variables] IfStatement collected free variables", Colors::red), highlight("IfStatement", Colors::blue));

            for (const auto& freeVar : block->collectFreeVariables()){
                freeVars.insert(freeVar);
            }
            DEBUG_LOG(LogLevel::DEBUG, highlight("[Free Variables] If Statement freeVars Added", Colors::red), highlight("IfStatement", Colors::blue));

            DEBUG_LOG(LogLevel::DEBUG, highlight("[Free Variables] Checking", Colors::yellow), highlight("IfStatement", Colors::blue));

        }

        else if (type == AstType::WhileLoop){
            DEBUG_LOG(LogLevel::DEBUG, highlight("[Free Variables] Checking", Colors::yellow), highlight("WhileLoop", Colors::blue));

            const auto* block = static_cast<const WhileLoop*>(child.get());
          
            for (const auto& freeVar : block->collectFreeVariables()){
                freeVars.insert(freeVar);
            }
        }

        else {
            DEBUG_LOG(LogLevel::DEBUG, "AstType: ", highlight(child->getAstTypeAsString(), Colors::red), "AstType Int: ", static_cast<int>(child->getAstType()));
            processNode(child.get());
        }
    }

    DEBUG_FLOW_EXIT();
    return freeVars;
}

FreeVars ElseStatement::collectFreeVariables() const {
    DEBUG_FLOW();

    for (const auto& nestedFree : getBody()->collectFreeVariables()){
        freeVars.insert(nestedFree);
    }

    DEBUG_FLOW_EXIT();
    return freeVars;

}

FreeVars ElifStatement::collectFreeVariables() const {
    DEBUG_FLOW();

    for (const auto& nestedFree : getBody()->collectFreeVariables()){
        freeVars.insert(nestedFree);
    }

    DEBUG_FLOW_EXIT();
    return freeVars;

}

FreeVars IfStatement::collectFreeVariables() const {
    DEBUG_FLOW();
    freeVars.clear();
    localAssign.clear();
  
        
    for (const auto& nestedFree : getBody()->collectFreeVariables()){
        freeVars.insert(nestedFree);
    }


    for (auto& elif : getElifs()) {
        for (const auto& nestedFree : elif->getBody()->collectFreeVariables()){
            freeVars.insert(nestedFree);
        }
    }

    if (getElse()){
        for (const auto& nestedFree : getElse()->getBody()->collectFreeVariables()){
            freeVars.insert(nestedFree);
        }
    }

    DEBUG_FLOW_EXIT();
    return freeVars;
}

FreeVars WhileLoop::collectFreeVariables() const {
    DEBUG_FLOW();
    for (const auto& nestedFree : getBody()->collectFreeVariables()){
        freeVars.insert(nestedFree);
    }
    DEBUG_FLOW_EXIT();

    return freeVars;
        
}



