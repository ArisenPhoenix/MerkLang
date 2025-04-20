#include "utilities/helper_functions.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <variant>
#include <iostream>
#include "core/types.h"
#include "core/scope.h"
#include "utilities/debugging_functions.h"


String joinPaths(const String& dir, const String& file) {
    if (dir.empty()) return file;
    if (dir.back() == '/' || dir.back() == '\\') return dir + file;
    return dir + "/" + file;
}

String getFilePath(int argc, char* argv[], const String& defaultDir, const String& defaultFile) {
    String baseDir = "../"; // Adjust relative path from 'build' directory to project root
    if (argc == 2) {
        return baseDir + joinPaths(defaultDir, argv[1]);
    } else {
        std::cout << "No file specified. Using default: " << defaultFile << "\n";
        return baseDir + joinPaths(defaultDir, defaultFile);
    }
}

String readFileContent(const String& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Cannot open file '" + filePath + "'. Please ensure the file exists and is accessible.");
    }
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

int printIndent(std::ostream& os, int indent) {
    os << std::string(indent, ' '); // Print `indent` number of spaces
    return indent + 2;
}


void outputFileContents(String fileContent, int chars){
    std::cout << "File content loaded. Size: " << fileContent.size() << " bytes\n";
    std::cout << "First " << chars << " characters of the merk Code:\n"
      << fileContent.substr(0, std::min(fileContent.size(), size_t(chars))) << "\n";
}

String readFile(String filePath) {
    String content;
    try {
        content = readFileContent(filePath);
        

    } catch (const std::exception& ex) {
        std::cerr << "Error reading file: " << ex.what() << std::endl;
    }

    if (content.empty()) {
        throw RunTimeError("Error: File is empty or unreadable.");
    }
    
    return content;
}

bool validateScope(SharedPtr<Scope> scope, String methodName, String forWhat, bool debug) {
    if (!scope) {
        throw RunTimeError(methodName+":" + " Null scope passed" + (!forWhat.empty() ? "For " + forWhat : ""));
    }
    debugLog(debug);
    debugLog(debug, methodName+":", "Scope validated for", (!forWhat.empty() ? forWhat : ",", "scope level: "), scope->getScopeLevel(), "\n");
    return true;
}

String toLower(const String& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

String normalizeFileName(const String& filePath) {
    std::filesystem::path p(filePath);
    std::filesystem::path normalized;
    bool found = false;
    // Iterate over each element in the path.
    for (const auto& part : p) {
        String partLower = toLower(part.string());

        // When encountering "src" or "include", start including subsequent parts.
        if (partLower == "src" || partLower == "include") {
            found = true;
            continue; // Skip the folder itself if you want.
        }
        if (found) {
            normalized /= part;
        }
    }
    
    // If "src" or "include" wasn't found, just use the filename.
    // This is for root files that may be used, currently it is where main.cpp and debugger_config.cpp is found
    if (normalized.empty())
        normalized = p.filename();


    return normalized.string();
}





String generateScopeOwner(String userName, String itemName) {
    return userName + "(" + itemName + ")";
}
