#include "core/types.h"
#include "ast/ast_base.h"

#include "utilities/helper_functions.h"
class Scope;

enum class ResolutionMode {
    Normal,
    ClassInstance,
    BaseClass,
    Global,
    Module,
};

// struct ChainElement {
//     String name;
//     IdentifierType type;
//     String delimiter;
//     void printAST(std::ostream& os, int indent = 0) const {
//         printIndent(os, indent);
//         os << "Name: " + name + ", Type: " + identifierTypeToString(type) + ", Delimiter: " + delimiter << std::endl;
//     };
// };

struct ChainElement {
    String name;
    UniquePtr<BaseAST> object;
    String delimiter;
    TokenType type;
    ChainElement();
    // Move constructor
    ChainElement(ChainElement&& other) noexcept;
    ~ChainElement();
    void clear();
    // Move assignment operator
    ChainElement& operator=(ChainElement&& other) noexcept;

    // Delete copy constructor (or default if you don't want accidental copies)
    ChainElement(const ChainElement&) = delete;
    ChainElement& operator=(const ChainElement&) = delete;
    void printAST(std::ostream& os, int indent = 0) const {
        // printIndent(os, indent);
        object->printAST(os, indent);
        // os << "Name: " + name + ", Type: " + identifierTypeToString(type) + ", Delimiter: " + delimiter << std::endl;
    };
};

class Chain : public ASTStatement {
private:
    Vector<ChainElement> elements;
    WeakPtr<Scope> classScope; // ← set when evaluating in class
    int resolutionStartIndex = 0;  // default is 0, resolve from the beginning
    ResolutionMode mode = ResolutionMode::Normal;


public:
    explicit Chain(SharedPtr<Scope> scope);
    ~Chain();
    void clear();
    // void addElement(const ChainElement& element);
    void addElement(ChainElement&& elem);
    void replaceLastElementWith(ChainElement&& elem);
    void setResolutionStartIndex(int index);

    const Vector<ChainElement>& getElements() const;

    AstType getAstType() const override {return AstType::Chain;}
    void printAST(std::ostream& os, int indent = 0) const override;

    // Example of a resolution function that walks the chain:
    // Node resolve(SharedPtr<Scope> scope) const;
    // void assign(SharedPtr<Scope> scope, const Node& value) const;

    int getResolutionStartIndex() const;
    ResolutionMode getResolutionMode() const;
    SharedPtr<Scope> getSecondaryScope() const;

    virtual String toString() const override;
    
    virtual Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const {return evaluate(getScope());}

    void setSecondaryScope(SharedPtr<Scope> secondary);
    void setResolutionMode(ResolutionMode newMode);
    
    virtual UniquePtr<BaseAST> clone() const override;
    Vector<const BaseAST*> getAllAst(bool includeSelf) const override;
    void setScope(SharedPtr<Scope> scope) override;
    // SharedPtr<Scope> getClassScope();

};
