// #ifndef AST_STRUCTURES_H
// #define AST_STRUCTURES_H

// #include "core/types.h"
// #include "ast/ast_base.h"
// #include "ast/ast.h"

// #include "core/node.h"
// #include <unordered_map>

// #include "ast/ast_base.h"
// #include "core/node.h"
// #include <unordered_map>




// // EnumStructure: Implements a key-value structure with iteration
// class EnumStructure : public BaseAST, public MapIterator {
//     private:
//         int lastValue = 0; // Tracks the last assigned integer for auto-increment
    
//     public:
//         explicit EnumStructure(SharedPtr<Scope> scope = nullptr);
    
//         void insert(const Node& key, const Node& value = Node());
    
//         AstType getAstType() const override;
//         UniquePtr<BaseAST> clone() const override;
//     };
    




// #endif // AST_STRUCTURES_H
