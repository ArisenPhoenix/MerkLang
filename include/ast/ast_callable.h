#ifndef AST_CALLABLE_H
#define AST_CALLABLE_H

#include <unordered_set>
#include "utilities/debugger.h" 
#include "core/callables/param_node.h"
#include "ast/ast_base.h"
#include "ast/ast.h"
#include "ast/ast_control.h" 

class Scope;
class MethodDef;
class Callable;
class MethodBody;
class FunctionBody;
class Chain;


struct Argument {
public:
    UniquePtr<BaseAST> key;   // nullptr if positional
    UniquePtr<BaseAST> value; // always set

    bool isKeyword() const { return key != nullptr; }
    AstType getAstType() const {return AstType::Argument;}
    String getAstTypeAsString() const {return astTypeToString(getAstType());}

    String toString() const;
    void setScope(SharedPtr<Scope> newScope);
    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const;
    Argument clone() const;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const;
    FreeVars collectFreeVariables() const;
};

class Arguments: public ASTStatement {
    Vector<Argument> arguments;

public:
    AstType getAstType() const override {return AstType::Arguments;}
    Arguments(SharedPtr<Scope> scope = nullptr);

    explicit Arguments(Vector<Argument> a, SharedPtr<Scope> scope = nullptr);

    void addPositional(UniquePtr<ASTStatement> val);

    void addKeyword(UniquePtr<ASTStatement> key, UniquePtr<ASTStatement> val);

    void add(Argument);

    const Vector<Argument>& getArgs() const;

    bool hasKeywords() const;

    String toString() const override;
    void setScope(SharedPtr<Scope> newScope) override;
    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
    ArgResultType evaluateAll(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr);
    UniquePtr<BaseAST> clone() const override;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
    FreeVars collectFreeVariables() const override;

    void printAST(std::ostream& os, int indent = 0) const override;

    auto begin() { return arguments.begin(); } 
    auto begin() const { return arguments.begin(); }

    auto end() { return arguments.end(); }
    auto end() const { return arguments.end(); }

    auto cbegin() const { return arguments.cbegin(); }
    auto cend() const { return arguments.cend(); }

    
};


 
class CallableSignature {
private:
    SharedPtr<Callable> callable;
    CallableType callType;
    CallableType subType; // DEF, FUNCTION, etc.
    mutable Vector<NodeValueType> parameterTypes; 
    ParamList parameters;

public:
    explicit CallableSignature(SharedPtr<Callable> callable, CallableType callType);
    explicit CallableSignature(SharedPtr<Callable> callable);
    virtual ~CallableSignature();

    Node call(const ArgumentList& args, SharedPtr<Scope> scope) const;
    virtual Node call(const ArgumentList& args, SharedPtr<Scope> scope, SharedPtr<Scope> classScope) const;

    bool matches(const Vector<NodeValueType>& otherTypes) const;
    
    SharedPtr<Callable> getCallable() const;
 
    void setParameterTypes(Vector<NodeValueType> paramTypes) {parameterTypes = paramTypes;} 
    const Vector<NodeValueType>& getParameterTypes() const;
    const ParamList getParameters();
    CallableType getCallableType() { return callType; }
    void setCallableType(CallableType callType);
    bool getIsUserFunction();
    CallableType getSubType() {return subType;}
    void setSubType(CallableType subClassification) {subType = subClassification;}
    void setParameters(ParamList params);
 
};


class CallableBody : public CodeBlock {
public:
    // friend class MethodBody;
    CallableBody(SharedPtr<Scope> scope);
    CallableBody(UniquePtr<CodeBlock>&& block);

    CallableBody(UniquePtr<CallableBody>&& block);
    CallableBody(UniquePtr<CallableBody>* body);
    virtual ~CallableBody();


    virtual AstType getAstType() const override { return AstType::CallableBody;}
    virtual Vector<UniquePtr<BaseAST>>& getChildren(){return children;};
    virtual String toString() const override {return astTypeToString(getAstType());}

    virtual void printAST(std::ostream& os, int indent = 0) const;
    virtual Node evaluate(SharedPtr<Scope> scope, [[maybe_unused]] SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
    Node evaluate() const override {return evaluate(getScope());}

    virtual UniquePtr<BaseAST> clone() const override;

    // For Calling Within A Method's Context
    MethodBody* toMethodBody();
    
    // For Calling Within A Function's Context
    FunctionBody* toFunctionBody(); 

    // CatchAll
    CallableBody* toCallableBody();

    bool getIsStatic();

    void setNonStaticElements(Vector<Chain*> nonStaticEls);

    Vector<Chain*> getNonStaticElements();
    friend class MethodBody;
    friend class FunctionBody;

protected:
    Vector<Chain*> nonStaticElements = {};
    
};



// FunctionDef: defines a function, registers it, and may return nothing.
class CallableDef : public ASTStatement {
public:
    String name;
    mutable ParamList parameters;
    UniquePtr<CallableBody> body;
    ~CallableDef();

    CallableType callType;
    friend class MethodDef;

    CallableDef(String name, ParamList parameters, UniquePtr<CallableBody> body, CallableType funcType, SharedPtr<Scope> scope);

    virtual AstType getAstType() const override { return AstType::CallableDefinition;}
    virtual Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance) const override;
    virtual UniquePtr<BaseAST> clone() const override;
    void printAST(std::ostream& os, int indent = 0) const override;
    virtual String toString() const override {return astTypeToString(getAstType());}

    UniquePtr<CallableBody>& getBody() { return body; }
    CallableBody* getBody() const {return body.get();}
    String getName() {return name;}
    ParamList getParameters() {return parameters;}
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
    FreeVars collectFreeVariables() const override;

    void setScope(SharedPtr<Scope> newScope);
    
    
};

class CallableCall : public ASTStatement {

public:
    String name;
    UniquePtr<ArgumentType> arguments;

    CallableCall(String name, UniquePtr<ArgumentType> arguments, SharedPtr<Scope> scope);
    ~CallableCall();
    virtual AstType getAstType() const override { return AstType::CallableCall;}
    

    virtual Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
    virtual void printAST(std::ostream& os, int indent = 0) const override;
    virtual UniquePtr<BaseAST> clone() const override;

    String toString() const override {return "<" + astTypeToString(getAstType()) + "name" + getName() + ">";}
    String getName() const {return name;}
    ArgResultType handleArgs(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const;
    Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
    FreeVars collectFreeVariables() const override;
    void setScope(SharedPtr<Scope>) override;
    UniquePtr<ArgumentType> cloneArgs() const;
};


class CallableRef : public ASTStatement {

public:
    String name;

    CallableRef(String name, SharedPtr<Scope> scope);
    AstType getAstType() const override { return AstType::CallableReference;}
    String toString() const override {return astTypeToString(getAstType());}

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instance = nullptr) const override;
 
    String getName() const {return name;}
    virtual UniquePtr<BaseAST> clone() const override;
    // Vector<const BaseAST*> getAllAst(bool includeSelf = true) const override;
};
    


    
    




#endif // AST_CALLABLE_H 