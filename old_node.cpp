// FunctionCall (Name = print Scope Level 3 ): 
//     Literal (value = LitNode(Value: printing, Type: String, isConst: false, isMutable: true, isStatic: false, isCallable: false) Scope Level 3 ) 
//   ChainOperation)
//     Chain: 
//       Name: sq TokenType: ChainEntryPoint 
//         Accessor(sqScope Level 2 ) 
//       Name: get_x TokenType: ClassMethodCall 
//         Unknown (Name = get_x Scope Level 3 ): 
//             VariableReference (variable = otherValue ,  Scope Level 3 ) 


// FunctionCall (Name = print Scope Level 3 ): 
//                         Literal (value = LitNode(Value: printing, Type: String, isConst: false, isMutable: true, isStatic: false, isCallable: false) Scope Level 3 ) 
//             ChainOperation)
//               Chain: 
//                 Name: sq TokenType: ChainEntryPoint 
//                   VariableReference (variable = sq ,  Scope Level 3 ) 
//                 Name: get_x TokenType: ClassMethodCall 
//                   ClassMethodCall (Name = get_x Scope Level 3 ): 
//                                 VariableReference (variable = otherValue ,  Scope Level 3 ) 