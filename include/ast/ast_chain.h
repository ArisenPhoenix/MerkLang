#include "core/types.h"
#include "ast/ast_base.h"

#include "utilities/helper_functions.h"
class Scope;



struct ChainElement {
    String name;
    IdentifierType type;
    String delimiter;
    void printAST(std::ostream& os, int indent = 0) const {
        printIndent(os, indent);
        os << "Name: " + name + ", Type: " + identifierTypeToString(type) + ", Delimiter: " + delimiter << std::endl;
    };
};

class Chain : public ASTStatement {
private:
    Vector<ChainElement> elements;
public:
    explicit Chain(SharedPtr<Scope> scope);
    
    void addElement(const ChainElement& element);

    const Vector<ChainElement>& getElements() const;

    AstType getAstType() const override {return AstType::Chain;}
    void printAST(std::ostream& os, int indent = 0) const override;

    // Example of a resolution function that walks the chain:
    Node resolve(SharedPtr<Scope> scope) const;
    void assign(SharedPtr<Scope> scope, const Node& value) const;

    virtual String toString() const override;
    
    virtual Node evaluate(SharedPtr<Scope> scope) const override;
    Node evaluate() const {return evaluate(getScope());}
    
    virtual UniquePtr<BaseAST> clone() const override;
};
