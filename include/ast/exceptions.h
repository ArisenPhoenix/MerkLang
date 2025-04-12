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

class ContinueException : public std::exception {
public:
    const char* what() const noexcept override {
        return "Continue statement encountered.";
    }
};


class ReturnException : public std::exception {
    private:
        Node returnValue;
    
    public:
        explicit ReturnException(Node value) : returnValue(std::move(value)) {}
    
        const char* what() const noexcept override {return "Return statement encountered.";}
    
        Node getValue() const { return returnValue; }
};
// Should probably include the return exception here too