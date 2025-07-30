#include "core/types.h"
#include "utilities/debugger.h"
#include <algorithm>
#include <iomanip> // For setw, left, etc.
#include <unordered_map>
#include <sstream>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <regex>
#pragma GCC diagnostic pop




String stripAnsi(const String& input) {
    static const std::regex ansiPattern("\033\\[[0-9;]*m");
    return std::regex_replace(input, ansiPattern, "");
}
// Global log level.
void Debugger::setGlobalLogLevel(LogLevel level) { currentLevel = level; }
void Debugger::setGlobalFlowLevel(FlowLevel level) {flowLevel = level;}

LogLevel Debugger::getLogLevel() const { return currentLevel; }
FlowLevel Debugger::getFlowLevel() {return flowLevel;}
void Debugger::setGlobalLevels(LogLevel logLevel, FlowLevel flowLevel){
    setGlobalLogLevel(logLevel);
    setGlobalFlowLevel(flowLevel);
}

String Debugger::logLevelToString(LogLevel level) const {
    switch(level) {
        case LogLevel::ERROR:      return highlight("ERROR", Colors::bg_red);
        case LogLevel::WARNING:    return highlight("WARNING", Colors::orange);
        case LogLevel::INFO:       return highlight("INFO", Colors::bold_purple);
        case LogLevel::DEBUG:      return highlight("DEBUG", Colors::bold_yellow);
        case LogLevel::TRACE:      return highlight("TRACE", Colors::red);
        case LogLevel::FLOW:       return highlight("FLOW", Colors::bold_red);
        case LogLevel::PERMISSIVE: return highlight("PERMISSIVE", Colors::bg_bright_yellow);
        default:                   return highlight("UNKNOWN", Colors::bg_red);
    }
}
String Debugger::flowLevelToString(FlowLevel level) const {
    switch (level) {
        case FlowLevel::VERY_HIGH: return highlight("Very High", Colors::bold_blue);
        case FlowLevel::HIGH:      return highlight("HIGH", Colors::cyan);
        case FlowLevel::MED:       return highlight("MED", Colors::green);
        case FlowLevel::LOW:       return highlight("LOW", Colors::yellow);
        case FlowLevel::VERY_LOW:  return highlight("Very LOW", Colors::red);
        case FlowLevel::FLOW:      return highlight("FLOW", Colors::bg_magenta);

        case FlowLevel::NONE:      return highlight("NONE", Colors::bg_bright_red);
        case FlowLevel::PERMISSIVE: return highlight("PERMISSIVE", Colors::bg_magenta);
        case FlowLevel::UNKNOWN:   return highlight("Unknown", Colors::pink);
        
    
    default:
        return "UNKNOWN";
    }
    
}

void Debugger::printLog(String message){
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << message << std::endl;
}

String Debugger::currentTime() const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_time;
    
#if defined(_MSC_VER)
    localtime_s(&tm_time, &now_time);
#else
    localtime_r(&now_time, &tm_time);
#endif
    std::ostringstream timeStream;
    timeStream << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S");
    return highlight(timeStream.str(), Colors::yellow);
}



// include a timestamp.
void Debugger::setIncludeTimestamp(bool include) { includeTimestamp = include; }
bool Debugger::getIncludeTimestamp() const { return includeTimestamp; }

// include file info.
void Debugger::setIncludeFileInfo(bool include) { includeFileInfo = include; }
bool Debugger::getIncludeFileInfo() const { return includeFileInfo; }


// Set a file-specific log level.
void Debugger::setFileLogLevel(const String& file, LogLevel level) {
    std::lock_guard<std::mutex> lock(mtx);        
    fileLevels[file] = level;
}

void Debugger::setFileFlowLevel(const String& file, FlowLevel level){
    std::lock_guard<std::mutex> lock(mtx);
    fileFlowLevels[file] = (level != FlowLevel::UNKNOWN) ? level : flowLevel;
}

void Debugger::setLogLevels(const String& file, LogLevel fileLoglevel, FlowLevel fileFlowLevel) {
    setFileLogLevel(file, fileLoglevel);
    setFileFlowLevel(file, fileFlowLevel);
}

void Debugger::printFiles(){
    Vector<String> fileNames;
    for (auto& file : fileLevels){
        fileNames.emplace_back(file.first);
    }
    std::sort(fileNames.begin(), fileNames.end());

    // Set column widths.
    const int fileColWidth = 40;
    const int logLevelColWidth = 20;
    const int flowLevelColWidth = 20;

    // Print header.
    std::cout << std::left 
              << std::setw(fileColWidth) << "FILE" 
              << std::setw(logLevelColWidth) << "LOG LEVEL" 
              << std::setw(flowLevelColWidth) << "FLOW LEVEL" 
              << std::endl;

    std::cout << String(fileColWidth + logLevelColWidth + flowLevelColWidth, '-') << std::endl;

    // Print each row.
    for (const auto& fileName : fileNames){
        String logLevelStr = logLevelToString(fileLevels[fileName]);
        String flowLevelStr = flowLevelToString(getFileFlowLevel(fileName));
    
        // Stripped versions for no color codes as they distort the table
        String logLevelStripped = stripAnsi(logLevelStr);
        String flowLevelStripped = stripAnsi(flowLevelStr);
    
        std::cout << std::left 
                  << std::setw(fileColWidth) << fileName
                  << std::setw(logLevelColWidth) << logLevelStripped
                  << std::setw(flowLevelColWidth) << flowLevelStripped
                  << std::endl;
    }
    std::cout << std::endl;
}


String Debugger::handleTime() {
    return includeTimestamp ? "[" + currentTime() + "]" : "";
}

String Debugger::handleLevel(LogLevel level) {
    return "[" + logLevelToString(level) + "]";

}

String Debugger::handleLevel(FlowLevel level) {
    return "[" + flowLevelToString(level) + "]";

}

String Debugger::handleFileDisplay(String filePath, int line) {
    return includeFileInfo ? "[" + filePath + "][" + std::to_string(line) + "] " : " ";
}

FlowLevel Debugger::getFileFlowLevel(const String& file) {
    if (auto it = fileFlowLevels.find(file); it != fileFlowLevels.end()) {
        return it->second;
    }
    return flowLevel;
}

 

bool Debugger::handleLogProceed(LogLevel level, const String& file){
    if (!enabled) {return false;}
    if (level == LogLevel::PERMISSIVE){
        return true;
    }
    if (level == LogLevel::FLOW) {
        // USED FOR LOGGING EXTRA INFORMATION WITHIN THE NORMAL DEBUG_FLOW...flow
        if (flowLevel == FlowLevel::NONE){
            return false;
        } else {
            FlowLevel fileFlowLevel = getFileFlowLevel(file);
            return handleFlowProceed(fileFlowLevel, file);
        }

    } else {
        // Determine effective log level (global or file-specific).
        LogLevel effectiveLevel = currentLevel;
        String fileName = normalizeFileName(file);
        {
            auto it = fileLevels.find(file);
            if (it != fileLevels.end())
                effectiveLevel = it->second;
        }
        if (static_cast<int>(level) > static_cast<int>(effectiveLevel))
            return false;
    }
    return true;
}

bool Debugger::handleFlowProceed(FlowLevel methodFlowLevel, const String& file){
    if (!enabled) {return false;}
    if (methodFlowLevel == FlowLevel::PERMISSIVE){
        return true;
    }
    if (static_cast<int>(flowLevel) >= static_cast<int>(FlowLevel::NONE)){
        return false;
    }

    FlowLevel fileFlowLevel = getFileFlowLevel(file);

    if (flowLevel == FlowLevel::PERMISSIVE){

        if (static_cast<int>(methodFlowLevel) <= static_cast<int>(fileFlowLevel) && fileFlowLevel != FlowLevel::NONE){
            return true;
        }
        return false;
    }

    // Check if the file flow level is <= global flow level
    if (static_cast<int>(fileFlowLevel) <= static_cast<int>(flowLevel)) {
        if (static_cast<int>(methodFlowLevel) <= static_cast<int>(fileFlowLevel)) {
            return true;
        }
    }

    return false;
    
}

void Debugger::setEnabled(bool enable) {enabled = enable;}



void Debugger::handleLogDisplay(LogLevel level, const String &normFile, int line, String args) {
        std::ostringstream oss;
        oss << handleLevel(level);
        oss << handleTime();
        oss << handleFileDisplay(normFile, line);
        oss << args << " ";
        
        printLog(oss.str());
}

void Debugger::handleFlowDisplay(FlowLevel level, const String &normFile, int line, String methodName, bool entering) {
    std::ostringstream oss;
    oss << handleLevel(FlowLevel::FLOW) << handleLevel(level)
    << handleTime()
    << handleFileDisplay(normFile, line) 
    << highlight(entering ? "Entering: " : "Exiting", Colors::orange)
    << highlight(methodName, Colors::bold_blue);
    printLog(oss.str());
}


std::function<void()> Debugger::flowEnter(const String &methodName, FlowLevel flowLevel, const String &file, int line) {
    String normFile = normalizeFileName(file);
    if (!handleFlowProceed(flowLevel, normFile)){
        return [](){};
    }
    handleFlowDisplay(flowLevel, normFile, line, methodName, true);


    // Return a lambda that logs the "exiting" message using the same context.
    return [this, normFile, line, methodName, flowLevel]() {
        handleFlowDisplay(flowLevel, normFile, line, methodName, false);
    };
}


