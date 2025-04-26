#ifndef AST_CLASS_H
#define AST_CLASS_H

#include "core/types.h"               // For String, Vector, UniquePtr, SharedPtr, NodeValueType, etc.
#include "utilities/debugger.h"       // For DEBUG_LOG

#include "ast/ast_base.h"             // For ASTStatement, printIndent, etc.
#include "ast/ast.h"
#include "ast/ast_control.h"
#include "ast/ast_function.h"
#include "core/functions/param_node.h"

#include "core/errors.h"              // For error types

class Scope;
class Method;


class ClassBody : public CallableBody {
    SharedPtr<Scope> classScope = nullptr;
    String accessor;
    SharedPtr<Scope> classCapturedScope = nullptr;
    public:
        explicit ClassBody(SharedPtr<Scope> scope);
        
        // Steal constructor: take over a CodeBlock's children.
        ClassBody(UniquePtr<CodeBlock>&& block);
    
        ~ClassBody();
    
        Node evaluate(SharedPtr<Scope> scope) const override;
    
        UniquePtr<BaseAST> clone() const override;    
        AstType getAstType() const override {return AstType::ClassBlock;}
        void setClassScope(SharedPtr<Scope> classScope);
        void setAccessor(String& classAccessor);
        void setCapturedScope(SharedPtr<Scope> capturedScope);

    };

class MethodBody : public CallableBody {
public:
    MethodBody(SharedPtr<Scope> scope);
    MethodBody(UniquePtr<CodeBlock>&& body);
    MethodBody(UniquePtr<CallableBody>* body);
    ~MethodBody();
    // Node evaluate(SharedPtr<Scope> scope) const override;
    // void printAST(std::ostream& os, int indent = 0) const;
    Vector<UniquePtr<BaseAST>>& getChildren(){return children;};
    UniquePtr<BaseAST> clone() const override;

    AstType getAstType() const override { return AstType::ClassMethodBlock;}

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

    Node evaluate(SharedPtr<Scope> scope) const override;

    // Print the AST for debugging.
    void printAST(std::ostream& os, int indent = 0) const override;
    // String toString() const override;
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
    Node evaluate(SharedPtr<Scope> scope) const override;

};



// Class Member Access AST Node
class ClassRef : public ASTStatement {
public:
    String instanceName;
    // Constructor: takes the instance variable name and the member name.
    ClassRef(String instanceName, SharedPtr<Scope> scope);
    UniquePtr<BaseAST> clone() const override;

    // Evaluate: retrieves the member from the class instance.
    // Node evaluate(SharedPtr<Scope> scope) const override;
    // void printAST(std::ostream& os, int indent = 0) const override;

    AstType getAstType() const override {return AstType::ClassReference;}
};


class MethodDef : public CallableDef {
protected:
    String accessor;
    CallableType methodType = CallableType::DEF;
    SharedPtr<Scope> classScope;
public:
    // Constructor: Force the FunctionType to METHOD.
    MethodDef(String name, ParamList parameters, UniquePtr<MethodBody> body, SharedPtr<Scope> scope);
    MethodDef(String name, ParamList parameters, UniquePtr<MethodBody> body, CallableType methodType, SharedPtr<Scope> scope);
    MethodDef(UniquePtr<FunctionDef> funcDef);
    ~MethodDef();

    // Node evaluate(SharedPtr<Scope> scope) const override;
    UniquePtr<BaseAST> clone() const override;

    String toString() const override;
    AstType getAstType() const override { return AstType::ClassMethodDef;}
    void setMethodAccessor(String& accessorName);
    String getMethodAccessor() const;
    Node evaluate(SharedPtr<Scope> scope) const override;
    ParamList& getParameters();
    const ParamList& getParameters() const;   // For inspection

    void setClassScope(SharedPtr<Scope> scope);
    SharedPtr<Scope> getClassScope() const;
    bool isConstructor() const;

};
    

class MethodCall : public CallableCall {
public:
    MethodCall(String methodName, Vector<UniquePtr<ASTStatement>> args, SharedPtr<Scope> scope);
    ~MethodCall();
    // Node evaluate(SharedPtr<Scope> scope) const override;
    AstType getAstType() const override { return AstType::ClassMethodCall;}
    // UniquePtr<BaseAST> clone() const override;
    // String toString() const override;

};

    
class MethodRef : public CallableRef {
protected:
    String accessor;
public:
    // Constructor: Force the FunctionType to METHOD.
    MethodRef(String name, SharedPtr<Scope> scope);

    Node evaluate(SharedPtr<Scope> scope) const override;

    AstType getAstType() const override {return AstType::ClassMethodRef;}
};





#endif // AST_CLASS_H








// class AttributeHandler {

// // AttributeHandler::AttributeHandler(String name);
// private:
//     String accessor;
//     String delimeter;
//     String varName;
//     String fullName;
// public:
//     AttributeHandler() {}
//     String getFullName() const;  // {return fullName;}
//     String getDelimiter() const; // {return delimeter;}
//     String getAccessor() const; // {return accessor;}
//     String getVarName() const; // {return varName;}
//     void setAccessor(String accessorVal); // {accessor = accessorVal;}
//     void setDelimiter(String delimiterVal); // {delimeter = delimiterVal;}
//     void setVarName(String varNameVal); //  {varName = varNameVal;}
//     void setFullName(String fullName);
// };


// class AttributeDeclaration : public VariableDeclaration, public AttributeHandler {
// public:
//     AttributeDeclaration(UniquePtr<VariableDeclaration> varDec);
//     AstType getAstType() const override {return AstType::AttributeDeclaration;}

//     String toString() const override;    
//     Node evaluate(SharedPtr<Scope> scope) const override;
//     void printAST(std::ostream& os, int indent = 0) const override;
//     UniquePtr<BaseAST> clone() const override;

// };


// class AttributeReference : public VariableReference, public AttributeHandler {
// public:
//     AttributeReference(UniquePtr<VariableReference> varRef);
//     AstType getAstType() const override {return AstType::AttributeReference;}

//     String toString() const override;    
//     Node evaluate(SharedPtr<Scope> scope) const override;
//     void printAST(std::ostream& os, int indent = 0) const override;
//     UniquePtr<BaseAST> clone() const override;

// };


// class AttributeAssignment : public VariableAssignment, public AttributeHandler {
// public:
//     AttributeAssignment(UniquePtr<VariableAssignment> varAssign);
//     AstType getAstType() const override {return AstType::AttributeAssignment;}

//     String toString() const override;    
//     Node evaluate(SharedPtr<Scope> scope) const override;
//     void printAST(std::ostream& os, int indent = 0) const override;
//     UniquePtr<BaseAST> clone() const override;
// };