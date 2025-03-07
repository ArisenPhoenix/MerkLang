#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdexcept>
#include <string>
#include <sstream>

#include "core/tokenizer.h"
#include "core/context.h"

template <typename Container>
void ensureWithinBounds(
    const Container& container, 
    typename Container::size_type index, 
    const std::string& location = "Unknown location", 
    const std::string& errorMessage = "Out-of-bounds access") 
{
    if (index >= container.size()) {
        std::ostringstream oss;
        oss << errorMessage << "->" << location << ": attempted to access index " << index 
            << " but size is " << container.size();
        throw std::runtime_error(oss.str());
    }
    
}

inline VariantType validateAndConvertValue(const Token& token) {
    switch (token.type) {
        case TokenType::Number: {
            const auto& value = token.value;
            if (value.find('.') != String::npos) {
                // Float detected
                try {
                    return std::stof(value);
                } catch (const std::exception& e) {
                    throw RunTimeError("Failed to convert token to float: " + value);
                }
            } else {
                // Integer detected
                try {
                    return std::stoi(value);
                } catch (const std::exception& e) {
                    throw RunTimeError("Failed to convert token to int: " + value);
                }
            }
        }
        case TokenType::String:
            return token.value; // Already a string

        case TokenType::Bool: {
            const auto& value = token.value;
            if (value == "true" || value == "false") {
                return value == "true"; // Convert to boolean
            } else {
                throw RunTimeError("Invalid boolean token value: " + value);
            }
        }

        default:
            throw RunTimeError("Unsupported token type for value extraction: " + token.toString());
    }
}

inline VariantType resolveVariantTypeValue(const VariantType& value) {
    return std::visit([](const auto& val) -> VariantType {
        using T = std::decay_t<decltype(val)>;

        // Handle each supported type explicitly
        if constexpr (std::is_same_v<T, int>) {
            return val; // Already an integer
        } else if constexpr (std::is_same_v<T, float>) {
            return val; // Already a float
        } else if constexpr (std::is_same_v<T, String>) {
            // Check if the string represents a numeric value (int or float)
            if (!val.empty() && std::all_of(val.begin(), val.end(), ::isdigit)) {
                try {
                    return std::stoi(val); // Convert to int
                } catch (...) {
                    try {
                        return std::stof(val); // Convert to float if int fails
                    } catch (const std::exception& e) {
                        throw RunTimeError("Failed to convert string to numeric value: " + val);
                    }
                }
            }
            return val; // Return as string if not numeric
        } else if constexpr (std::is_same_v<T, bool>) {
            return val; // Already a boolean
        } else {
            throw RunTimeError("Unsupported VariantType for resolution.");
        }
    }, value);
}

// Was originally used for displaying context before a method was officially established
inline void keyValueDisplayContext(const Context& context) {
    for (const auto& [name, node] : context.getVariables()) {
        debugLog(true, name, " = ", node->getValue(), " type: ", node);
    }
    std::cout << "\n";
}


#endif //UTILITIES_H




