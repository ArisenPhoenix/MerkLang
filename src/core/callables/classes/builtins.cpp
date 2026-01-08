#include "core/node/ArgumentNode.hpp"

#include "core/types.h"
#include "core/Scope.hpp"
#include "core/callables/classes/bultins.h"
#include "core/callables/classes/ClassBase.hpp"
#include "core/callables/classes/NativeMethod.hpp"
#include "core/callables/classes/NativeClass.hpp"
#include "core/node/NodeStructures.hpp"
#include "core/evaluator.h"


void validateSelf(SharedPtr<ClassInstanceNode> self, String className, String methodName) {
    if (!self) {throw MerkError("instance was not provided to [" + className + "] -> " + methodName);}
}
 
SharedPtr<NativeNode> pullNativeData(SharedPtr<ClassInstanceNode> self, String forWhat) {
    self->getValue();
    auto data = self->getInstance()->getNativeData();
    if (!data) {throw MerkError(forWhat + "::NativeData Failed as it is null");}
    return data;
}

SharedPtr<ListNode> pullList(SharedPtr<ClassInstanceNode> self) {
    auto list = std::static_pointer_cast<ListNode>(pullNativeData(self, "ListNode"));
    return list;
}


SharedPtr<DictNode> pullDict(SharedPtr<ClassInstanceNode> self) {
    auto dict = std::static_pointer_cast<DictNode>(pullNativeData(self, "DictNode"));
    return dict;
}


SharedPtr<ArrayNode> pullArray(SharedPtr<ClassInstanceNode> self) {
    auto arr = std::static_pointer_cast<ArrayNode>(pullNativeData(self, "ArrayNode"));
    return arr;
}


SharedPtr<HttpNode> pullHttp(SharedPtr<ClassInstanceNode> self) {
    auto http = std::static_pointer_cast<HttpNode>(pullNativeData(self, "ArrayNode"));
    return http;
}

SharedPtr<FileNode> pullFile(SharedPtr<ClassInstanceNode> self) {
    auto file = std::static_pointer_cast<FileNode>(pullNativeData(self, "ArrayNode"));
    return file;
}

template <typename T>
SharedPtr<T> pullNative(SharedPtr<ClassInstanceNode> self,
                        std::string_view forWhat = typeid(T).name()) {

    if (!self) {
        throw MerkError(String(forWhat) + "::pullNative received null instance");
    }

    self->getValue();

    auto data = self->getInstance()->getNativeData();
    if (!data) {
        throw MerkError(String(forWhat) + "::NativeData is null");
    }

    return std::static_pointer_cast<T>(data);
}

template <typename T>
T& pullNativeRef(SharedPtr<ClassInstanceNode> self,
                 std::string_view forWhat = typeid(T).name()) {
    return *pullNative<T>(self, forWhat);
}





SharedPtr<Scope> generateScope(SharedPtr<Scope> globalScope) {
    SharedPtr<Scope> classDefCapturedScope = globalScope->detachScope({});
    if (!classDefCapturedScope) { throw MerkError("classDefCapturedScope Was Not Made"); }
    auto classScope = classDefCapturedScope->makeCallScope();
    if (!classScope) {throw MerkError("Class Scope Was Not Made");}

    classDefCapturedScope->appendChildScope(classScope);
    globalScope->appendChildScope(classDefCapturedScope);
    return classScope;   
}


SharedPtr<DictNode> pullHttpDict(String varName, SharedPtr<ClassInstanceNode> self) {
    auto headersNode = self->getInstance()->getField(varName);
    if (std::holds_alternative<SharedPtr<Callable>>(headersNode.getValue())) {
        auto inst = std::static_pointer_cast<ClassInstance>(std::get<SharedPtr<Callable>>(headersNode.getValue()));
        auto nativeNode = inst->getNativeData();
        auto dict = std::static_pointer_cast<DictNode>(nativeNode);
        return dict;
        
    } else {
        throw MerkError("headers node doesn't hold NativeNode, but a " + nodeTypeToString(DynamicNode::getTypeFromValue(headersNode.getValue())));
    }
}

SharedPtr<NativeClass> createNativeListClass(SharedPtr<Scope> globalScope) {
    SharedPtr<Scope> classScope = generateScope(globalScope);

    ParamList parameters;
    auto param = ParamNode("items", NodeValueType::Any);
    param.setIsVarArgsParam(true);
    parameters.addParameter(param);
    

    String className = "List";
    String accessor = "list";

    auto listClass = makeShared<NativeClass>(className, accessor, classScope);
    if (listClass->getSubType() != CallableType::NATIVE) {throw MerkError("NativeClass is not properly subtyped: is " + callableTypeAsString(listClass->getSubType()));}

    listClass->setParameters(parameters);
    listClass->setCapturedScope(classScope->getParent());

    String varName = ListNode::getOriginalVarName();
    

    auto constructFunction = [className, varName](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(self, callScope);
        validateSelf(self, className, "constructFunction");
        auto list = makeShared<ListNode>(args);
        if (!list) {throw MerkError("List Doesn't Exist");}
        
        self->getInstance()->setNativeData(list);
        return Node();
    };

    auto appendFunction = [varName](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(self, callScope);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (args.size() > 1) {throw MerkError("Only one argument accepted in the list::append method");}
        
        auto vector = pullList(self);
        vector->append(args[0]);
        DEBUG_FLOW_EXIT();
        return Node();  // None
    };

    auto removeParams = ParamList();
    auto rmParam = ParamNode("value", NodeValueType::Any);
    removeParams.addParameter(rmParam);
    auto removeFunction = [varName](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(callScope);
        if (!self) {throw MerkError("Cannot run 'remove' method without instance");}
        if (args.size() != 1) {throw MerkError("Only One Argument is allowed in List.remove, provided: " + joinVectorNodeStrings(args.getPositional()));}
        auto instanceScope = self->getInstanceScope();

        auto vector = pullList(self);
        vector->remove(args[0]);
        return Node();  // None
    };

    auto insertParams = ParamList();
    auto insParam1 = ParamNode("index", NodeValueType::Int);
    auto insParam2 = ParamNode("value", NodeValueType::Any);
    insertParams.addParameter(insParam1);
    insertParams.addParameter(insParam2);
    auto insertFunction = [varName](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(callScope);
        if (!self) {throw MerkError("Cannot run 'insert' method without instance");}
        if (args.size() != 2) {throw MerkError("Only Two Arguments are allowed in List.insert, provided: " + joinVectorNodeStrings(args.getPositional()));}

        auto instanceScope = self->getInstanceScope();
        auto vector = pullList(self);
        vector->insert(args[0], args[1]);
        return Node();  // None
    };


    auto popParams = ParamList();
    auto popParam1 = ParamNode("index", NodeValueType::Int);
    popParam1.setIsVarArgsParam(true);
    popParams.addParameter(popParam1);
    auto popFunction = [varName](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(callScope);
        if (!self) {throw MerkError("Cannot run 'pop' method without instance");}
        if (args.size() > 1) {throw MerkError("Only Two Arguments are allowed in List.insert, provided: " + joinVectorNodeStrings(args.getPositional()));}
        auto instanceScope = self->getInstanceScope();
        auto vector = pullList(self);
        auto arg = args.size() == 1 ? args[0] : Node();
        Node result = vector->pop(arg);
        return result;
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

SharedPtr<NativeClass> createNativeDictClass(SharedPtr<Scope> globalScope) {
    SharedPtr<Scope> classScope = generateScope(globalScope);

    ParamList parameters;
    auto param = ParamNode("items", NodeValueType::Any);
    param.setIsVarArgsParam(true);
    parameters.addParameter(param);
    String className = "Dict";
    String accessor = "dict";

    auto dictClass = makeShared<NativeClass>(className, accessor, classScope);
    if (dictClass->getSubType() != CallableType::NATIVE) {throw MerkError("NativeClass is not properly subtyped: is " + callableTypeAsString(dictClass->getSubType()));}

    dictClass->setParameters(parameters);
    dictClass->setCapturedScope(classScope->getParent());

    String varName = DictNode::getOriginalVarName();
    
    // DEBUG_LOG(LogLevel::PERMISSIVE, "Got the Dict Class initialized");

    auto constructFunction = [className, varName](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(self, callScope);
        validateSelf(self, className, "constructFunction");
        // DictNode(args);
        auto dict = makeShared<DictNode>(args);
        // auto something = DictNode(args);
        if (!dict) {throw MerkError("Dict Doesn't Exist");}
        // self->getInstance()->setNativeData(dict);
        self->getInstance()->setNativeData(dict);
        return Node();
    };

    auto setParams = ParamList();
    auto setParam = ParamNode("key", NodeValueType::Any);
    auto setParam2 = ParamNode("value", NodeValueType::Any);
    setParams.addParameter(setParam);
    setParams.addParameter(setParam2);

    auto setFunction = [varName](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(self, callScope);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (args.size() != 2) {throw MerkError("Only two arguments accepted in the dict::set method");}
        
        // auto vector = pullList(self, varName);
        auto dict = pullDict(self);
        dict->set(args[0], args[1]);
        DEBUG_FLOW_EXIT();
        return Node();  // None
    };

    auto getParams = ParamList();
    auto getParam = ParamNode("key", NodeValueType::Any);
    auto getParam2 = ParamNode("defaultReturn", NodeValueType::Any);
    getParam2.setIsVarArgsParam(true);
    getParams.addParameter(getParam);
    getParams.addParameter(getParam2);


    auto getFunction = [varName](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(callScope);
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (args.size() != 1 && args.size() != 2) {throw MerkError("Only Two Arguments are allowed in Dict.get, provided: " + args.toString());}
        // throw MerkError("getFunction");
        DEBUG_LOG(LogLevel::PERMISSIVE, "ARGS: ", args.toString());
        // throw MerkError("ERROR");
        auto dict = pullDict(self);
        Node result = dict->get(args[0], args.size() == 2 ? args[1] : Node(Null));
        return result;  // None
    };

    auto popParams = ParamList();
    auto popParam1 = ParamNode("key", NodeValueType::Any);
    popParams.addParameter(popParam1);
    
    auto popFunction = [varName](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(callScope);
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (args.size() > 1) {throw MerkError("Only Two Arguments are allowed in Dict.pop, provided: " + joinVectorNodeStrings(args.getPositional()));}
        auto dict = pullDict(self);
        Node result = dict->pop(args[0]);
        return result;
    };

    auto removeParams = ParamList();
    auto insParam1 = ParamNode("key", NodeValueType::Int);
    removeParams.addParameter(insParam1);

    auto removeFunction = [varName](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(callScope);
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (args.size() != 2) {throw MerkError("Only Two Arguments are allowed in Dict.remove, provided: " + joinVectorNodeStrings(args.getPositional()));}

        auto instanceScope = self->getInstanceScope();
        auto dict = pullDict(self);
        dict->remove(args[0]);
        return Node();  // None
    };


    




    auto constructMethod = makeShared<NativeMethod>("construct", parameters.clone(), classScope, constructFunction);
    dictClass->addMethod("construct", constructMethod);

    auto setMethod = makeShared<NativeMethod>("set", parameters, classScope, setFunction);
    dictClass->addMethod("set", setMethod);

    auto getMethod = makeShared<NativeMethod>("get", getParams, classScope, getFunction);
    dictClass->addMethod("get", getMethod);

    auto popMethod = makeShared<NativeMethod>("pop", popParams, classScope, popFunction);
    dictClass->addMethod("pop", popMethod);

    auto removeMethod = makeShared<NativeMethod>("remove", removeParams, classScope, removeFunction);
    dictClass->addMethod("remove", removeMethod);

    return dictClass;
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

    auto constructFunction = [className](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        (void)args; (void)callScope; (void)self;
        validateSelf(self, className, "constructFunction");
        auto arr = makeShared<ListNode>(args);
        // auto var = VarNode(arr);
        auto var = Node(arr);
        var.getFlags().name = className;
        var.getFlags().fullType.setBaseType(className);
        // var.name = className;

        if (!var.isArray()) {throw MerkError("Var created is not an Array");}        
        self->getInstance()->setNativeData(arr);
        return var;
    };

    auto appendFunction = [className](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
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
    auto removeFunction = [](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        (void)callScope;
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (args.size() != 1) {
            throw MerkError("Only One Argument is allowed in List.remove, provided: " + joinVectorNodeStrings(args.getPositional()));
        }

        auto vector = pullArray(self);
        vector->remove(args[0]);
        return Node();  // None
    };

    auto insertParams = ParamList();
    auto insParam1 = ParamNode("index", NodeValueType::Int);
    auto insParam2 = ParamNode("value", NodeValueType::Any);
    insertParams.addParameter(insParam1);
    insertParams.addParameter(insParam2);
    auto insertFunction = [](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        (void)callScope;
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (args.size() != 2) {
            throw MerkError("Only Two Arguments are allowed in List.insert, provided: " + joinVectorNodeStrings(args.getPositional()));
        }

        auto vector = pullArray(self);
        vector->insert(args[0], args[1]);
        return Node();  // None
    };


    auto popParams = ParamList();
    auto popParam1 = ParamNode("index", NodeValueType::Int);
    popParams.addParameter(popParam1);
    auto popFunction = [](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        (void)callScope;
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (args.size() != 1) {
            throw MerkError("Only Two Arguments are allowed in List.insert, provided: " + joinVectorNodeStrings(args.getPositional()));
        }

        auto vector = pullArray(self);
        return vector->pop(args[0]);
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


SharedPtr<NativeClass> createNativeHttpClass(SharedPtr<Scope> globalScope) {
    SharedPtr<Scope> classScope = generateScope(globalScope);

    ParamList parameters;
    auto param = ParamNode("url", NodeValueType::String);
    auto param2 = ParamNode("method", NodeValueType::String, true);

    parameters.addParameter(param);
    parameters.addParameter(param2);
    String className = "Http";
    String accessor = "http";
    
    auto httpClass = makeShared<NativeClass>(className, accessor, classScope);
    if (httpClass->getSubType() != CallableType::NATIVE) {throw MerkError("NativeClass is not properly subtyped: is " + callableTypeAsString(httpClass->getSubType()));}

    httpClass->setParameters(parameters);
    httpClass->setCapturedScope(classScope->getParent());

    String varName = DictNode::getOriginalVarName();
    
    auto constructFunction = [className, varName](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(self, callScope);
        validateSelf(self, className, "constructFunction");
        auto instance = self->getInstance();
        

        auto instanceScope = instance->getInstanceScope();

        auto methodArgs = ArgResultType();
        if (args.size() >= 1) { instance->declareField("url", args[0]); } else {instance->declareField("url", Node("http"));}
        if (args.size() >= 2) {instance->declareField("method", args[1]); } else {instance->declareField("method", Node("GET")); }
        
        auto http = makeShared<HttpNode>(self);
        self->getInstance()->setNativeData(http);

        if (!http) { throw MerkError("Http Doesn't Exist"); }
        return Node();
    };

    auto setHeaderParams = ParamList();
    auto setHeaderParam = ParamNode("key", NodeValueType::String);
    auto setHeaderParam2 = ParamNode("value", NodeValueType::Any);
    setHeaderParams.addParameter(setHeaderParam);
    setHeaderParams.addParameter(setHeaderParam2);

    auto setHeader = [varName](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(self, callScope);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (args.size() != 2) {throw MerkError("Only two arguments accepted in the dict::set method");}
    
        auto headers = pullHttpDict("headers", self);
        headers->set(args[0], args[1]);
        DEBUG_FLOW_EXIT();
        return Node();  // None
    };

    auto getHeaderParams = ParamList();
    auto getHeaderParam = ParamNode("key", NodeValueType::String);
    getHeaderParams.addParameter(getHeaderParam);
    auto getHeader = [varName](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(self, callScope);
        DEBUG_FLOW(FlowLevel::PERMISSIVE);
        if (!self) {throw MerkError("Cannot run 'append' method without instance");}
        if (args.size() > 2) {throw MerkError("Only two arguments accepted in the dict::set method");}
        auto headers = pullHttpDict("headers", self);
        DEBUG_FLOW_EXIT();

        return headers->get(args[0], Node(Null));
    };



    auto sendParams = ParamList(); // no args for now
    auto sendFn = [](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
        MARK_UNUSED_MULTI(args, callScope);
        if (!self) throw MerkError("Http.send requires instance");
        auto native = self->getInstance()->getNativeData(); // however you store it
        auto http = std::static_pointer_cast<HttpNode>(native);
        if (!http) throw MerkError("Http native not attached");
        return http->send();
    };

    

    




    auto constructMethod = makeShared<NativeMethod>("construct", parameters.clone(), classScope, constructFunction);
    httpClass->addMethod("construct", constructMethod);

    auto setMethod = makeShared<NativeMethod>("setHeader", setHeaderParams, classScope, setHeader);
    httpClass->addMethod("setHeader", setMethod);

    auto getMethod = makeShared<NativeMethod>("getHeader", getHeaderParams, classScope, getHeader);
    httpClass->addMethod("getHeader", getMethod);

    auto sendMethod = makeShared<NativeMethod>("send", sendParams, classScope, sendFn);
    httpClass->addMethod("send", sendMethod);

    return httpClass;
}

SharedPtr<NativeClass> createNativeFileClass(SharedPtr<Scope> globalScope) {
    auto classScope = generateScope(globalScope);
    ParamList params;
    params.addParameter(ParamNode("path",   NodeValueType::String));
    params.addParameter(ParamNode("mode",   NodeValueType::String, true)); // default "r"

    auto cls = makeShared<NativeClass>("File", "file", classScope);
    cls->setParameters(params);
    cls->setCapturedScope(classScope->getParent());

    // construct(path, mode="r")
    auto constructFn = [](ArgResultType args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self)->Node {
        MARK_UNUSED_MULTI(callScope);
        auto inst = self->getInstance();
        inst->declareField("path",   args.size() >= 1 ? args[0] : Node(""));
        inst->declareField("mode",   args.size() >= 2 ? args[1] : Node("r"));
        auto f = makeShared<FileNode>(self);
        inst->setNativeData(f);
        return Node();
    };

    auto m_construct = makeShared<NativeMethod>("construct", params.clone(), classScope, constructFn);
    

    // open(), close(), readAll(), read(n), write(s), exists(path), size(path), remove(path)
    auto noArgs = ParamList{};
    auto openFn = [](ArgResultType, SharedPtr<Scope>, SharedPtr<ClassInstanceNode> self)->Node {
        pullNative<FileNode>(self)->open(); return Node();
    };

    

    auto closeFn = [](ArgResultType, SharedPtr<Scope>, SharedPtr<ClassInstanceNode> self)->Node {
        pullNative<FileNode>(self)->close(); return Node();
    };

    auto readAllFn = [](ArgResultType, SharedPtr<Scope>, SharedPtr<ClassInstanceNode> self)->Node {
        return Node(pullNative<FileNode>(self)->readAll());
    };

    ParamList readN;
    auto readN1 = ParamNode("n", NodeValueType::Int, true);
    readN.addParameter(readN1);
    auto readFn = [](ArgResultType a, SharedPtr<Scope>, SharedPtr<ClassInstanceNode> self)->Node {
        size_t arg = 0;
        if (a.size() != 0) {
            arg = a[0].toInt();
        }
        return Node(pullNative<FileNode>(self)->read(arg));
    };

    ParamList writeP; writeP.addParameter(ParamNode("s", NodeValueType::Any));
    auto writeFn = [](ArgResultType a, SharedPtr<Scope>, SharedPtr<ClassInstanceNode> self)->Node {
        auto val = a[0].toString();
        // throw MerkError("Trying to write \n" + val);
        pullNative<FileNode>(self)->write(a[0].toString()); return Node();
    };
    

    // Static helpers: exists, size, remove, readAll(path), writeAll(path, s)
    auto addStatic = [&](const char* name, ParamList ps, auto fn) {
        auto m = makeShared<NativeMethod>(name, ps, classScope, fn);
        m->setIsStatic(true);
        cls->addMethod(name, m);
    };

    ParamList pathP; pathP.addParameter(ParamNode("path", NodeValueType::String));
    addStatic("exists", pathP, [](ArgResultType a, SharedPtr<Scope>, SharedPtr<ClassInstanceNode>){
        return Node(FileNode::exists(a[0].toString()));
    });

    addStatic("size", pathP, [](ArgResultType a, SharedPtr<Scope>, SharedPtr<ClassInstanceNode>){
        return Node(static_cast<int>(FileNode::sizeOf(a[0].toString())));
    });

    addStatic("remove", pathP, [](ArgResultType a, SharedPtr<Scope>, SharedPtr<ClassInstanceNode>){
        FileNode::removeFile(a[0].toString()); return Node();
    });

    ParamList rAllP = pathP;
    addStatic("readAll", rAllP, [](ArgResultType a, SharedPtr<Scope>, SharedPtr<ClassInstanceNode>){
        std::ifstream ifs(a[0].toString(), std::ios::binary);
        if (!ifs) throw MerkError("File.readAll failed");
        std::ostringstream oss; oss << ifs.rdbuf(); return Node(oss.str());
    });

    ParamList wAllP; wAllP.addParameter(ParamNode("path", NodeValueType::String));
    wAllP.addParameter(ParamNode("data", NodeValueType::Any));
    addStatic("writeAll", wAllP, [](ArgResultType a, SharedPtr<Scope>, SharedPtr<ClassInstanceNode>){
        std::ofstream ofs(a[0].toString(), std::ios::binary|std::ios::trunc);
        if (!ofs) throw MerkError("File.writeAll failed");
        auto s = a[1].toString(); ofs.write(s.data(), static_cast<std::streamsize>(s.size())); return Node();
    });



    cls->addMethod("construct", m_construct);
    cls->addMethod("open", makeShared<NativeMethod>("open", noArgs, classScope, openFn));
    cls->addMethod("close", makeShared<NativeMethod>("close", noArgs, classScope, closeFn));
    cls->addMethod("readAll", makeShared<NativeMethod>("readAll", noArgs, classScope, readAllFn));
    cls->addMethod("read", makeShared<NativeMethod>("read", readN, classScope, readFn));
    cls->addMethod("write", makeShared<NativeMethod>("write", writeP, classScope, writeFn));

    return cls;
}




std::unordered_map<String, NativeClassFactory> nativeClassFactories = {
    {"List", createNativeListClass},
    {"Array", createNativeArrayClass},
    {"Dict", createNativeDictClass},
    {"Http", createNativeHttpClass},
    {"File", createNativeFileClass}
};


std::unordered_map<String, SharedPtr<ClassBase>> getAllNativeClasses(SharedPtr<Scope> globalScope) {
    std::unordered_map<String, SharedPtr<ClassBase>> builtins;
    for (auto& [funcName, classSigCall] : nativeClassFactories) {
        if (funcName.empty()) {throw MerkError("FuncName empty");}

        // DEBUG_LOG(LogLevel::PERMISSIVE, "Getting Class: ", funcName);
        
        auto nativeCls = classSigCall(globalScope); // safe
        builtins.emplace(funcName, nativeCls);
    }
    return builtins;
}


