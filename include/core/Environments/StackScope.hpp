#pragma once

#include "core/Environments/Scope.hpp"
#include "core/Environments/Frame.hpp"
#include <atomic>

// Lightweight callable-frame specialization.
// Keeps Scope API unchanged so call sites can adopt it incrementally.
class StackScope final : public Scope {
public:
    // Root scope (e.g. global) - uses frame for top-level variables
    StackScope(int scopeNum, bool interpretMode, bool isRoot = false)
        : Scope(scopeNum, interpretMode, isRoot) {}

    StackScope(SharedPtr<FunctionRegistry> globalF, SharedPtr<ClassRegistry> globalC, SharedPtr<TypeRegistry> globalT, SharedPtr<TypeSignatureRegistryManager> globalTypeSigs, bool interpretMode)
        : Scope(std::move(globalF), std::move(globalC), std::move(globalT), std::move(globalTypeSigs), interpretMode) {}

    StackScope(SharedPtr<Scope> parentScope,
               SharedPtr<FunctionRegistry> globalF,
               SharedPtr<ClassRegistry> globalC,
               SharedPtr<TypeRegistry> globalT,
               bool interpretMode)
        : Scope(std::move(parentScope),
                std::move(globalF),
                std::move(globalC),
                std::move(globalT),
                interpretMode) {}

    void declareVariable(const String& name, UniquePtr<VarNode> value) override;
    void clear(bool internalCall = true) override;
    void updateVariable(const String& name, const Node& value) override;
    VarNode& getVariable(const String& name) override;
    bool hasVariable(const String& name) const override;
    bool tryReadInt(const String& name, int& out) const override;
    bool tryWriteInt(const String& name, int value) override;

    SharedPtr<Scope> clone(bool strict = false) const override;
    Frame& getFrame() { return frame; }
    const Frame& getFrame() const { return frame; }

    static void resetInstrumentation();
    static void printInstrumentation(std::ostream& os = std::cout);

private:
    VarNode* findLocalVar(const String& name);
    const VarNode* findLocalVar(const String& name) const;
    static void noteSlotKind(NodeValueType valueType);

    mutable Frame frame;
    static inline std::atomic<uint64_t> cacheHits{0};
    static inline std::atomic<uint64_t> cacheMisses{0};
    static inline std::atomic<uint64_t> localContextHits{0};
    static inline std::atomic<uint64_t> localContextMisses{0};
    static inline std::atomic<uint64_t> parentFallbackLookups{0};
    static inline std::atomic<uint64_t> parentFallbackUpdates{0};
    static inline std::atomic<uint64_t> slotKindUnknown{0};
    static inline std::atomic<uint64_t> slotKindInt{0};
    static inline std::atomic<uint64_t> slotKindDouble{0};
    static inline std::atomic<uint64_t> slotKindBool{0};
    static inline std::atomic<uint64_t> slotKindObject{0};
};
