#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <typeinfo>
#include <cxxabi.h>
#include <optional>
#include <typeindex>

class BreakException : public std::exception {
public:
    const char* what() const noexcept override {
        return "Break statement executed.";
    }
};


// Should probably include the return exception here too