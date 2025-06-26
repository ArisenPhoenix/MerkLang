// #include "core/types.h"
// #include "core/scope.h"
// #include "core/callables/classes/bultins.h"
// #include "core/callables/classes/class_base.h"
// #include "core/callables/classes/native_method.h"
// #include "core/callables/classes/native_class.h"
// #include "core/callables/classes/node_structures.h"


// SharedPtr<NativeClass> createNativeListClass(SharedPtr<Scope> classScope) {
//     if (!classScope) {
//         throw MerkError("Class Scope Not Provided to Native Class List");
//     }
//     auto cls = NativeClass("List", "list", classScope);
//     auto listClass = makeShared<NativeClass>("List", "list", classScope);

//     ParamList parameters;
//     auto param = ParamNode("item", NodeValueType::Any);
//     param.setIsVarArgsParam(true);
//     parameters.addParameter(param);
//     cls.setParameters(parameters);


//     auto construct = [](NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
//         (void)args; (void)callScope; (void)self;
//         // ListNode(args);
//         auto listImpl = std::make_shared<ListNode>(args);
//         // auto var = makeUnique<VarNode>(listImpl);
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






// std::unordered_map<String, NativeClassFactory> nativeClassFactories = {
//     {"List", createNativeListClass},
// };


// std::unordered_map<String, SharedPtr<ClassBase>> getAllNativeClasses(SharedPtr<Scope> scope) {
//     std::unordered_map<String, SharedPtr<ClassBase>> builtins;
//     for (auto& [funcName, classSigCall] : nativeClassFactories) {
//         auto nativeCls = classSigCall(scope); // safe
//         builtins[funcName] = nativeCls;
//     }
//     return builtins;
// }





// // SharedPtr<NativeClass> createNativeArrayClass([[maybe_unused]] SharedPtr<Scope> classScope) {
// //     auto cls = NativeClass("List", "list", classScope);
// //     auto listClass = makeShared<NativeClass>("List", "list", classScope);

// //     ParamList parameters;
// //     auto param = ParamNode("item", NodeValueType::Any);
// //     param.setIsVarArgsParam(true);
// //     parameters.addParameter(param);
// //     cls.setParameters(parameters);


// //     auto construct = [](NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
// //         (void)args; (void)callScope; (void)self;
// //         auto listImpl = std::make_shared<ListNode>();
// //         // callScope->declareVariable("vector", ListNode)
// //         return Node();
// //     };

// //     auto constructMethod = makeShared<NativeMethod>("construct", parameters.clone(), classScope, construct);

// //     listClass->addMethod("construct", constructMethod);

// //     auto appendFunction = [](NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
// //         (void)args; (void)callScope; (void)self;
// //         if (!self){
// //             throw MerkError("Cannot run 'append' method without instance");
// //         }
// //         auto instanceScope = self->getInstanceScope();
// //         auto vector = instanceScope->getVariable("vector");
        
// //         return Node();  // None
// //     };

// //     // NativeMethod("append", parameters, classScope, appendFunction);
// //     auto appendMethod = makeShared<NativeMethod>("append", parameters, classScope, appendFunction);


    

// //     listClass->addMethod("append", appendMethod);

// //     return listClass;
// // }