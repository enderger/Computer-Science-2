---
title: "Vectype: AI Process Log (M2 Design)"
author: Danielle Hutzley
date: "4 Oct 2026"
geometry: margin=0.75in
---
 
<!--
Partial AI: drafted with AI on 2026-10-04 from our session notes.
-->
 
*This log was drafted with AI from my session notes and reviewed by me. I use AI as a reviewer and thinking partner; I write the code myself, and AI-written code is marked in the source.*
 
## 1. Class diagram review 
- **Goal:** Draw a class diagram for the whole compiler that I could build from, and that would show where each rubric category lives.
- **What I asked:** For a review of my first draft (my first UML diagram at this scale), and then of the second draft.
- **What I got and what I did:**
    + The reviews found 17 issues, then 18. The important ones were design bugs, not drawing mistakes:
        - `AtomImpl::accept(Visitor<T>&)` couldn't override `Atom::accept(VisitorBase&)`, so `Atom` would have stayed abstract.
        - `Module` is move-only (its AST holds `unique_ptr<Atom>`) but was owned twice, by the module graph and by the name resolver's `BST<SymbolRef, Module>`.
        - My Category 6 BST had drifted into a name-to-module map; I replaced it with a `Scope` holding a BST of `Binding`s.
    + Short on time, I had the AI apply the text fixes to the `.drawio` file; it checked its edits by rendering the file. I drew every arrow myself and committed it labelled as partial AI.
    + For the document, I designed a simplified overview with category tags, with the full diagram linked by QR code. I rejected a landscape appendix page. The AI pointed out that a self-contained draw.io URL (about 11k characters) won't fit in a QR code, so I linked the file in my repo, pinned to a commit.
- **What I learned:** Drawing the design exposed bugs that weren't visible in my head: ownership and which structure actually serves which category. I also spent far more time on the diagram than planned, and should budget design artefacts like any other task.

## 2. Modelling the AST 
- **Goal:** Decide how the reader's output (`Datum`) and the analyzer's output (`AST`) represent S-expressions, including improper lists like `(a . b)`.
- **What I asked:** Whether `Datum` should be a variant of the polymorphic `Atom` and a list, whether a list's tail should be a variant of `T` and `List<T>`, and what the standard terms are (datum, form, reader, analyzer).
- **What I got and what I did:**
    + Compiling small test cases showed that `std::variant` needs complete types, so a list tail can't hold its own element type by value; it has to be a `unique_ptr`. `std::optional<T>` also fails on an incomplete type.
    + I proposed CRTP-ing the tail type, then settled on one `ConsList<T>` (a list plus a tail) for both the datum and AST stages, leaving `std::variant` to choose between atom and list. One structure does one job, like the Unix philosophy.
    + The AI introduced a `DatumList` without saying the variant's alternative changed too, which confused things until it was clarified.
- **What I learned:** C++'s completeness rules decide a lot of the data model for recursive types, and it's quicker to check them by compiling than by reasoning. A single generic cons list kept both stages simpler than special node types would have.

## 3. Choosing the hash function 
- **Goal:** Pick the hash for the interner's hash table, which the spec says I must be able to explain.
- **What I asked:** For a good, simple hash. Earlier, I had also asked whether the rubric wants a hash *set* specifically, or whether a BST set would count.
- **What I got and what I did:**
    + I had already ruled out K&R for collisions. The AI showed why concretely: with multiplier 31, `"Aa"` and `"BB"` hash to the same value, and this repeats structurally.
    + I chose 64-bit FNV-1a. The hash became a template parameter of `Map`, because specializing `std::hash<std::string>` isn't allowed.
    + Rereading the spec answered the set question: it requires a hash table with separate chaining used for real lookups. The interner needs name → `SymbolRef`, so it became a `Map` with my own `List` as buckets.
- **What I learned:** A hash can fail on very ordinary inputs, not just adversarial ones, and that's worth checking before choosing. Questioning the spec's wording before building saved me from building the wrong structure.

## 4. Pass order and where the interner lives 
- **Goal:** Fix the order of compiler passes, to settle the class diagram and the milestone plan.
- **What I asked:** To check my order: Lexer → Reader → Analyzer → Name Resolver → Module Resolver → Linear Type Checker → Emitter. Also where the interner belongs; I had pictured it after name resolution as its own small phase.
- **What I got and what I did:**
    + Module resolution has to come before name resolution, because names can't be resolved until imports are loaded. Name resolution then runs per module, in the graph's compile order.
    + The interner moved into the analyzer, shared across all modules, because the AST already needs `SymbolRef`s.
    + I chose define-before-use, with explicit `define-mutrec` and `let-mutrec` groups for mutual recursion, so resolution is a single pass. The cost is having to order definitions, which I accepted for a language of this size.
- **What I learned:** The order of passes follows from what data each pass needs. Writing down each pass's inputs made the mistake obvious.

## 5. Testing and fuzzing the lexer 
- **Goal:** Make the lexer's test suite trustworthy before building the parser on it, including GNU-style error locations (`file:line.col-line.col`).
- **What I asked:** To review failing and passing tests. At one point I asked "is this WIP code correct? It passes, but so do a lot of bad tests". I also asked how to combine the fuzzer's property checks on its hot path, and for the fuzz token loop itself.
- **What I got and what I did:**
    + On the GNU format: I found that columns must start at 1, and came up with printing a zero-width token as a single `file:line.col`. A span ending in a newline ends on the previous line. My `get_newline_column` took several rounds: it first failed on blank lines, then its guard was inverted, then off by one.
    + The review of the passing tests found real bugs: a `zip` that silently truncated (with a missing size check), a span factory built from the wrong data, impossible test spans, and a pasted block that made the fuzzer 3.4× slower.
    + The AI wrote the fuzz token-loop body at my request. It's marked `BEGIN/END AI CODE` in the test file, and I checked my comment describing it for accuracy.
    + The final suite passes 128 assertions under ASan and UBSan, and mutation tests (deliberately broken lexers) confirmed the suite catches them.
- **What I learned:** A passing test suite proves nothing on its own; deliberately breaking the code is how you find out whether the tests can fail. Boundary cases like line/column arithmetic are where my bugs actually were.
