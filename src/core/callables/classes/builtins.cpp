#include "core/types.h"
#include "core/scope.h"
#include "core/callables/classes/bultins.h"
#include "core/callables/classes/class_base.h"
#include "core/callables/classes/native_method.h"
#include "core/callables/classes/native_class.h"
#include "core/callables/classes/node_structures.h"
 

SharedPtr<NativeClass> createNativeListClass(SharedPtr<Scope> globalScope) {
    if (!globalScope) {throw MerkError("Class Scope Not Provided to Native Class List");}
    
    SharedPtr<Scope> classDefCapturedScope = globalScope->detachScope({});
    if (!classDefCapturedScope) { throw MerkError("classDefCapturedScope Was Not Made");}
    auto classScope = classDefCapturedScope->makeCallScope();
    if (!classScope) {throw MerkError("Class Scope Was Not Made");}

    
    
    String className = "List";
    String accessor = "list";
    // NativeClass(className, accessor, classScope);
    auto listClass = makeShared<NativeClass>(className, accessor, classScope);
    DEBUG_LOG(LogLevel::PERMISSIVE, "Got the List Class initialized");
    ParamList parameters;
    auto param = ParamNode("items", NodeValueType::Any);
    param.setIsVarArgsParam(true);
    parameters.addParameter(param);
    listClass->setParameters(parameters);
    listClass->setCapturedScope(classDefCapturedScope);

    // if (!listClass->getCapturedScope()) {throw MerkError("The captured scope was not set for class list");}

    auto construct = [className](NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        (void)args; (void)callScope; (void)self;
        // ListNode(args);
        auto listImpl = makeShared<ListNode>(args);
        DEBUG_LOG(LogLevel::PERMISSIVE, "list data: ", listImpl->toString(), "Type: ", listImpl->getTypeAsString());
        // auto data = VarNode(listImpl);
        auto var = makeUnique<VarNode>(listImpl);
        var->name = className;
        if (!var->isList()) {throw MerkError("Var created is not a list");}
        
        DEBUG_LOG(LogLevel::PERMISSIVE, "vector Var: ", var);
        
        self->getInstanceScope()->declareVariable("vector", std::move(var));
        auto vector = self->getInstanceScope()->getVariable("vector");
        if (vector.isList()) {
            // auto data = vector.getValue();
            // auto list = std::get<ListNode>(vector.getValue());
            // auto data = static_cast<ListNode>(list);
            // DEBUG_LOG(LogLevel::PERMISSIVE, data.getElements());

            DEBUG_LOG(LogLevel::PERMISSIVE, "IS a ClassInstance");
        } 

        // else {
        //     DEBUG_LOG(LogLevel::PERMISSIVE, vector);
        //     DEBUG_LOG(LogLevel::PERMISSIVE, vector.getTypeAsString());
        //     throw MerkError("vector is not a list");
        // }
        return vector;
    };

    DEBUG_LOG(LogLevel::PERMISSIVE, "construct method created");
    auto constructName = "construct";


    // NativeMethod(constructName, parameters.clone(), classScope, construct);
    auto constructMethod = makeShared<NativeMethod>(constructName, parameters.clone(), classScope, construct);
    DEBUG_LOG(LogLevel::PERMISSIVE, "Constructor Added");
    listClass->addMethod(constructName, constructMethod);


    classDefCapturedScope->appendChildScope(classScope);
    globalScope->appendChildScope(classDefCapturedScope);

    auto appendFunction = [](NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        (void)args; (void)callScope; (void)self;
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        auto instanceScope = self->getInstanceScope();
        auto vector = instanceScope->getVariable("vector");
        auto data = std::get<SharedPtr<ListNode>>(vector.getValue());
        for (auto& arg : args) {
            data->append(arg);
        }
        // auto data = std::static_pointer_cast<SharedPtr<ListNode>>(vector.getValue());
        DEBUG_LOG(LogLevel::PERMISSIVE, "Append varNode is: ", vector);
        DEBUG_LOG(LogLevel::PERMISSIVE, "type held: ", vector.getTypeAsString());
        // auto list = std::static_pointer_cast<ClassInstance>(std::get<SharedPtr<ClassInstance>>(vector.data.value))
        return Node();  // None
    };

    auto appendName = "append";
    // NativeMethod("append", parameters, classScope, appendFunction);
    auto appendMethod = makeShared<NativeMethod>(appendName, parameters, classScope, appendFunction);
    listClass->addMethod(appendName, appendMethod);

    return listClass;
}






std::unordered_map<String, NativeClassFactory> nativeClassFactories = {
    {"List", createNativeListClass},
};


std::unordered_map<String, SharedPtr<ClassBase>> getAllNativeClasses(SharedPtr<Scope> globalScope) {
    std::unordered_map<String, SharedPtr<ClassBase>> builtins;
    for (auto& [funcName, classSigCall] : nativeClassFactories) {
        if (funcName.empty()) {throw MerkError("FuncName empty");}

        DEBUG_LOG(LogLevel::PERMISSIVE, "Getting Class: ", funcName);
        
        auto nativeCls = classSigCall(globalScope); // safe
        builtins.emplace(funcName, nativeCls);
    }
    return builtins;
}





// SharedPtr<NativeClass> createNativeArrayClass([[maybe_unused]] SharedPtr<Scope> classScope) {
//     auto cls = NativeClass("List", "list", classScope);
//     auto listClass = makeShared<NativeClass>("List", "list", classScope);

//     ParamList parameters;
//     auto param = ParamNode("item", NodeValueType::Any);
//     param.setIsVarArgsParam(true);
//     parameters.addParameter(param);
//     cls.setParameters(parameters);


//     auto construct = [](NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
//         (void)args; (void)callScope; (void)self;
//         auto listImpl = std::make_shared<ListNode>();
//         // callScope->declareVariable("vector", ListNode)
//         return Node();
//     };

//     auto constructMethod = makeShared<NativeMethod>("construct", parameters.clone(), classScope, construct);

//     listClass->addMethod("construct", constructMethod);

//     auto appendFunction = [](NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
//         (void)args; (void)callScope; (void)self;
//         if (!self){
//             throw MerkError("Cannot run 'append' method without instance");
//         }
//         auto instanceScope = self->getInstanceScope();
//         auto vector = instanceScope->getVariable("vector");
        
//         return Node();  // None
//     };

//     // NativeMethod("append", parameters, classScope, appendFunction);
//     auto appendMethod = makeShared<NativeMethod>("append", parameters, classScope, appendFunction);


    

//     listClass->addMethod("append", appendMethod);

//     return listClass;
// }