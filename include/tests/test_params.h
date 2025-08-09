// #ifndef TEST_PARAM_NODE_H
// #define TEST_PARAM_NODE_H

// #include "core/functions/param_node.h"
// #include <cassert>
// #include <iostream>

// // also took a while to clear

// // Function to test ParamNode behavior
// void testParamNodeBasic() {
//     std::cout << "Running testParamNodeBasic()..." << std::endl;

//     // Create a parameter without a default value
//     ParamNode param1("param1", NodeValueType::Int, false, false);

//     // Check name
//     assert(param1.getName() == "param1");

//     // Check type
//     assert(param1.getType() == NodeValueType::Int);

//     // Check no default value exists
//     assert(!param1.hasDefault());

//     // Assign a value and verify
//     param1.setValue(10);
//     assert(std::get<int>(param1.getValue()) == 10);

//     std::cout << "testParamNodeBasic() passed!\n" << std::endl;
// }

// // Function to test default value handling
// void testDefaultValueHandling() {
//     std::cout << "Running testDefaultValueHandling()..." << std::endl;

//     // Create a parameter with a default value
//     ParamNode param2("param2", 42, false, false);

//     // Check name
//     assert(param2.getName() == "param2");

//     // Check type
//     assert(param2.getType() == NodeValueType::Int);

//     // Verify default value exists
//     assert(param2.hasDefault());

//     // Check stored default value
//     assert(std::get<int>(param2.getDefaultValue()) == 42);

//     std::cout << "testDefaultValueHandling() passed!\n" << std::endl;
// }

// // Function to test type enforcement
// void testTypeEnforcement() {
//     std::cout << "Running testTypeEnforcement()..." << std::endl;

//     // Create a parameter with type enforcement
//     ParamNode param3("param3", NodeValueType::Float, false, false);

//     // Set valid value
//     param3.setValue(3.14f);
//     assert(std::get<float>(param3.getValue()) == 3.14f);

//     try {
//         param3.setValue(10);  // Should throw an error due to type mismatch
//         assert(false);        // Should never reach this line
//     } catch (const RunTimeError& e) {
//         std::cout << "Caught expected type mismatch error: " << e.what() << std::endl;
//     }

//     std::cout << "testTypeEnforcement() passed!\n" << std::endl;
// }

// // Function to test constant parameter behavior
// void testConstParam() {
//     std::cout << "Running testConstParam()..." << std::endl;

//     try {
//         // Create a constant parameter
//         ParamNode param4("param4", NodeValueType::Int, true, false);

//         // Set initial value
//         param4.setValue(100);
//         assert(std::get<int>(param4.getValue()) == 100);

//         // Attempt reassignment (should throw an exception)
//         std::cout << "Attempting to reassign param4 (should fail)..." << std::endl;
//         param4.setValue(200); // ❌ This should throw!

//         // If it gets here, the test failed
//         std::cerr << "testConstParam() failed: Expected an exception, but no exception was thrown." << std::endl;
//         exit(EXIT_FAILURE);
//     }
//     catch (const RunTimeError& e) {
//         std::cout << "Caught expected constant reassignment error: " << e.what() << std::endl;
//     }
//     catch (const std::runtime_error& e) {
//         std::cerr << "Caught unexpected std::runtime_error: " << e.what() << std::endl;
//         exit(EXIT_FAILURE);
//     }
//     catch (const std::exception& e) {
//         std::cerr << "Caught unexpected std::exception: " << e.what() << std::endl;
//         exit(EXIT_FAILURE);
//     }
//     catch (...) {
//         std::cerr << "Caught unknown exception type!" << std::endl;
//         exit(EXIT_FAILURE);
//     }

//     std::cout << "testConstParam() passed!\n" << std::endl;
// }





// // Function to test ParamNode toString() method
// void testToString() {
//     std::cout << "Running testToString()..." << std::endl;

//     // Create a parameter
//     ParamNode param5("param5", 99, false, true);

//     std::cout << "param5.toString(): " << param5.toString() << std::endl;

//     assert(param5.toString().find("param5") != std::string::npos);
//     assert(param5.toString().find("Int") != std::string::npos);
//     assert(param5.toString().find("Default: 99") != std::string::npos);

//     std::cout << "testToString() passed!\n" << std::endl;
// }

// // Run all tests
// void runAllParamNodeTests() {
//     testParamNodeBasic();
//     testDefaultValueHandling();
//     testTypeEnforcement();
//     testConstParam();
//     testToString();
//     std::cout << "All ParamNode tests passed successfully!" << std::endl;
// }

// #endif // TEST_PARAM_NODE_H
