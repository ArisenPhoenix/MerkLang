# Merk README

## Introduction
Merk is a programming language being developed with C++ as its backend. It is designed to serve as a testbed for integrating and mapping SAL (Super Assembly Language) with a high-level language. The current iteration of Merk is solely for testing SAL as a universal backend. Once SAL is fully developed, Merk will be re-implemented to ensure seamless and optimized integration.

## Core Components

### Tokenizer
The tokenizer processes Merk code into structured tokens containing:

- Type information
- Value
- Line and column location

The parser then processes these tokens into an Abstract Syntax Tree (AST) for further evaluation.

### Parser
The parser constructs an AST from tokens and assigns scopes to different code blocks. Scope management is centralized within the parser, ensuring:

- Child scopes are automatically created within the parser.
- Scope levels are tracked numerically (e.g., global scope = level 0, nested blocks = level 1, etc.).
- Scope synchronization with blocks, ensuring correct variable/function resolution.

### Scope
Scope acts as an intermediary between:

- Context (for variable storage/retrieval)
- Registry (for function storage/invocation)

It enforces:

- Scoped function and variable retrieval (i.e., accessing only parent scopes recursively).
- Strict error handling for undefined variables or incorrect assignments.

### Context & Registry
- **Context**: Manages variables within a scope using an unordered map.
- **Registry**: Stores functions (both native and user-defined) and executes them when called.

### Variable Management
- **VarNode**: Stores variables, determines if they can be updated, and ensures type safety.
- **LitNode**: Stores literal values.
- **ParamNode**: Inherits from `VarNode` and is used as a placeholder for function parameters, holding type information.

### Function Execution
Merk distinguishes between:

- **User-defined functions** (parsed from Merk code, stored in scopes).
- **Native functions** (predefined C++ implementations).

### Function Storage & Execution Flow
- Function definitions are stored in the scope in which they are declared.
- Function calls pass the current scope for execution.
- Function arguments are stored in `ParamNode`, preventing direct scope modifications.
- Return values propagate back to the calling scope.
- Function execution follows the AST evaluation flow, ensuring consistency with other AST structures.

## AST Structure
- **BaseAST**: The foundation for all AST nodes.
- **ASTStatement**: Handles variable declarations, assignments, and expressions.
- **CodeBlock**: Represents a block of code and manages child AST nodes.
- **FunctionBlock**: Inherits from `CodeBlock` to encapsulate function logic.
- **FunctionCall**: Represents function invocation.
- **FunctionDef**: Represents function definitions.
- **Evaluator**: A namespace dedicated to AST node evaluation.

### Numerical Scoping
- **Global Scope (Level 0)**
- **Child Scopes (Level 1, Level 2, etc.)** based on nesting depth
- **Traversal is recursive** (child scopes access parent scopes, but not vice versa)

#### Example:
```
Global Scope (Level 0)
│
├── Child Scope (Level 1)   [Variables, Functions]
│   ├── Variable: x
│   ├── Function: myFunction()
│
├── Child Scope (Level 1)   [Classes]
│   ├── Class: MyClass
│   │   ├── Method: myMethod()
```

## Evaluation & Execution
- **Deferred execution** (compilation-like behavior)
- **Immediate execution** (interpretation-like behavior)
- Each AST node has an `evaluate()` method that executes its logic

## Current Development Priorities
- **Functions:** Implementing function storage, execution, and handling parameters.
- **Scope refinement:** Ensuring optimal resolution of variables and functions.
- **Performance improvements:** Optimizing AST traversal and evaluation.
- **Better error handling:** Improving debugging and error reporting within the parser and evaluator.

## Future Considerations
- **Function compilation & optimization**
- **Extending function nodes for native function support**
- **Integrating Return handling for function calls**
- **Advanced class system & method resolution**
- **Potential immutability optimizations**

## SAL Overview
SAL (Super Assembly Language) aims to abstract hardware-specific details while retaining high performance. It serves as a unified intermediary language for high-level languages.

### Key Features:
- Cross-architecture mapping
- Ontology-based abstraction
- BNF-defined functional grammar
- Optimized execution paths for hardware



<h2>Typing Rules and Conversions in Merk</h2>

<h3>Static vs Dynamic Typing</h3>
<p>Merk allows both static and dynamic typing. If a variable is declared with a type annotation, it is statically typed and cannot change type. If no type is provided, the variable is dynamically typed and can change type based on usage.</p>

<ul>
  <li><strong>Static Typed Variable:</strong> <code>var x: int = 12</code> (Fixed as an integer)</li>
  <li><strong>Dynamic Typed Variable:</strong> <code>var x = 12</code> (Type inferred and can change within rules)</li>
</ul>

<h3>Mutability, Constness, and Locked Variables</h3>
<p>Merk provides three key attributes that influence variable behavior:</p>
<ul>
  <li><strong>Mutability</strong>: Determines whether the value of the variable can change.</li>
  <li><strong>Constness</strong>: Determines whether the variable itself can be reassigned.</li>
  <li><strong>Locked Variables</strong>: Declared using <code>const</code> and <code>:=</code>, meaning they are immutable in value, cannot be reassigned, and are statically typed.</li>
</ul>

<p>Example:</p>
<pre><code>const x: int := 12  // Fully locked: immutable, non-reassignable, statically typed</code></pre>

<h3>Type Promotion for Dynamic Variables</h3>
<p>For dynamically typed variables, Merk ensures the type is promoted only when necessary. The system follows these rules:</p>

<ul>
  <li><strong>Integer + Integer → Integer</strong></li>
  <li><strong>Integer + Float → Float</strong> (instead of Double)</li>
  <li><strong>Float + Float → Float</strong> (no unnecessary promotion)</li>
  <li><strong>Integer + Double → Double</strong></li>
  <li><strong>Float + Double → Double</strong></li>
  <li><strong>Long + Integer → Long</strong></li>
  <li><strong>Long + Float → Float</strong> (instead of Double)</li>
  <li><strong>Long + Double → Double</strong></li>
</ul>

<h3>Static Type Enforcement</h3>
<p>For statically typed variables, the type is locked at declaration and cannot be changed. Implicit type conversions do not apply.</p>

<pre><code>
var a: int = 10
var b: float = 5.5

// a = b   // ❌ Error: Cannot assign float to int
</code></pre>

<h3>Opting Out of Type Promotions</h3>
<p>If a developer wants strict type enforcement for dynamic variables, an explicit cast is required before the operation.</p>

<pre><code>
var x = 10  // Initially an int
x = x + 2.5  // Becomes a float due to promotion

var y = 10
x = float(y) + 2.5  // Explicit cast ensures behavior
</code></pre>

<p>These rules ensure Merk remains fast, efficient, and predictable while allowing developers the flexibility they need.</p>

### 🔧 Variable Type Combinations in Merk

Merk uses three key boolean flags to determine variable behavior:

- `isConst`: Whether the variable name can be reassigned.
- `isMutable`: Whether the internal data can be changed.
- `isStatic`: Whether the type is fixed after assignment.

These combinations allow developers to precisely control **immutability**, **type stability**, and **optimization potential**.

| `isConst` | `isMutable` | `isStatic` | Description                                                                 | Tags         |
|-----------|-------------|------------|-----------------------------------------------------------------------------|--------------|
| `false`   | `false`     | `false`    | Reassignable, data locked, type flexible.                                   | ⚠️🧩          |
| `false`   | `false`     | `true`     | Reassignable, data locked, type fixed.                                      | 🚀           |
| `false`   | `true`      | `false`    | Reassignable, data mutable, type flexible (default `var`).                  | ⚠️🧩          |
| `false`   | `true`      | `true`     | Reassignable, data mutable, type fixed — stable & performant.              | 🚀🧩          |
| `true`    | `false`     | `false`    | Not reassignable, data locked, type flexible — weak `const`.               | 🔒⚠️          |
| `true`    | `false`     | `true`     | Fully immutable constant (`const x := val`) — ideal for optimization.      | 🔒🚀✅         |
| `true`    | `true`      | `false`    | Not reassignable, but mutable internals — dynamic objects with permanence. | 🧩⚠️          |
| `true`    | `true`      | `true`     | Not reassignable, mutable internals, type fixed — ideal for classes/maps.  | 🔒🚀          |

---

**Legend**:

- 🔒 = Immutable or constant behavior  
- 🧩 = Flexible and dynamic  
- 🚀 = Optimizable and type-safe  
- ⚠️ = Requires caution (less strict behavior)  
- ✅ = Ideal for performance-critical use cases  


---

Merk is being developed as a proof of concept for SAL, ensuring that a high-level language can seamlessly map to its functional grammar while maintaining efficiency.

