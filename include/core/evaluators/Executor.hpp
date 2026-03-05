#include "core/TypesFWD.hpp"

namespace Executor {
    Node Function(const String& name, SharedPtr<Scope> callScope, SharedPtr<Scope> capturedScope, bool requiresReturn, const ArgumentList& args, CodeBlock* body, ParamList& parameters,
        SharedPtr<ClassInstanceNode> instanceNode);

    Node Method(const String& name, SharedPtr<Scope> callScope, SharedPtr<Scope> capturedScope, bool requiresReturn, const ArgumentList& args, CodeBlock* body, ParamList& parameters,
        SharedPtr<ClassInstanceNode> instanceNode);

}