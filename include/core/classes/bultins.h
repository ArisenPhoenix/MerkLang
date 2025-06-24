#include "core/classes/native_class.h"
#include "core/classes/class_base.h"
#include "core/functions/param_node.h"

class Scope;
class Callable;

using NativeClassFactory = std::function<SharedPtr<NativeClass>(SharedPtr<Scope>)>;
std::unordered_map<String, SharedPtr<CallableSignature>> getAllNativeClasses(SharedPtr<Scope>);

