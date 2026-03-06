#pragma once

#include "core/TypesFWD.hpp"
#include "core/registry/Context.hpp"

enum class EnvironmentKind {Virtual, Scope, Stack, Frame};
enum class InterpretMode {Interpret, Jit, Compile};
// Runtime environment contract for lexical scopes, stack frames, and future
// specialized environments. Keep this API narrow and execution-oriented.
class Environment {
public:
    virtual ~Environment() = default;

    virtual SharedPtr<Scope> getParent() const = 0;
    virtual int getScopeLevel() const = 0;
    virtual SharedPtr<Scope> createChildScope() = 0;
    virtual SharedPtr<Scope> makeCallScope() = 0;

    virtual const Context& getContext() const = 0;
    virtual Context& getContext() = 0;

    virtual void updateVariable(const String& name, const Node& value) = 0;
    virtual void declareVariable(const String& name, UniquePtr<VarNode> value) = 0;
    virtual bool hasVariable(const String& name) const = 0;
    virtual VarNode& getVariable(const String& name) = 0;
    virtual bool tryReadInt(const String& name, int& out) const = 0;
    virtual bool tryWriteInt(const String& name, int value) = 0;
};

