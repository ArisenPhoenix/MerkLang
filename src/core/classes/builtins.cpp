#include "core/types.h"
#include "core/scope.h"
#include "core/classes/bultins.h"
#include "core/classes/native_method.h"
#include "core/classes/native_class.h"


// SharedPtr<NativeClass> createNativeListClass([[maybe_unused]] SharedPtr<Scope> classScope) {
//     auto listClass = makeShared<NativeClass>("List", "list", classScope);

//     ParamList parameters;
//     parameters.addParameter(ParamNode("item", NodeValueType::Any));

//     auto append = [](NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {
//         // auto list = self->getField("vector");  
//         // list.push_back(args[0]);
//         return Node();  // None
//     };


//     auto method = NativeMethod("append", parameters, classScope, append);


//     auto construct = [](NodeList args, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> self) -> Node {


//         return Node();
//     };

//     // listClass->addMethod(makeShared<NativeMethod>(
//     //     "append", parameters,
//     //     [](Vector<Node> args, SharedPtr<ClassInstance> self, SharedPtr<Scope>) -> Node {
//     //         auto list = self->getField("vector").toVector(); 
//     //         list.push_back(args[0]);
//     //         return Node();  // None
//     //     }
//     // ));
    
//     listClass->setConstructor([](Vector<Node> arguments, SharedPtr<Scope> callScope, SharedPtr<ClassInstanceNode> instance) {
//         instance->getInstance()->placeArgsInCallScope(arguments, callScope);
//                 // instance->setAttribute("internal", Node(elements)); 
//     });

//     return listClass;
// }

