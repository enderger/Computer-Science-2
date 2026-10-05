---
title: "CS2 M1 Process Log"
author: Danielle Hutzley
---

# As of M1
The M1 document originated as a conversation between me and Claude Opus 5.5. I then had it structure what we discussed into the skeleton of this very document. It has seen the already work in progress lexer and helped me write its test suite. I will include updated AI process documentation in future documents.

# Testing
I have used AI to produce some test code. It can thoroughly test its tests faster than a human can write them, especially for the fuzz test. See the fuzzer as an example (I wrote the descriptive "how this works" and had Claude review it)

# M2 Graph
- I drew the whole diagram myself; this was my first UML diagram at this scale. The AI reviewed two drafts (17 points, then 18).
- The second review found that `AtomImpl::accept(Visitor<T>&)` couldn't override `Atom::accept(VisitorBase&)`. It also found that Module was move-only but owned by both the graph and the `BST<SymbolRef, Module>` the name resolver returned, and that the Category 6 BST had drifted into a name → module map.
- Short on time, I handed the AI the `.drawio` file. It applied the text and box fixes by editing the XML, and checked them by rendering with draw.io's own viewer (no clipped labels, no overlapping boxes). It added VisitorBase, Scope + Binding (a per-scope BST of bindings, owned by Module through `List<Scope>`), `PrefixEntry`, the `ModuleGraph` alias and a `cs2_lib::algorithm package` for Category 8. It also gave `List<T>` a last pointer and `pop_front`, and gave `Map` pair<const K, V> with a find that returns `V*`.
- I drew every arrow myself, renamed the package to algorithm, and committed the whole thing as one commit, labelled partial AI with a co-author tag.
