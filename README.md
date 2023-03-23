# CORE
A constructive proof assistant for second order logic.

## Introduction

CORE is a minimal proof assistant capable of verifying proofs in various axiomatic systems. See the `proofs` folder for an implementation of set theory.

## Building and Installation

To build, use `make`. The flag `USE_CUSTOM_ALLOC` determines whether the verifyier wraps the allocator, which is useful for debugging.

To install, copy the executable `core` created after building to `/usr/bin`.

## Examples

CORE comes with a model of the Zermelo Fraenkel set theory axioms and appropriate proofs to show that definitions are well defined. To verify these files, simply
run `core proofs/main.core`. These example programs are an excellent way to get to know the language.

See the (incomplete) [documentation](https://core-verifier.readthedocs.io/en/latest/index.html) for a detailed description of the language.

## Various Features

- Straight-forward syntax
- Detailed error messages
- Axiom schema
- Proof schema
- Built-in rules to seamlessly determine whether one sentence trivially implies another (See [trivialtruth](https://core-verifier.readthedocs.io/en/latest/trivialtruth.html) in the documentation for more information.)
- Import and include system for organizing results and applying proofs in multiple contexts
