#include "core/types.h"
#include "utilities/debugger.h"

namespace Debug
{
    // // Option 1: Explicit configuration function

    void configureDebugger()
    {
        Debugger &debugger = Debugger::getInstance();
        debugger.setEnabled(true);
        
        debugger.setGlobalLogLevel(LogLevel::NONE);
        debugger.setGlobalFlowLevel(FlowLevel::LOW);

        debugger.setIncludeTimestamp(false);
        debugger.setIncludeFileInfo(true);
        debugger.setIncludeFlowLevel(true);

        // // // MAIN
        // debugger.setLogLevels("main.cpp", LogLevel::ERROR, FlowLevel::LOW);

        // TOKENIZER
        debugger.setLogLevels("core/tokenizer.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/tokenizer/tokenizer_main.cpp", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/tokenizer/tokenizer_helpers.cpp", LogLevel::ERROR, FlowLevel::NONE);

        // NODE - DATA
        debugger.setLogLevels("core/node.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/node.cpp", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/node_constructors.cpp", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/node_conversions.cpp", LogLevel::ERROR, FlowLevel::NONE);

        // REGISTRY
        debugger.setLogLevels("core/registry/function_registry.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/registry/function_registry.cpp", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/registry/class_registry.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/registry/class_registry.cpp", LogLevel::TRACE, FlowLevel::NONE);

        // CONTEXT
        debugger.setLogLevels("core/context.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/context.cpp", LogLevel::ERROR, FlowLevel::NONE);

        // SCOPE
        debugger.setLogLevels("core/scope.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/scope/scope_helpers.cpp", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/scope/scope_main.cpp", LogLevel::WARNING, FlowLevel::NONE);
        debugger.setLogLevels("core/scope/scope_builders.cpp", LogLevel::NONE, FlowLevel::NONE);

        // AST
        debugger.setLogLevels("ast/ast_base.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("ast/ast.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("ast/ast.cpp", LogLevel::ERROR, FlowLevel::NONE);

        // AST - CONTROL
        debugger.setLogLevels("ast/ast_control.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("ast/ast_control.cpp", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("ast/ast_clone.cpp", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("ast/ast_collect.cpp", LogLevel::ERROR, FlowLevel::NONE); 
        debugger.setLogLevels("ast/ast_print.cpp", LogLevel::ERROR, FlowLevel::NONE);

        // AST - FUNCTIONS
        debugger.setLogLevels("ast/ast_function.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("ast/ast_function.cpp", LogLevel::DEBUG, FlowLevel::NONE);
        debugger.setLogLevels("ast/ast_method.cpp", LogLevel::ERROR, FlowLevel::NONE);


        // AST OBJECT
        debugger.setLogLevels("ast/ast_callable.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("ast/ast_callable.cpp", LogLevel::ERROR, FlowLevel::MED);
        debugger.setLogLevels("ast/ast_class.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("ast/ast_class.cpp", LogLevel::ERROR, FlowLevel::MED);
        debugger.setLogLevels("ast/ast_chain.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("ast/ast_chain.cpp", LogLevel::ERROR, FlowLevel::MED);


        // PARSER
        debugger.setLogLevels("core/parser.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/parser/parse_control.cpp", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/parser/parse_functions.cpp", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/parser/parse_main.cpp", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/parser/parse_simple.cpp", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/parser/parse_classes.cpp", LogLevel::ERROR, FlowLevel::NONE);

        // FUNCTIONS
        debugger.setLogLevels("core/callables/functions/function.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/callables/functions/function.cpp", LogLevel::ERROR, FlowLevel::NONE);

        debugger.setLogLevels("core/callables/param_node.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/callables/param_node.cpp", LogLevel::ERROR, FlowLevel::NONE);

        debugger.setLogLevels("core/callables/callable.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/callables/callable.cpp", LogLevel::ERROR, FlowLevel::NONE);


        // CLASSES
        debugger.setLogLevels("core/callables/classes/class_base.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/callables/classes/class_base.cpp", LogLevel::ERROR, FlowLevel::MED);
        debugger.setLogLevels("core/callables/classes/method.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/callables/classes/method.cpp", LogLevel::ERROR, FlowLevel::NONE);

        // EVALUATOR
        debugger.setLogLevels("core/evaluator.h", LogLevel::ERROR, FlowLevel::NONE);
        debugger.setLogLevels("core/evaluator.cpp", LogLevel::ERROR, FlowLevel::NONE);

        // HELPERS
        debugger.setLogLevels("core/helpers/class_helpers.cpp", LogLevel::ERROR, FlowLevel::NONE);
        
        // debugger.printFiles();
        // Colors::setEnabled(false);
    }
}