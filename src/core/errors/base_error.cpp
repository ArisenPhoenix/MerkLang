#include "core/types.h"
#include "core/errors.h"




// Base Error Class: MerkError
// class MerkError : public RunTimeError {

MerkError::MerkError(const String& message)
        : RunTimeError(message), message(message) {}

String MerkError::errorString() const {
    return highlight("MerkError", Colors::red) + ": " + highlight(message, Colors::yellow);
}

const char* MerkError::what() const noexcept {
    errorStringCache = errorString(); // Cache the result
    return errorStringCache.c_str(); // Return the cached string
}

bool MerkError::isWhitespaceOnly(const String& str) const {
    return str.find_first_not_of(" \t\n\r") == String::npos;
}


