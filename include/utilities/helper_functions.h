#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <string>
#include "core/node/Node.hpp"

#include "core/types.h"


class Scope;

String joinPaths(const String& dir, const String& file);
String getFilePath(int argc, char* argv[], const String& defaultDir, const String& defaultFile);
String readFileContent(const String& filePath);
String readFile(String filepath);
void outputFileContents(String fileContent, int chars = 0);
String normalizeFileName(const String& filePath);
int printIndent(std::ostream& os, int indent);

bool validateScope(SharedPtr<Scope> scope, String methodName, String forWhat = "", bool debug = false);

String generateScopeOwner(String userName, String itemName);
String joinUnorderedSetStrings(const std::unordered_set<String>& input, const String& delimiter = ",");
String joinVectorNodeStrings(const NodeList& nodes, const String& delimiter = ",");
String joinVectorStrings(const Vector<String>, const String& delimiter = ",");
#endif // HELPER_FUNCTIONS_H
