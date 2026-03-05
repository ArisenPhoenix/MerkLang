#include <iostream>
#include <unordered_set>

#include "core/Scope.hpp"
#include "utilities/debugger.h"

#if MERK_SCOPE_DIAGNOSTICS
#include <sstream>
#endif

void Scope::printContext(int depth) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    String indent(depth * 2, ' ');
    auto parent = parentScope.lock();
    std::cout << indent << "Scope Level: " << scopeLevel
              << " | Memory Loc: " << this
              << " | Parent Loc: " << (parent ? parent.get() : nullptr)
              << " | Number of Variables: " << context.getVariables().size()
              << " | Number of Children: " << childScopes.size() << std::endl;

    if (parent) {
        parent->printContext(depth + 1);
    }
    DEBUG_FLOW_EXIT();
}

void Scope::printVariables(int depth) const {
    DEBUG_FLOW(FlowLevel::VERY_LOW);
    String indent(depth * 2, ' ');
    for (const auto& [name, value] : context.getVariables()) {
        debugLog(true, indent, name, " = ", value->toString());
    }

    for (const auto& child : childScopes) {
        if (child.get() != this) {
            child->printVariables(depth + 1);
        }
    }
    DEBUG_FLOW_EXIT();
}

void Scope::printScopeTree() const {
    std::unordered_set<const Scope*> visited;

    auto walk = [&](auto&& self, const Scope* node, int depth) -> void {
        if (!node) {
            return;
        }

        String indent(depth * 2, ' ');
        if (!visited.insert(node).second) {
            std::cout << indent << "[cycle] addr=" << node << " owner=" << node->owner << std::endl;
            return;
        }

        auto parent = node->parentScope.lock();
        std::cout << indent
                  << "Scope(addr=" << node
                  << ", owner='" << node->owner
                  << "', kind=" << scopeKindToString(node->kind)
                  << ", level=" << node->scopeLevel
                  << ", parent=" << (parent ? parent.get() : nullptr)
                  << ", children=" << node->childScopes.size()
                  << ", vars=" << node->context.getVariables().size()
                  << ")" << std::endl;

        for (const auto& child : node->childScopes) {
            self(self, child.get(), depth + 1);
        }
    };

    std::cout << "=== Scope Tree Start ===" << std::endl;
    walk(walk, this, 0);
    std::cout << "=== Scope Tree End ===" << std::endl;
}

#if MERK_SCOPE_DIAGNOSTICS
[[maybe_unused]] static String scopeAttachTag(const Scope* s) {
    if (!s) {
        return "<null-scope>";
    }
    std::ostringstream oss;
    oss << s;
    return "owner='" + s->owner + "' kind='" + scopeKindToString(s->kind) +
           "' level=" + std::to_string(s->scopeLevel) + " addr=" + oss.str();
}
#endif

void Scope::appendDiagnostics(ParentAppendAttempt attempt) {
#if MERK_SCOPE_DIAGNOSTICS
    switch (attempt) {
        case ParentAppendAttempt::None:
            break;
        case ParentAppendAttempt::Parent:
            parentAppendAttempts.fetch_add(1, std::memory_order_relaxed);
            break;
        case ParentAppendAttempt::Ancestor:
            ancestorAppendAttempts.fetch_add(1, std::memory_order_relaxed);
            break;
        case ParentAppendAttempt::Reparent:
            reparentAttempts.fetch_add(1, std::memory_order_relaxed);
            break;
    }
#else
    (void)attempt;
#endif
}

void Scope::diagnosticAppend(const SharedPtr<Scope>& childScope) {
#if MERK_SCOPE_DIAGNOSTICS
    if (!childScope) {
        return;
    }

    // Verify attach-cycle via graph reachability in debug only.
    if (childScope->has(shared_from_this())) {
        appendDiagnostics(ParentAppendAttempt::Ancestor);
        DEBUG_LOG(
            LogLevel::WARNING,
            "[ScopeDiag] cycle would be created by append (has-check) | parent=",
            scopeAttachTag(this),
            " | child=",
            scopeAttachTag(childScope.get())
        );
    } else {
        DEBUG_LOG(
            LogLevel::TRACE,
            "[ScopeDiag] has-check passed (no cycle) | parent=",
            scopeAttachTag(this),
            " | child=",
            scopeAttachTag(childScope.get())
        );
    }

    if (auto parent = parentScope.lock(); parent && parent.get() == childScope.get()) {
        appendDiagnostics(ParentAppendAttempt::Parent);
        DEBUG_LOG(
            LogLevel::WARNING,
            "[ScopeDiag] child is direct parent during append | current=",
            scopeAttachTag(this),
            " | attemptedChild=",
            scopeAttachTag(childScope.get())
        );
    }

    for (const Scope* p = this; p != nullptr; ) {
        if (p == childScope.get()) {
            appendDiagnostics(ParentAppendAttempt::Ancestor);
            DEBUG_LOG(
                LogLevel::WARNING,
                "[ScopeDiag] child is ancestor during append | current=",
                scopeAttachTag(this),
                " | attemptedChild=",
                scopeAttachTag(childScope.get())
            );
            break;
        }
        auto next = p->parentScope.lock();
        p = next.get();
    }

    if (auto existingParent = childScope->parentScope.lock();
        existingParent && existingParent.get() != this) {
        appendDiagnostics(ParentAppendAttempt::Reparent);
        DEBUG_LOG(
            LogLevel::WARNING,
            "[ScopeDiag] reparent append detected | oldParent=",
            scopeAttachTag(existingParent.get()),
            " | newParent=",
            scopeAttachTag(this),
            " | child=",
            scopeAttachTag(childScope.get())
        );
    }
#else
    (void)childScope;
#endif
}
