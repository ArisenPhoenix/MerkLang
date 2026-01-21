#include "core/TypesFWD.hpp"

namespace Executor {
    Node Function(String name, SharedPtr<Scope> callScope, SharedPtr<Scope> capturedScope, bool requiresReturn, ArgumentList args, CodeBlock* body, ParamList& parameters,
        SharedPtr<ClassInstanceNode> instanceNode);

    Node Method(String name, SharedPtr<Scope> callScope, SharedPtr<Scope> capturedScope, bool requiresReturn, ArgumentList args, CodeBlock* body, ParamList& parameters,
        SharedPtr<ClassInstanceNode> instanceNode);

}