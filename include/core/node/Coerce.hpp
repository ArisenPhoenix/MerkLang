#include "core/TypesFWD.hpp"
#include "core/errors.h"

enum class CoerceMode : uint8_t {
    Strict,      // only safe / explicitly allowed coercions
    Permissive   // numeric widening, string->number if parseExact, etc
};

struct CoerceError : MerkError {
    using MerkError::MerkError;
};

struct Coercer {
    static int         toInt   (const VariantType& v, CoerceMode m = CoerceMode::Permissive);
    static double      toDouble(const VariantType& v, CoerceMode m = CoerceMode::Permissive);
    static float       toFloat (const VariantType& v, CoerceMode m = CoerceMode::Permissive);
    static bool        toBool  (const VariantType& v, CoerceMode m = CoerceMode::Permissive);
    static char        toChar  (const VariantType& v, CoerceMode m = CoerceMode::Permissive);
    static String      toString(const VariantType& v, CoerceMode m = CoerceMode::Permissive);

    template<class T>
    static T to(const VariantType& v, CoerceMode m = CoerceMode::Permissive);
};
