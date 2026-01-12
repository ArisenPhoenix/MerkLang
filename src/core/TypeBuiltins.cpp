#include "core/TypesFWD.hpp"
#include "core/TypeBuiltins.hpp"
#include "core/Scope.hpp" // for scope->... access

static inline void addPrim(TypeRegistry& tr, NodeValueType t) {
    tr.add(TypeRegistry::primitive(t));
}

static inline void addNamed(TypeRegistry& tr, const String& n) {
    tr.add(TypeRegistry::named(n));
}

// If your Scope exposes its type registry as `scope->globalTypes` or similar,
// adjust accordingly.
void registerBuiltinTypes(SharedPtr<Scope> scope) {
    if (!scope) return;

    // 1) Core meta
    scope->globalTypes->add(TypeRegistry::any());
    scope->globalTypes->addPrim(NodeValueType::None);
    scope->globalTypes->addPrim(NodeValueType::Null);
    scope->globalTypes->addPrim(NodeValueType::Uninitialized);

    // 2) Scalar primitives
    scope->globalTypes->addPrim(NodeValueType::Bool);
    scope->globalTypes->addPrim(NodeValueType::Char);
    scope->globalTypes->addPrim(NodeValueType::String);

    scope->globalTypes->addPrim(NodeValueType::Int);
    scope->globalTypes->addPrim(NodeValueType::Long);
    scope->globalTypes->addPrim(NodeValueType::Float);
    scope->globalTypes->addPrim(NodeValueType::Double);

    // These are "group supertypes" in your matcher; include if you want them name-visible
    scope->globalTypes->addPrim(NodeValueType::Number);
    scope->globalTypes->addPrim(NodeValueType::Text);

    // 3) Containers (as nominal names + structural generics)
    // Decide whether you want them as:
    //  - primitives (NodeValueType::List/Dict/etc)
    //  - OR "Named"/"Container" kinds
    //
    // With your TypeKind, these should be Container (base + params).
    // Seed the typical "unknown element type" versions:
    {
        auto Any = TypeRegistry::any();
        scope->registerType(TypeRegistry::container("List",   {Any}));
        scope->registerType(TypeRegistry::container("Array",  {Any}));
        scope->registerType(TypeRegistry::container("Set",    {Any}));
        scope->registerType(TypeRegistry::container("Vector", {Any}));
        scope->registerType(TypeRegistry::container("Dict",   {Any, Any}));
    }

    // 4) Nominal builtins that exist as classes in your runtime
    // These should match your native classes:

    scope->globalTypes->addNamed("List");
    scope->globalTypes->addNamed("Dict");
    scope->globalTypes->addNamed("Array");
    scope->globalTypes->addNamed("Set");
    scope->globalTypes->addNamed("Http");
    scope->globalTypes->addNamed("File");


    // 5) Callable-ish nominals (only if you want them visible as types)
    scope->globalTypes->addPrim(NodeValueType::Function);
    scope->globalTypes->addPrim(NodeValueType::Method);
    scope->globalTypes->addPrim(NodeValueType::Callable);
    scope->globalTypes->addPrim(NodeValueType::CallableSignature);
    scope->globalTypes->addPrim(NodeValueType::Function);

    // 6) Class / instances (if you want them name-visible)
    scope->globalTypes->addPrim(NodeValueType::Class);
    scope->globalTypes->addPrim(NodeValueType::ClassInstance);

    // If you have "Scope" as a runtime tag but you DON'T want it as a user-visible type, skip it.
    // addPrim(scope->globalTypes, NodeValueType::Scope);
}
