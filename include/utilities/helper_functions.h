#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <string>
#include "core/types.h"
#include "core/node.h"


class Scope;

String joinPaths(const String& dir, const String& file);
String getFilePath(int argc, char* argv[], const String& defaultDir, const String& defaultFile);
String readFileContent(const String& filePath);
String getTypeName(const Node& value);  // Not Implemented Anymore -- perhaps a use for it later?
String readFile(String filepath);
void outputFileContents(String fileContent, int chars = 0);
String normalizeFileName(const String& filePath);
int printIndent(std::ostream& os, int indent);

bool validateScope(SharedPtr<Scope> scope, String methodName, String forWhat = "", bool debug = false);

String generateScopeOwner(String userName, String itemName);

#endif // HELPER_FUNCTIONS_H
