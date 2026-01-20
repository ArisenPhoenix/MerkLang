#pragma once

#include <stdexcept>
#include <string>
#include <sstream>

#include "core/Tokenizer.hpp"
#include "core/registry/Context.hpp"


class Timer {
private:
    std::chrono::steady_clock::time_point t0;

public:
    Timer() : t0(std::chrono::steady_clock::now()) {}

    double elapsed() const {
        auto t1 = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(t1 - t0).count();
    }

    void reset() {
        t0 = std::chrono::steady_clock::now();
    }

    double printElapsed(const std::string& label) const {
        auto t1 = std::chrono::steady_clock::now();
        double seconds = std::chrono::duration<double>(t1 - t0).count();
        std::cout << label << ": " << seconds << " sec\n";
        return seconds;
    }
};
// // Was originally used for displaying context before a method was officially established
// inline void keyValueDisplayContext(const Context& context) {
//     for (const auto& [name, node] : context.getVariables()) {
//         debugLog(true, name, " = ", node->getValue(), " type: ", node);
//     }
//     std::cout << "\n";
// }




