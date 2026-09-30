---
title: "Vectype: Capstone Proposal"
author: Danielle Hutzley
date: "27 Sep 2026"
geometry: margin=0.5in,top=0.05in,bottom=0.25in
---

### Project Description
Vectype is a compiled LISP-family programming language targeting WebAssembly. It is based on linear types with compile-time reference-counted reference types — this allows for safe FFI without a garbage collector or a borrow checker. It supports web components for web native integration with the DOM, outside of any JavaScript framework. The program is the compiler which converts Vectype to WebAssembly text.

### The Umbrella
- The program is the S-expression parser combined with a compiler to WebAssembly text.
- A lot of implementation details will live in `lib/` at the repository root (formally called `cs2-lib`) allowing me to dogfood the reusable components.

##### Shared Data
- A carefully lifetime managed copy of the program text
- An Abstract Syntax Tree that will be read by module loading, name resolution, linear type checking, and codegen (in pipeline order).
- A BST symbol table which is made by the frontend and read by later stages.
- A module graph used to track dependencies and an import table per module.

### Category Mapping
1. The language is built around a generic `LinkedList<T>` class template, and the `Atom` "leaf" node in the main AST is an abstract base class stored within a `std::unique_ptr` (`NumAtom`, `SymbolAtom`, etc are children).
2. The `LinkedList<T>` follows the rule of 5: its destructor will be implemented to be iterative (recursion overflows the stack), copying will be handled with a custom deep copy, and moves will be defaulted. `Atom` derivatives will follow the rule of 0 and have a virtual destructor in `Atom`.
3. The `AST` will be built on `LinkedList<T>`, which is implemented as a cons cell.
4. A `LinkedList<T>` will be used to implement a parse stack bounded by the heap rather than the stack. 
5. `LinkedSet<T>` will use buckets implemented on my `LinkedList` and `std::hash`. It will be used in symbol interning to map each name to one canonical copy.
6. The symbol table will use a BST keyed by symbol name created during name resolution and queried after.
7. A graph will be used to link together modules and detect cycles. It will be directed (allowing us to catch cyclical imports; imports are asymmetric) and unweighted.
8. Quicksort will be used on each module's import table to allow for binary search to be used. This was chosen as it's constructed once read multiple, compared to the local symbol table's multiple insertions during name resolution.

### Scope Assessment
- [ ] By W7: Lexer in `cs2-lib`
- [ ] By W8: Parser in `cs2-lib`, `LinkedList<T>`, `Ast`
- [ ] By W9: Parser translation, `Atom` and children, name resolution, `LinkedSet<T>`, TextMate grammar, early linear type checker
- [ ] By W10: Module loading, early codegen (This means **at least** generating "Hello, world" and **includes** linear types)
- [ ] By W11: Passing all the basic level codegens
    + The language's success will be based on a set of pre-specified code generation examples separated into 3 categories: basic, stretch, and future. Basic is the goal, stretch is extra, and future is for later.
- [ ] By end of W11: `define-component` + finishing up
- [ ] Polishing stage: Stretch goals (LSP, Multithreading), syntax fixes, pretty error reporting
- Web Components are complex and may not make it into the project release

##### Example of Basic Examples (MUST pass linear type checker)
```scheme
(assert (= (console.log "Hello, world!") '())) ; Should print "Hello, world!"
```
