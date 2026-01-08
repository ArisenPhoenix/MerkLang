#ifndef DEBUGGING_FUNCTIONS_H
#define DEBUGGING_FUNCTIONS_H

#include <iostream>
#include <variant>
#include <sstream>
#include <unordered_map> // Include for variable context
#include <iomanip>
#include <type_traits>

#include "core/types.h"
#include "core/Tokenizer.hpp" // Include the tokenizer header file


// class VarNode;

// Check if a type is streamable to std::ostream
template <typename T>
class is_streamable {
private:
    template <typename U>
    static auto test(int) -> decltype(std::declval<std::ostream&>() << std::declval<U>(), std::true_type());

    template <typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// Helper variable template for easy usage
template <typename T>
inline constexpr bool is_streamable_v = is_streamable<T>::value;

// Helper to check if a type has a `toString()` method
template <typename T, typename = void>
struct has_toString : std::false_type {};

template <typename T>
struct has_toString<T, decltype(std::declval<T>().toString(), void())> : std::true_type {};

template <typename T>
constexpr bool has_toString_v = has_toString<T>::value;



// The original DEBUG_LOG, was useful for the beginning, but when the number of files kept increasing
// a more robust solution was needed. It is still useful for printing dynamic output however, but should probably rename it
template <typename... Args>
inline void debugLog(bool condition, Args&&... args) {
    if (!condition) {
        return;
    }

    std::ostringstream oss;

    // Process each argument in the fold expression
    ([&oss](const auto& arg) {
        using ArgType = std::decay_t<decltype(arg)>;
        if constexpr (is_streamable_v<ArgType>) {
            // Streamable types
            oss << arg;
        } else if constexpr (std::is_pointer_v<ArgType>) {
            // Raw pointers
            if (arg) {
                oss << *arg;
            } else {
                oss << "[null pointer]";
            }
        } else if constexpr (std::is_same_v<ArgType, SharedPtr<VarNode>> || 
                             std::is_same_v<ArgType, UniquePtr<VarNode>>) {
            // Handle UniquePtr<VarNode>
            if (arg) {
                oss << *arg; // This will use VarNode's operator<<
            } else {
                oss << "[null UniquePtr<VarNode>]";
            }
        } else if constexpr (std::is_same_v<ArgType, SharedPtr<Node>> || 
                             std::is_same_v<ArgType, UniquePtr<Node>>) {
            // Smart pointers to Node
            if (arg) {
                oss << *arg;
            } else {
                oss << "[null smart pointer]";
            }
        } else if constexpr (has_toString_v<ArgType>) {
            // Objects with a toString() method
            oss << arg.toString();
        } else {
            // Non-streamable or unknown types
            oss << "[Non-streamable type]";
        }
        oss << " "; // Add space between arguments
    }(std::forward<Args>(args)), ...);

    // Output the combined result
    std::cout << oss.str() << std::endl;
}



inline void displayTokenInformation(Vector<Token> tokens) {
    for (const auto& token : tokens) {
        std::cout << "Token(Type: " << tokenTypeToString(token.type)
        << ", Value: \"" << token.value
        << "\", Line: " << token.line
        << ", Column: " << token.column << ")\n";
    }
}



#endif // DEBUGGING_FUNCTIONS_H













