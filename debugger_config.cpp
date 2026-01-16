#include "core/types.h"
#include "utilities/debugger.h"

namespace Debug
{
    // // Option 1: Explicit configuration function

    void configureDebugger()
    {
        Debugger& debugger = Debugger::getInstance();
        debugger.setEnabled(true);

        auto globalLevel = FlowLevel::VERY_LOW;
        
        debugger.setGlobalLogLevel(LogLevel::NONE);
        debugger.setGlobalFlowLevel(FlowLevel::LOW);

        debugger.setIncludeTimestamp(false);
        debugger.setIncludeFileInfo(true);
        debugger.setIncludeFlowLevel(true);

        // MAIN
        // TOKENIZER
        debugger.setLogLevels("core/Tokenizer.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/tokenizer/tokenizer_main.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/tokenizer/tokenizer_helpers.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);

        // NODE - DATA
        debugger.setLogLevels("core/node.h", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/node.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/node_constructors.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/node_conversions.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);

        // REGISTRY
        debugger.setLogLevels("core/registry/FunctionRegistry.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/registry/function_registry.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/registry/ClassRegistry.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/registry/class_registry.cpp", LogLevel::TRACE, FlowLevel::VERY_LOW);

        // CONTEXT
        debugger.setLogLevels("core/registry/Context.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/context.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);

        // SCOPE
        debugger.setLogLevels("core/Scope.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/scope/scope_helpers.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/scope/scope_main.cpp", LogLevel::WARNING, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/scope/scope_builders.cpp", LogLevel::NONE, FlowLevel::VERY_LOW);

        // AST
        debugger.setLogLevels("ast/AstBase.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("ast/Ast.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("ast/Ast.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);


        // AST - CONTROL
        debugger.setLogLevels("ast/AstControl.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("ast/ast_control.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("ast/ast_clone.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("ast/ast_collect.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW); 
        debugger.setLogLevels("ast/ast_print.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);

        // AST - FUNCTIONS
        debugger.setLogLevels("ast/AstFunction.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("ast/AstFunction.cpp", LogLevel::DEBUG, FlowLevel::VERY_LOW);
        debugger.setLogLevels("ast/AstMethod.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);


        // AST OBJECT
        debugger.setLogLevels("ast/AstCallable.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("ast/AstCallable.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("ast/AstClass.h", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("ast/AstClass.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("ast/AstChain.h", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("ast/AstChain.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);


        // PARSER
        debugger.setLogLevels("core/Parser.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/parser/parse_control.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/parser/parse_functions.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/parser/parse_main.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/parser/parse_simple.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/parser/parse_classes.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);

        // FUNCTIONS
        debugger.setLogLevels("core/callables/functions/Function.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/callables/functions/Function.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);

        debugger.setLogLevels("core/callables/param_node.h", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/callables/param_node.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);

        debugger.setLogLevels("core/callables/Callable.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/callables/callable.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);


        // CLASSES
        debugger.setLogLevels("core/callables/classes/ClassBase.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/callables/classes/ClassBase.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/callables/classes/Method.hpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/callables/classes/Method.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);

        // EVALUATOR
        debugger.setLogLevels("core/evaluator.h", LogLevel::ERROR, FlowLevel::VERY_LOW);
        debugger.setLogLevels("core/evaluator.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);

        // HELPERS
        debugger.setLogLevels("core/helpers/class_helpers.cpp", LogLevel::ERROR, FlowLevel::VERY_LOW);
        
        // debugger.printFiles();
        // Colors::setEnabled(false);
    }
}