#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <iostream>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <string>
#include <type_traits>
#include <memory>
#include <filesystem>
#include "core/types.h"



// #include "core/node.h"
class VarNode;
class Node;
class LitNode;
#include "utilities/helper_functions.h"



// --- Helper templates - modified names for clarity ---
template <typename T>
class is_streamable2 {
private:
    template <typename U>
    static auto test(int) -> decltype(std::declval<std::ostream&>() << std::declval<U>(), std::true_type());
    template <typename>
    static std::false_type test(...);
public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template <typename T>
inline constexpr bool is_streamable_v2 = is_streamable2<T>::value;

template <typename T, typename = void>
struct has_toString2 : std::false_type {};

template <typename T>
struct has_toString2<T, decltype(std::declval<T>().toString(), void())> : std::true_type {};

template <typename T>
constexpr bool has_toString_v2 = has_toString2<T>::value;

// --- Log levels ---
// Add FLOW as a special log level.
enum class LogLevel {
    ERROR = 0,
    WARNING = 1,
    INFO = 2,
    DEBUG = 3,
    TRACE = 4,
    FLOW  = 100  // Special value; will be treated separately.
};

// Levels Describe Amount Of Detail in terms of Abstraction Level. High Would Be Higher Level Details, rather than smaller
enum class FlowLevel {
    VERY_HIGH = 0,  
    HIGH = 1,
    MED = 2,
    LOW = 3,
    VERY_LOW = 4,
    PERMISSIVE = 99,
    NONE = 100,
    UNKNOWN = 101,

};

// --- Debugger Singleton ---
class Debugger {
public:
    static Debugger& getInstance() {
        static Debugger instance;
        return instance;
    }

    // Global log level.
    void setGlobalLogLevel(LogLevel level);
    void setGlobalFlowLevel(FlowLevel level);

    LogLevel getLevel() const;

    // Optionally include a timestamp in output.
    void setIncludeTimestamp(bool include);
    bool getIncludeTimestamp() const;

    // Optionally include file info in output.
    void setIncludeFileInfo(bool include);
    bool getIncludeFileInfo() const;

    FlowLevel getFlowLevel();
    FlowLevel getFileFlowLevel(const String& file);

    // Set a file-specific log level and flow level.
    void setLogLevels(const String& file, LogLevel fileLoglevel, FlowLevel fileFlowLevel = FlowLevel::UNKNOWN);
    void printFiles();

    void setIncludeFlowLevel(bool include) { includeFlowLevel = include; }
    bool getIncludeFlowLevel() const { return includeFlowLevel; }
    

    String handleTime();
    String handleLevel(LogLevel level);
    String handleFileDisplay(String normFileName, int line);
    String handleFlowLevelDisplay(FlowLevel level);

    bool handleProceed(LogLevel level, const String& file);
    bool handleFlowProceed(FlowLevel level, const String& file);
    void printLog(String message);

    template<typename ... Args>
    void log(LogLevel level, const String &file, int line, Args&& ... args) {
        String fileName = normalizeFileName(file);
        if (handleProceed(level, fileName)){
            std::ostringstream oss;
            oss << handleLevel(level);
            oss << handleTime();
            oss << handleFileDisplay(fileName, line);
            oss << " ";
            ((oss << stringify(std::forward<Args>(args)) << " "), ...);
            
            printLog(oss.str());
        }
    }

    std::function<void()> flowEnter(const String &methodName, FlowLevel level, const String &file, int line);
    
    


private:
    LogLevel currentLevel = LogLevel::ERROR;
    std::unordered_map<String, LogLevel> fileLevels;
    std::unordered_map<String, FlowLevel> fileFlowLevels;

    bool includeTimestamp = false;    // Default: timestamp off
    bool includeFileInfo = true;      // Default: include file info
    bool includeFlowLevel = false;    // Default: Don't show FlowLevel

    FlowLevel flowLevel = FlowLevel::NONE;
    std::mutex mtx;

    Debugger() = default;
    Debugger(const Debugger&) = delete;
    Debugger& operator=(const Debugger&) = delete;

    String logLevelToString(LogLevel level) const;
    String flowLevelToString(FlowLevel level) const;


    String currentTime() const;

    // Helper: Convert an argument to a string.
    template <typename T>
    String stringify(const T& arg) {
        using ArgType = std::decay_t<T>;
        if constexpr (is_streamable_v2<ArgType>) {
            std::ostringstream oss;
            oss << arg;
            return oss.str();
        }
        else if constexpr (std::is_pointer_v<ArgType>) {
            return arg ? stringify(*arg) : "[null pointer]";
        }
        else if constexpr (std::is_same_v<ArgType, std::shared_ptr<VarNode>> ||
                           std::is_same_v<ArgType, std::unique_ptr<VarNode>>) {
            return arg ? stringify(*arg) : "[null VarNode pointer]";
        }
        else if constexpr (std::is_same_v<ArgType, std::shared_ptr<Node>> ||
                           std::is_same_v<ArgType, std::unique_ptr<Node>>) {
            return arg ? stringify(*arg) : "[null Node pointer]";
        }
        else if constexpr (has_toString_v2<ArgType>) {
            return arg.toString();
        }
        else {
            return "[Non-streamable type]";
        }
    }
    void setFileLogLevel(const String& file, LogLevel level);
    void setFileFlowLevel(const String& file, FlowLevel level);
};

#ifdef _MSC_VER
  #define FUNCTION_SIGNATURE __FUNCSIG__
#else
  #define FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#endif


// --- Overloaded helper functions for flow logging ---
// When no FlowLevel is provided, use FlowLevel::LOW to allow minute details
inline auto flowEnterHelper(const char* funcSig, const char* file, int line) {
    return Debugger::getInstance().flowEnter(
        highlight(funcSig, Colors::bold_blue),
        FlowLevel::LOW, file, line);
}

// // When a FlowLevel is provided, use it instead of a default.
inline auto flowEnterHelper(const char* funcSig, FlowLevel level, const char* file, int line) {
    return Debugger::getInstance().flowEnter(
        highlight(funcSig, Colors::bold_blue),
        level, file, line);
}



// Always define FUNCTION_SIGNATURE regardless of logging state
#ifdef _MSC_VER
  #define FUNCTION_SIGNATURE __FUNCSIG__
#else
  #define FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#endif







#define DEBUG_LOG(level, ...) Debugger::getInstance().log(level, __FILE__, __LINE__, __VA_ARGS__)
#define DEBUG_FLOW(...) \
    auto __DEBUG_FLOW_EXIT__ = flowEnterHelper(FUNCTION_SIGNATURE, __VA_OPT__(__VA_ARGS__, ) __FILE__, __LINE__)
#define DEBUG_FLOW_EXIT() __DEBUG_FLOW_EXIT__()
#define MARK_UNUSED_MULTI(...) \
    do { (void)([](auto&&... args){ ((void)args, ...); }(__VA_ARGS__)); } while (0)





 

namespace Debug {
    // for configuring the debugger in whatever combination(s) desired
    // can set FlowLevel, which basically allows for seeing the full trace of methods levels are based on abstraction level 
    // High would provide the least amount of information
    // can set LogLevel for which logs to include.
    // All can be done on a file based level as well to limit output etc.
    void configureDebugger();
}




#endif // DEBUGGER_H 

