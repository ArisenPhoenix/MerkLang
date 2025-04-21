#ifndef STREAMING_H
#define STREAMING_H
#include <any>
#include <vector>
#include <typeindex>
#include <stdexcept>
#include <string>
#include <iostream>
#include <unordered_map>

// Overload the streaming operator for std::type_index
inline std::ostream& operator<<(std::ostream& os, const std::type_index& type) {
    static const std::unordered_map<std::type_index, std::string> typeMap = {
        {typeid(int), "int"},
        {typeid(float), "float"},
        {typeid(double), "double"},
        {typeid(long double), "long double"},
        {typeid(char), "char"},
        {typeid(bool), "bool"},
        {typeid(std::string), "string"},
        {typeid(std::vector<int>), "vector<int>"},
        {typeid(std::vector<std::any>), "vector<any>"},
        {typeid(void), "void"}
    };

    auto it = typeMap.find(type);
    if (it != typeMap.end()) {
        os << it->second; // Write the string representation to the stream
    } else {
        os << "unknown"; // Default for unknown types
    }

    return os;
}




#endif //STREAMING_H