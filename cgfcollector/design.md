# Design Overview

This document provides a high-level overview of `cgfcollector`.

More implementation details can be found in the relevant source/header files.

## Purpose

`cgfcollector` builds a call graph for Fortran code by traversing the Flang
parse tree and collecting:

- Nodes (procedures and related entities)
- Edges (call relations between nodes)

## Execution Flow

1. `Main.cpp` initializes the plugin and starts parse tree traversal.
2. `ParseTreeVisitor` walks grammar nodes (based on Flang Fortran 2018 grammar).
3. During traversal, procedures/types/temporary state are collected.
4. This information is used to insert the appropriate nodes and edges into the
   call graph.
5. Some edges (for example finalizer-related) can only be resolved at the end of
   the parse tree traversal, so they are collected as "potential" edges and
   added in a post-processing step.
6. Collected nodes and edges are emitted as the resulting call graph.

Grammar reference: https://flang.llvm.org/docs/f2018-grammar.html

## Main Components

- `Main.cpp`
  - Entry point and traversal setup.
- `ParseTreeVisitor`
  - Core traversal logic and feature handling.
- `Function`
  - Representation for Fortran procedures and related metadata.
- `Type`
  - Representation for Fortran type information and inheritance.
- `Edge` and `EdgeSymbol`
  - Representation of call-graph edges.
- `EdgeManager`
  - Manages edge collection.
- `PotentialFinalizer`
  - Deferred finalizer call candidate, resolved at end of traversal.
- `VariableTracking`
  - Variable initialization/state tracking (currently used for finalizers).
- `FortranUtil`
  - Shared Fortran helper utilities.

## Currently Covered Features

### Node Collection

- Procedures
- Main program
- Entry statements
- Intrinsics
- Statement function

### Edge Collection

- Subroutine calls
- Function calls
- Object-oriented calls
- Finalizer calls
- Operator overloading
- Generics and interface grouping
- Constructor calls
- C interoperability
- Statement function calls
- `USE` statement relations
- Nesting relations

## Current limitations

- Function pointer calls
- Unlimited polymorphic calls
- Operator overloading
- Variable tracking
