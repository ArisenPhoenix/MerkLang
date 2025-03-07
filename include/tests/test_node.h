#include "core/node.h"
#include "utilities/debugging_functions.h"
#include <iostream>
#include <cassert>
#include <cmath>

// This took a while to clear


// floatSum.toFloat
void testNode() {
    std::cout << "Running Node tests...\n";
    
    // Test basic Node initialization
    Node n1(10);
    assert(n1.isInt());
    assert(n1.toInt() == 10);
    
    Node n2(10.5);
    assert(n2.isDouble());
    assert(n2.toDouble() == 10.5);
    
    Node n3("Hello");
    assert(n3.isString());
    assert(n3.toString() == "Hello");
    
    Node n4(true);
    assert(n4.isBool());
    assert(n4.toBool() == true);
    
    // Test arithmetic operations
    Node sum = n1 + Node(5);
    assert(sum.isInt() && sum.toInt() == 15);
    
    Node floatSum = n1 + Node(2.5f);
    std::cout << "floatSum Type: " << floatSum.getTypeAsString()
          << ", Value: " << floatSum.toFloat() << std::endl;

    assert(floatSum.isFloat() &&
       std::fabs(floatSum.toFloat() - 12.5) < std::numeric_limits<float>::epsilon() * 10);

    
    Node doubleSum = n2 + Node(4.5);
    assert(doubleSum.isDouble() && doubleSum.toDouble() == 15.0);
    
    Node difference = n1 - Node(3);
    assert(difference.isInt() && difference.toInt() == 7);
    
    Node product = n1 * Node(3);
    assert(product.isInt() && product.toInt() == 30);
    
    Node quotient = Node(10.0) / Node(2.0);
    assert(quotient.isDouble() && quotient.toDouble() == 5.0);
    
    std::cout << "Node tests passed!\n";
}

void testLitNode() {
    std::cout << "Running LitNode tests...\n";
    
    LitNode l1(42);
    assert(l1.isInt());
    assert(l1.toInt() == 42);
    
    LitNode l2("Test");
    assert(l2.isString());
    assert(l2.toString() == "Test");
    
    LitNode l3(3.14);
    assert(l3.isDouble());
    assert(l3.toDouble() == 3.14);
    
    Node sum = l1 + LitNode(8);
    assert(sum.isInt() && sum.toInt() == 50);
    
    std::cout << "LitNode tests passed!\n";
}

void testVarNode() {
    std::cout << "Running VarNode tests...\n";
    
    VarNode v1(100, false, true, false);
    assert(v1.isInt());
    assert(v1.toInt() == 100);
    
    v1 += Node(50);
    assert(v1.toInt() == 150);
    
    v1 -= Node(10);
    assert(v1.toInt() == 140);
    
    v1 *= Node(2);
    assert(v1.toInt() == 280);
    
    v1 /= Node(4);
    assert(v1.toInt() == 70);
    
    // Ensure const variables cannot be modified
    VarNode v2(20, true, true, false);
    std::cout << "Before modification: v2.isConst = " << v2.isConst
            << ", v2.isMutable = " << v2.isMutable
            << ", v2.isStatic = " << v2.isStatic << std::endl;
    try {
        v2 += Node(10);
        std::cout << "VarNode Type After Modification: " << v2.getTypeAsString() 
          << ", Value: " << v2.toString() 
          << ", isMutable: " << v2.isMutable 
          << ", isStatic: " << v2.isStatic 
          << std::endl;
        assert(false); // Should not reach here
    } catch (const RunTimeError&) {
        std::cout << "Correctly prevented modification of const VarNode.\n";
    }
    
    std::cout << "VarNode tests passed!\n";
}

// int main() {
//     testNode();
//     testLitNode();
//     testVarNode();
//     std::cout << "All tests completed successfully.\n";
//     return 0;
// }
