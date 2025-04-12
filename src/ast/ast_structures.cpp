// #include "core/types.h"
// #include "core/errors.h"
// #include "ast/ast_base.h"
// #include "ast/ast.h"
// #include "ast/ast_structures.h"
// #include "utilities/helper_functions.h"
// #include "utilities/debugger.h"
// #include "utilities/debugging_functions.h"
// #include "helpers/iterable.h"


// // EnumStructure Constructor
// EnumStructure::EnumStructure(SharedPtr<Scope> scope)
//     : BaseAST(scope), MapIterator() {branch = "EnumStructure";}

// // Insert method with auto-increment
// void EnumStructure::insert(const Node& key, const Node& value) {
//     Node assignedValue = value;

//     if (value.getType() == NodeValueType::Null) {
//         assignedValue = Node(lastValue++); // Auto-increment last assigned value
//     }

//     if (data.find(key) != data.end()) {
//         throw std::runtime_error("Duplicate key in EnumStructure: " + key.toString());
//     }

//     MapIterator::insert(key, assignedValue);
// }

// // AST Type Identification
// AstType EnumStructure::getAstType() const {
//     return AstType::Enum;
// }

// // Cloning Method
// UniquePtr<BaseAST> EnumStructure::clone() const {
//     auto clonedEnum = std::make_unique<EnumStructure>(getScope());
//     clonedEnum->data = this->data; // Copy the map data
//     clonedEnum->lastValue = this->lastValue;
//     return clonedEnum;
// }
