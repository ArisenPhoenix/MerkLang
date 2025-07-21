#include "core/types.h"
#include "core/scope.h"
#include "core/callables/classes/bultins.h"
#include "core/callables/classes/class_base.h"
#include "core/callables/classes/native_method.h"
#include "core/callables/classes/native_class.h"
#include "core/callables/classes/node_structures.h"

// SharedPtr<ListNode> pullList(SharedPtr<ClassInstanceNode> self, String varName) {
//     auto instanceScope = self->getInstanceScope();
//     auto vector = instanceScope->getVariable(varName);
//     return std::get<SharedPtr<ListNode>>(vector.getValue());
// };

void validateSelf(SharedPtr<ClassInstanceNode> self, String className, String methodName) {
    if (!self) {throw MerkError("instance was not provided to [" + className + "] -> " + methodName);}
}
 
SharedPtr<DataStructure> pullNativeData(SharedPtr<ClassInstanceNode> self, String forWhat) {
    auto data = self->getInstance()->getNativeData();
    if (!data) {throw MerkError(forWhat + "::NativeData Failed as it is null");}
    return data;
}

SharedPtr<ListNode> pullList(SharedPtr<ClassInstanceNode> self) {
    auto list = std::static_pointer_cast<ListNode>(pullNativeData(self, "ListNode"));
    return list;
}


SharedPtr<ArrayNode> pullArray(SharedPtr<ClassInstanceNode> self) {
    auto arr = std::static_pointer_cast<ArrayNode>(pullNativeData(self, "ArrayNode"));
    return arr;
}

SharedPtr<NativeClass> createNativeListClass(SharedPtr<Scope> globalScope) {
    SharedPtr<Scope> classDefCapturedScope = globalScope->detachScope({});
    if (!classDefCapturedScope) { throw MerkError("classDefCapturedScope Was Not Made"); }
    auto classScope = classDefCapturedScope->makeCallScope();
    if (!classScope) {throw MerkError("Class Scope Was Not Made");}

    ParamList parameters;
    auto param = ParamNode("items", NodeValueType::Any);
    param.setIsVarArgsParam(true);
    parameters.addParameter(param);
    
    classDefCapturedScope->appendChildScope(classScope);
    globalScope->appendChildScope(classDefCapturedScope);
    

    String className = "List";
    String accessor = "list";

    auto listClass = makeShared<NativeClass>(className, accessor, classScope);
    if (listClass->getSubType() != CallableType::NATIVE) {throw MerkError("NativeClass is not properly subtyped: is " + callableTypeAsString(listClass->getSubType()));}

    listClass->setParameters(parameters);
    listClass->setCapturedScope(classDefCapturedScope);

    String varName = ListNode::getOriginalVarName();
    
    DEBUG_LOG(LogLevel::PERMISSIVE, "Got the List Class initialized");

    auto constructFunction = [className, varName](NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(self, callScope);
        validateSelf(self, className, "constructFunction");
        auto list = makeShared<ListNode>(args);
        auto var = VarNode(list);
        
        var.name = className;

        if (!var.isList()) {throw MerkError("Var created is not a list");}

        // A declarVariable is needed to properly assign a variable to the ListNode
        // auto instanceScope = self->getInstanceScope();
        self->getInstance()->setNativeData(list);
        return var;
    };

    auto appendFunction = [varName](NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(self, callScope);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (args.size() > 1) {throw MerkError("Only one argument accepted in the list::append method");}
        
        // auto vector = pullList(self, varName);
        auto vector = pullList(self);
        vector->append(args[0]);
        DEBUG_FLOW_EXIT();
        return Node();  // None
    };

    auto removeParams = ParamList();
    auto rmParam = ParamNode("value", NodeValueType::Any);
    removeParams.addParameter(rmParam);
    auto removeFunction = [varName](NodeList arg, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(callScope);
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (arg.size() != 1) {throw MerkError("Only One Argument is allowed in List.remove, provided: " + joinVectorNodeStrings(arg));}
        auto instanceScope = self->getInstanceScope();

        auto vector = pullList(self);
        vector->remove(arg[0]);
        return Node();  // None
    };

    auto insertParams = ParamList();
    auto insParam1 = ParamNode("index", NodeValueType::Int);
    auto insParam2 = ParamNode("value", NodeValueType::Any);
    insertParams.addParameter(insParam1);
    insertParams.addParameter(insParam2);
    auto insertFunction = [varName](NodeList arg, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(callScope);
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (arg.size() != 2) {throw MerkError("Only Two Arguments are allowed in List.insert, provided: " + joinVectorNodeStrings(arg));}

        auto instanceScope = self->getInstanceScope();
        auto vector = pullList(self);
        vector->insert(arg[0], arg[1]);
        return Node();  // None
    };


    auto popParams = ParamList();
    auto popParam1 = ParamNode("index", NodeValueType::Int);
    popParams.addParameter(popParam1);
    auto popFunction = [varName](NodeList arg, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(callScope);
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (arg.size() != 1) {throw MerkError("Only Two Arguments are allowed in List.insert, provided: " + joinVectorNodeStrings(arg));}
        auto instanceScope = self->getInstanceScope();
        auto vector = pullList(self);
        return vector->pop(arg[0]);
    };




    auto constructMethod = makeShared<NativeMethod>("construct", parameters.clone(), classScope, constructFunction);
    listClass->addMethod("construct", constructMethod);

    auto appendMethod = makeShared<NativeMethod>("append", parameters, classScope, appendFunction);
    listClass->addMethod("append", appendMethod);

    auto removeMethod = makeShared<NativeMethod>("remove", removeParams, classScope, removeFunction);
    listClass->addMethod("remove", removeMethod);

    auto insertMethod = makeShared<NativeMethod>("insert", insertParams, classScope, insertFunction);
    listClass->addMethod("insert", insertMethod);

    auto popMethod = makeShared<NativeMethod>("pop", popParams, classScope, popFunction);
    listClass->addMethod("pop", popMethod);


    return listClass;
}


SharedPtr<NativeClass> createNativeArrayClass(SharedPtr<Scope> globalScope) {
    String className = "Array";
    String accessor = "array";
    String varName = ArrayNode::getOriginalVarName();

    if (!globalScope) {throw MerkError("Class Scope Not Provided to Native Class List");}
    
    SharedPtr<Scope> classDefCapturedScope = globalScope->detachScope({});
    if (!classDefCapturedScope) { throw MerkError("classDefCapturedScope Was Not Made");}
    auto classScope = classDefCapturedScope->makeCallScope();
    if (!classScope) {throw MerkError("Class Scope Was Not Made");}

    auto arrClass = makeShared<NativeClass>(className, accessor, classScope);

    ParamList parameters;
    auto param = ParamNode("items", NodeValueType::Int);
    param.setIsVarArgsParam(true);
    parameters.addParameter(param);

    arrClass->setParameters(parameters);
    arrClass->setCapturedScope(classDefCapturedScope);
    classDefCapturedScope->appendChildScope(classScope);
    globalScope->appendChildScope(classDefCapturedScope);

    auto constructFunction = [className](NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        (void)args; (void)callScope; (void)self;
        validateSelf(self, className, "constructFunction");
        auto arr = makeShared<ListNode>(args);
        auto var = VarNode(arr);
        var.name = className;

        if (!var.isList()) {throw MerkError("Var created is not a list");}        
        self->getInstance()->setNativeData(arr);
        return var;
    };

    auto appendFunction = [className](NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        (void)callScope;
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (args.size() != 1) {throw MerkError("Method append can only accept 1 argument");}
        validateSelf(self, className, "append");
        auto vector = pullArray(self);
        vector->append(args[0]);

        return Node();  // None
    };

    auto removeParams = ParamList();
    auto rmParam = ParamNode("value", NodeValueType::Any);
    removeParams.addParameter(rmParam);
    auto removeFunction = [](NodeList arg, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        (void)callScope;
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (arg.size() != 1) {
            throw MerkError("Only One Argument is allowed in List.remove, provided: " + joinVectorNodeStrings(arg));
        }
        // auto instanceScope = self->getInstanceScope();
        // auto vector = instanceScope->getVariable("vector");
        // auto data = std::get<SharedPtr<ArrayNode>>(vector.getValue());

        auto vector = pullArray(self);
        vector->remove(arg[0]);
        return Node();  // None
    };

    auto insertParams = ParamList();
    auto insParam1 = ParamNode("index", NodeValueType::Int);
    auto insParam2 = ParamNode("value", NodeValueType::Any);
    insertParams.addParameter(insParam1);
    insertParams.addParameter(insParam2);
    auto insertFunction = [](NodeList arg, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        (void)callScope;
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (arg.size() != 2) {
            throw MerkError("Only Two Arguments are allowed in List.insert, provided: " + joinVectorNodeStrings(arg));
        }
        // auto instanceScope = self->getInstanceScope();
        // auto vector = instanceScope->getVariable("vector");
        // auto data = std::get<SharedPtr<ArrayNode>>(vector.getValue());

        auto vector = pullArray(self);
        vector->insert(arg[0], arg[1]);
        return Node();  // None
    };


    auto popParams = ParamList();
    auto popParam1 = ParamNode("index", NodeValueType::Int);
    popParams.addParameter(popParam1);
    auto popFunction = [](NodeList arg, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        (void)callScope;
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (arg.size() != 1) {
            throw MerkError("Only Two Arguments are allowed in List.insert, provided: " + joinVectorNodeStrings(arg));
        }
        // auto instanceScope = self->getInstanceScope();
        // auto vector = instanceScope->getVariable("vector");
        // auto data = std::get<SharedPtr<ArrayNode>>(vector.getValue());

        auto vector = pullArray(self);
        return vector->pop(arg[0]);
        // DEBUG_LOG(LogLevel::PERMISSIVE, "Append varNode is: ", vector);
        // DEBUG_LOG(LogLevel::PERMISSIVE, "type held: ", vector.getTypeAsString());
        // return Node();  // None
    };




    auto constructMethod = makeShared<NativeMethod>("construct", parameters.clone(), classScope, constructFunction);
    arrClass->addMethod("construct", constructMethod);

    auto appendMethod = makeShared<NativeMethod>("append", parameters, classScope, appendFunction);
    arrClass->addMethod("append", appendMethod);

    auto removeMethod = makeShared<NativeMethod>("remove", removeParams, classScope, removeFunction);
    arrClass->addMethod("remove", removeMethod);

    auto insertMethod = makeShared<NativeMethod>("insert", insertParams, classScope, insertFunction);
    arrClass->addMethod("insert", insertMethod);

    auto popMethod = makeShared<NativeMethod>("pop", popParams, classScope, popFunction);
    arrClass->addMethod("pop", popMethod);


    return arrClass;
}



std::unordered_map<String, NativeClassFactory> nativeClassFactories = {
    {"List", createNativeListClass},
    {"Array", createNativeArrayClass}
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