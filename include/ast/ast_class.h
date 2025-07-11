#ifndef AST_CLASS_H
#define AST_CLASS_H

#include "core/types.h"               // For String, Vector, UniquePtr, SharedPtr, NodeValueType, etc.
#include "utilities/debugger.h"       // For DEBUG_LOG

#include "core/node.h"
#include "ast/ast_base.h"             // For ASTStatement, printIndent, etc.
#include "ast/ast.h"
#include "ast/ast_control.h"
#include "ast/ast_function.h"
#include "core/functions/param_node.h"

#include "core/errors.h"              // For error types

class Scope;
class Method;
class Function;
class Chain;

void applyInstanceMethodBodyAccessorScopeFix(const Vector<UniquePtr<BaseAST>>& bodyChildren, SharedPtr<Scope> classScope, const String& accessor);

Vector<Chain*> applyAccessorScopeFix(MethodDef* methodDef, SharedPtr<Scope> classScope, const String& accessor);


class ClassBody : public CallableBody {
    SharedPtr<Scope> classScope = nullptr;
    String accessor;
    SharedPtr<Scope> classCapturedScope = nullptr;
    public:
        explicit ClassBody(SharedPtr<Scope> scope);
        
        // Steal constructor: take over a CodeBlock's children.
        ClassBody(UniquePtr<CodeBlock>&& block);
    
        virtual ~ClassBody() override;
    
        Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
    
        UniquePtr<BaseAST> clone() const override;    
        AstType getAstType() const override {return AstType::ClassBlock;}
        void setClassScope(SharedPtr<Scope> classScope);
        void setAccessor(String& classAccessor);
        void setCapturedScope(SharedPtr<Scope> capturedScope);

    };

class MethodBody : public CallableBody {
private:
    Vector<Chain*> nonStaticElements;
    bool isStatic = false;
public:
    MethodBody(SharedPtr<Scope> scope);
    MethodBody(UniquePtr<CodeBlock>&& body);
    MethodBody(UniquePtr<CallableBody>* body);
    virtual ~MethodBody() override;

    Vector<UniquePtr<BaseAST>>& getChildren(){return children;};
    UniquePtr<BaseAST> clone() const override;

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
    Node evaluate() const override;

    void setNonStaticElements(Vector<Chain*> nonStaticEls);
    AstType getAstType() const override { return AstType::ClassMethodBlock;}
    bool getIsStatic();
    Vector<Chain*> getNonStaticElements();
    // UniquePtr<BaseAST> clone() const override;
};




// Class Definition AST Node
class ClassDef : public CallableDef {

private:
    String accessor;
    
public:
    // Constructor: capture the class name and the defining scope.
    ClassDef(String name, ParamList parameters, UniquePtr<ClassBody> body, String accessor, SharedPtr<Scope> scope);
    ~ClassDef();

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    // Print the AST for debugging.
    void printAST(std::ostream& os, int indent = 0) const override;
    Vector<String> removeMethodAccessors(UniquePtr<ClassBody>& clsBody) const;
    AstType getAstType() const override {return AstType::ClassDefinition;}
    void setClassAccessor(String accessorName);
    String getClassAccessor();
    ParamList& getParameters();
}; 

class ClassCall : public CallableCall {
    String accessor; 

public:
    ClassCall(String className, Vector<UniquePtr<ASTStatement>> args, SharedPtr<Scope> scope);
    ~ClassCall();
    void printAST(std::ostream& os, int indent = 0) const override;
    AstType getAstType() const override {return AstType::ClassCall;}
    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

};



// Class Member Access AST Node
class ClassRef : public ASTStatement {
public:
    String instanceName;
    // Constructor: takes the instance variable name and the member name.
    ClassRef(String instanceName, SharedPtr<Scope> scope);
    UniquePtr<BaseAST> clone() const override;

    AstType getAstType() const override {return AstType::ClassReference;}
};


class MethodDef : public CallableDef {
protected:
    String accessor;
    CallableType methodType = CallableType::DEF;
    SharedPtr<Scope> classScope;
    Vector<Chain*> nonStaticElements;
public:
    // Constructor: Force the FunctionType to METHOD.
    MethodDef(String name, ParamList parameters, UniquePtr<MethodBody> body, SharedPtr<Scope> scope);
    MethodDef(String name, ParamList parameters, UniquePtr<MethodBody> body, CallableType methodType, SharedPtr<Scope> scope);
    MethodDef(UniquePtr<FunctionDef> funcDef);
    ~MethodDef();

    UniquePtr<BaseAST> clone() const override;

    String toString() const override;
    AstType getAstType() const override { return AstType::ClassMethodDef;}
    void setMethodAccessor(String& accessorName);
    String getMethodAccessor() const;
    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
    ParamList& getParameters();
    const ParamList& getParameters() const;   // For inspection

    void setClassScope(SharedPtr<Scope> scope);
    SharedPtr<Scope> getClassScope() const;
    bool isConstructor() const;
    void setNonStaticElements(Vector<Chain*> nonStaticEls);

};

class MethodCall : public CallableCall {
public:
    MethodCall(String methodName, Vector<UniquePtr<ASTStatement>> args, SharedPtr<Scope> scope);
    ~MethodCall();
    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;
    AstType getAstType() const override { return AstType::ClassMethodCall;}

};

    
class MethodRef : public CallableRef {
protected:
    String accessor;
public:
    MethodRef(String name, SharedPtr<Scope> scope);

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override;

    AstType getAstType() const override {return AstType::ClassMethodRef;}
};



class Accessor : public ASTStatement {
protected:
    String accessor;

public:
    Accessor(String accessor, SharedPtr<Scope> scope);

    AstType getAstType() const override {return AstType::Accessor;}

    Node evaluate(SharedPtr<Scope> scope, SharedPtr<ClassInstanceNode> instanceNode = nullptr) const override; 
    Node evaluate() const override {return evaluate(getScope());}
    
    String getAccessor() const {return accessor;}
    String& getQualifiedAccessor() {return accessor;}
    void setAccessor(String access) {accessor = access;}
    UniquePtr<BaseAST> clone() const override;
    void printAST(std::ostream& os, int indent = 0) const override;
    void setScope(SharedPtr<Scope> scope) override;

};

#endif // AST_CLASS_H







