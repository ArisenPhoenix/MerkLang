#pragma once

#include "core/TypesFWD.hpp"
#include "core/errors.h"
#include <optional>
enum class CoerceMode : uint8_t {
    Strict,    
    Permissive
};

// struct CoerceError : MerkError {
//     using MerkError::MerkError;
// };

struct Coercer {
    static int         toInt   (const VariantType& v, CoerceMode m = CoerceMode::Permissive);
    static double      toDouble(const VariantType& v, CoerceMode m = CoerceMode::Permissive);
    static float       toFloat (const VariantType& v, CoerceMode m = CoerceMode::Permissive);
    static bool        toBool  (const VariantType& v, CoerceMode m = CoerceMode::Permissive);
    static char        toChar  (const VariantType& v, CoerceMode m = CoerceMode::Permissive);
    static String      toString(const VariantType& v, CoerceMode m = CoerceMode::Permissive);
};


namespace TypeEvaluator {
    template<class T>
    T to(const VariantType& v, CoerceMode m = CoerceMode::Strict);

    template<class T>
    T as(const VariantType& v);

    template <typename T>
    T parseExact(std::string_view s, const char* what);
    NodeValueType inferTypeFromString (String& valueStr, String& typeStr);
    NodeValueType getNodeTypeFromString(String& type);

    std::optional<std::string_view> tryStringView(const VariantType& v);

    NodeValueType getNodeTypeFromValue(VariantType&);

    std::pair<VariantType, NodeValueType> getCoercedStringAndType(const String& value, String& typeStr);
    NodeValueType getTypeFromValue(const VariantType& value);
}   
