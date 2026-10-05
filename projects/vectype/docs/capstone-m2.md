---
title: Capstone M2 Document
geometry: margin=0.5in
---

<!--
Partial AI. Written with AI on 2026-10-04: Milestone Plan, Concerns and Unknowns,
and corrections throughout the earlier sections.
-->

## System Overview
Vectype is a LISP family programming language with linear types targeting WebAssembly.

The pipeline of this compiler is as follows:
1. Source text, loaded once and passed by reference throughout, outliving the entire pipeline
2. Per file:
    1. Lexer: Range adaptor, turns source text into tokens which borrow the source text.
    2. Reader: Parses the tokens into a cons list of `Datum`s using a parse stack
    3. Analyzer: Translates the `Datums` into an `AST` of polymorphic typed atoms (behind `unique_ptr`) using the visitor pattern
        - This also interns every symbol into `SymbolRefs` via the `Interner`
3. Module Resolver: Starting from the entry file, loads each imported file through the "per file" stages and builds the module graph
    - A DFS produces the compile order and rejects import cycles
    - Each module's import table is frozen: quicksorted, then check for duplicates
4. Per module, in compoile order:
    1. Name Resolver: Fills the module's scope field: one BST of bindings per scope.
        - Any non-local symbol is binary searched in the import table
    2. Linear Type Checker: Provides memory safety guarantees without a complicated garbage collector or borrow checker
    3. Emitter: Produces WebAssembly (in the form of WAT) and JavaScript glue code

The `Lexer` and `Reader` live in `cs2_lib`; they can be reused as a data format as  a form of dogfooding.

## Class Diagram

![Class diagram, arrows point towards the user](./images/Vectype-Core Diagram.drawio.pdf){height=5in}

- The dashed lifeline shows how lifetimes flow through the program until destruction at the end of the program. 
- Everything in the left box is inside of Vectype itself while everything on the right is my computer science 2 library (`cs2_lib`)

##### Full Diagram
A full version of this diagram can be found [here](#appendix-1)

## Category Mapping
#### Containers / Generic Programming
##### Templates
- Examples: `List<T>`, `ConsList<T>`, `Bst<K, V>`, `Map<K, V, H>`, `Graph<T>`
- These all gain flexibility from being templates; `List` for instance works as an AST, a parse stack, `Map`'s bucket, etc.
- Concepts are used where they make sense (preferring standard concepts where possible).

##### Polymorphic Hierarchy
- Example: `Atom` interface
- `AtomImpl<T>` as a CRTP base class implementing the `accept(VisitorBase&)` override
- Children include (but are not limited to) `IntAtom`, `SymAtom`, `StrAtom`
- This is held via a `unique_ptr` in the `AST`
- Polymorphism is used here for a number of reasons:
    + Adding a new `Atom` type only requires creating an atom class and adding it to the visitor
    + Writing all the atom types into a type alias would become unwieldy
    + `Atom`s have a common interface that polymorphism (both regular and CRTP) can use to avoid code duplication.

#### Resource Management
##### Rule of Five
- `List<T>` is an example of the Rule of Five; it has a copy constructor/assignment operator, a move constructor/assignment operator, and a destructor.
- This is because a recursive destructor could easily overflow the stack (~59k nodes at `O0`, ~3.6k on a 512KiB thread), making custom iterative destruction the only solution.
    + This destructor means that the Rule of Five applies, since move semantics are desirable.
    + The Rule of Five is a strict superset of the Rule of Three, so it applies here.

##### Virtual Destructors
- `Atom` and `VisitorBase` have virtual destructors; their children both might have data that needs destruction.
- Everything built from these follow the Rule of Zero (`Module`, `Scope`, `ImportTable`, children of `Atom`, etc).

#### Linked Structure
- `List<T>` is a singly linked list with only a `unique_ptr<Node<T>>` as its head
- Its member functions are:
    + `emplace_front` : Construct a node in place on the front of the list.
    + `push_front` : Push an existing value at the front of the list
    + `pop_front` : Pop a value off the front and return it as an `prvalue` (or throws on the empty list)
    + `reverse` : Reverse the list in-place
    + `head` / `tail`: The `car` and `cdr` operations similar to LISP
    + `begin` / `end`: Range implementation
    + There is no back pointer; this list is singly linked
- S-expressions are fundamentally cons lists, making most structures based on them representable by a pair of a linked list and a CDR pointer
- I also use `List<T>` buckets in `Map<K, V, H>`

#### Stacks/Queues
- `List<T>` is used as a stack (via `emplace_front`, `push_front`, `head`, and `pop_front`) in the `Reader`'s `parseStack`.
    + An open list is pushed at `(`, children are `emplace_front`ed onto it, and `)` pops the list, reverses it, and adds it to the parent.
- A stack is used as nesting of S-expressions is inherently last-in, first-out and recursion is bounded by the stack.
- I'm using `List<T>` here because I understand how it works better

#### Hash Table
- `Map<K, V, H = std::hash<K>>` (where `H: func(const K&) -> size_t` and `K: std::equality_comparable`).
- Storage is a `std::vector<List<std::pair<const K, V>>>`
- Member functions:
    + `insert`: Add a value to a key
    + `find` : Get a pointer (because `optional` doesn't work on references until C++26)
    + `size`, `bucket_count` : Fairly self explanatory
    + `load_factor` : The load factor of the map
    + `begin`, `end` : The iterator pair for the map
- The load factor and growth strategy are both open questions; I don't know enough to make those decisions yet.

##### Uses
- `Interner`: `Map<std::string, SymbolRef, Fnv1a>` to map symbols to their interned values + `std::vector<std::string>` for reverse lookup
- `import-ffi` table, `Map<SymbolRef, FfiSignature>` keyed by a quoted JS path

##### Implementation:
Fnv1a works by multiplying a base (in our case `14695981039346656037`), then XORing in each byte and multiplying it by a multiplier (in our case `1099511628211`).

We aren't using K&R because a multiplier of `31` gives structural hash collisions

#### Trees 
- `Bst<K: totally_ordered, V>`, with `insert(K, V) → pair<V&, bool>`, `find(const K&) → V*`, `in_order(visitor)`, `size()` and `height()`. There's no deletion: scopes only grow.
- A BST is used as I insert one at a time during resolution. The import table is sorted once and thus gets regular binary search on a sorted list.
- In the worst case, insertion is in sorted order leading to a linked list with O(N) lookup.

##### Uses
- `Scope { table: Bst<SymbolRef, Binding>; parent: Scope* }`. `lookup` walks the parent chain. The traversal's real use is the symbol dump. The `SymbolRef` is an incrementing counter multiplied by the odd 64-bit constant `0x9E3779B97F4A7C15` to avoid the worst case (plain incrementing integers being in linked list order)

#### Graphs
- `Graph<T>` holds adjacency lists (`std::vector<std::vector<NodeIdx>>`) and node data.
- Its members are `add_node`, `add_edge`, `get`, `successors`, `topological_order` (a three-state DFS) and `to_dot`.
- Used as `ModuleGraph = Graph<Module>` where edge `a -> b` means `a` imports `b`
- Imports are directed (they are asymmetric and cycles mean a program is ill formed) and unewighted (as there is no cost to importing).
- We compile in DFS order

#### Search / Sort
- `cs2_lib::algorithm` provides `quicksort(std::span<T>, Compare)`, with a median-of-three pivot, and `binary_search(std::span<const T>, const K&, Compare) → const T*`, a lower-bound search.
- Median of 3 is used as the `SymbolRefs` arrive mostly sorted. Since the table can be frozen (and thus only sorted once), a BST that's slower but retains sort order is a poor trade-off.

##### Uses
- `ImportTable`: `freeze()` quicksorts the entries and prefixes, then rejects adjacent duplicates. `find` and `find_prefix` binary-search them.

##### Time Complexity

| Algorithm     | Complexity | Average Complexity | 
|---------------|------------|-------------|
| quicksort     | O(n^2)     | O(n log n)  |
| binary_search | O(log n)   | O(log n)    |

<!-- BEGIN AI WRITTEN SECTION -->
## Milestone Plan
- [x] Done: Lexer in `cs2_lib`, with a doctest suite and fuzzing
- [ ] W8: `List<T>` (Rule of Five) and the `Reader` with its parse stack
- [ ] W9: `Atom`/`AST`/`Analyzer`, `Map` + `Interner`, and the FFI forms for "Hello, world"
- [ ] By end of W10 (Categories 1–5):
    + "Hello, world" compiles to WAT and runs. This is the bare minimum for core.
    + Demo: parse a file and print the AST, the parse stack's maximum depth, and the interner's load factor and bucket count
- [ ] By end of W11 (Categories 6–8 + Integration): `Bst` + `Scope`, `Graph` + `ModuleResolver`, `algorithm` + `ImportTable`, and the `NameResolver`
    + Demo: a multi-module program, printing its load order (or `to_dot`), a symbol dump, and the frozen import table
- [ ] After W11: the full linear type checker and emitter, `define-component`, then stretch goals (LSP, multithreading)

##### Path to "Hello, world"
- `import-ffi` declares a JS function's signature once; `invoke-ffi` calls it by quoted name (`'console.log`), inside `unsafe`, and must match the declaration. `Str` is a glue type (pointer + length, decoded by the generated glue).
- A single file with a quoted name needs no module resolver, import table, or name resolver, and the glue generator is the same machinery `define-component` needs later.
- I currently have a working lexer and test suite for it. The Reader and list remain both as todos, I got carried away with the graph. I have more time this week than usual, so I can dedicate it to implementing those.
 
## Concerns and Unknowns
- Which grading scale is authoritative? I got a grade on the proposal, but the scale is still unclear.
- Can categories be demonstrated through debug output (the dumps in the milestone plan)?
- The hash table's maximum load factor and growth strategy need to wait until we've covered hash tables in class.
- The backend is the largest unknown, and there's only one week between W10 and W11.
- Web components need generated JavaScript glue, since WebAssembly can't touch the DOM directly.
- The BST's balance comes from scrambling keys, so a short tree is expected rather than guaranteed.
<!-- END AI WRITTEN SECTION -->

# Appendix
## Appendix 1: Full Graph {#appendix-1}
This QR code will take you to the full version of my class diagram, frozen in time by a commit.
This is a render of the diagram at `./diagrams/Vectype.drawio`.

![](images/full-diagram-qr.png){width=1.2in}
Full diagram: <https://viewer.diagrams.net/?nav=1#Uhttps%3A%2F%2Fraw.githubusercontent.com%2Fenderger%2FComputer-Science-2%2F4838ac879e808adfe857fc48bd07f3132dc97fa8%2Fprojects%2Fvectype%2Fdocs%2Fdiagrams%2FVectype.drawio#%7B%22pageId%22%3A%22lx7v-Pw6hcbanZI6vCwH%22%7D>
