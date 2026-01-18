// #include "core/callables/helpers.hpp"
// #include "ast/AstCallable.hpp"
// #include "core/errors.h"

// CallableSignature* resolveOverload(
//     const Vector<SharedPtr<CallableSignature>>& overloads,
//     const ArgumentList& callArgs,
//     TypeRegistry& typeReg,
//     CallableType callableType,
//     BoundArgs& outBound
// ) {
//     CallableSignature* best = nullptr;
//     int bestScore = -1;

//     // Optional: keep best bound args
//     BoundArgs bestBound;

//     for (const auto& sigPtr : overloads) {
//         if (!sigPtr) continue;
//         auto* sig = sigPtr.get();
//         if (sig->getCallableType() == callableType) {
//             // You need access to the ParamList for this signature.
//             // Assuming you have something like sig->params or sig->callable->getParams()
//             const ParamList& params = sig->getParameters(); // <-- adjust to your actual layout

//             BoundArgs bound = callArgs.bindToBound(params, /*allowDefaults=*/true);

//             auto r = typeReg.matchCall(sig->getTypeSignature(), callArgs);
//             if (!r.ok) continue;

//             if (r.score > bestScore) {
//                 best = sig;
//                 bestScore = r.score;
//                 bestBound = std::move(bound);
//             }
//         }
        
//     }

//     if (!best) {
//         throw MerkError("No matching overload found for call: " + callArgs.toString());
//     }

//     outBound = std::move(bestBound);
//     return best;
// }
