# CORE
A constructive proof assistant for propositional set theory.

## Introduction

CORE is a constructive proof assistant; more specifically, a language for writing computer verifiable proofs. A programmer for CORE writes axioms, definitions, 
and the logical steps for a proof in a constructive style of deduction, and the language checks syntax and verifies that *all* steps are logically sound. If
CORE successfully verifies a program, the programmer can rest soundly knowing that their proofs are air-tight, at least until they start questioning the axioms
that they are based on.

The language is in its infancy, so there are likely some ways of tricking the verifyer into having false statements proven. These are the most important bugs to
squash, so if anybody discovers one they should open an issue.

## Building and Installation

CORE only depends on `<stdlib.h>`, `<stdio.h>`, and `<string.h>`, so it can be compiled for most platforms. To build, simply use `make` if available, or otherwise
run:

```
CC dictionary.c proposition.c expression.c commands.c -Wall -o core
```

To install, simply copy the executable created after building to `/usr/bin` on UNIX-based distributions, or the equivalent directory on other platforms.

## Example Program

CORE comes with a model of the Zermelo Fraenkel set theory axioms and appropriate proofs to show that definitions are well defined. To verify these files, simply
run `core proofs/*`. These example programs are an excellent way to get to know the language.

The CORE verifier takes as arguments files to verify. The files are verified in-order, and subsequent files can be dependent on axioms, definitions, and theorems
from previous files. If there is an error, the CORE will give an error message and the location of the error. Otherwise, CORE will give a message that the proofs
have been successfully verified.

Since the language currently lacks documentation, let's go over an example program:

```
prove negate_exists[P(1)]: ~^X(P(X)) -> *X(~P(X)){
  implies not_exists{
    given X{
      not P_X{
        prove exists: ^X(P(X)){
          choose X{
            return P_X;
          }
        }
        return not_exists(exists);
      }
    }
  }
}
```

```
prove negate_exists[P(1)]: ~^X(P(X)) -> *X(~P(X)){
```

This is the proof that for any proposition `P`, if for no `X` is `P(X)` true, then for all `X`, `P(X)` is not true. The proof starts out with the `prove` keyword,
which just tells the language that we are starting a proof. Next comes the name of the proof, in this case `negate_exists`. The brackets afterwards bind the
proposition `P`, which depends on 1 object. What this means is that the proof will work for any proposition `P` we are given. Throughout the proof, we will be
able to refer to `P`, but we won't know anything about what `P` says since the proof should work for all such propositions. Finally, after the colon we state what
we are trying to prove. When we write a proposition like this, `~` means "not", `^` means "there exists", `->` means "implies", and `*` means "for all".

Now the actual proof begins. The proof always starts with braces, which declares a new scope similar to how other languages have loops scoped. Scopes are used
in CORE to separate variables. Variables in CORE are used to store two and only two types of data: *statements* and *objects*. Objects don't store anything inside
of them; instead, statements can refer to objects by name. The types of variables are never statically declared in CORE because in every operation which
instantiates a variable, the type of data the variable is storing is already defined by the operation.

```
implies not_exists{
```

When we begin our proof, our goal is to prove `~^X(P(X)) -> *X(~P(X))`. The language interprets this quite literally: proving no other statement will suffice.
However, it is often possible to transform the goal into something which is easier to prove. This is precisely what the `implies` command does. In order
to prove a statement which looks like `A -> B`, one can assume that `A` is true and prove `B`. In this case, `A` is `~^X(P(X))` and `B` is `*X(~P(X))`. `implies`
assigns to `not_exists` inside of the new scope the statement `~^X(P(X))`, and our new goal in the following scope becomes to prove that `*X(~P(X))`. 
By assigning `A` to `not_exists`, the language is allowing us to assume that `A` is true in order to prove inside of the new scope that `B` is true.

```
given X{
```

Now our goal looks like proving "for all X", something is true. The typical strategy for doing this is to assume you are given an arbitrary object, and to prove
that the statement is true for that object. This is what `given` does: we are given an object `X` and we now must prove that `~P(X)`. Unlike with `implies`, 
`X` is a variable that is created as an *object*, not a *statement*. Notice that in order for this object to have any meaning, there must be some other statements
which refer to the object `X`. In fact, our new goal in the next scope, to prove that `~P(X)`, gives this object meaning.

```
not P_X{
```

When proving a statement that looks like `~A`, a common strategy is to suppose `A` was true and to prove a contradiction. This is how the next keyword, `not`,
works in the proof. Since our goal looks like `~P(X)`, we are assuming `P(X)` by storing it into the variable `P_X`, and our new goal literally becomes to prove
`false`. Obviously false isn't actually true, but this is how we structure a proof by contradiction in CORE.

Let's recap. We have our proposition `P`, depending on one variable which we know nothing about. We have our variable `not_exists`, which says that there doesn't
exist an object for which `P` is true, ie `~^S(P(S))`. We have our object `X`, and now also our variable `P_X` which says that `P(X)` is true. Clearly we've
found an `S` for which `P(S)` is true: it's just `X`. This contradicts the statement stored in `not_exists`, so it looks like we're in a good spot. To finish the
contradiction, it would make sense to try to prove `^S(P(S))` by "choosing" our `X`.

```
prove exists: ^X(P(X)){
```

This is exactly what we do next. We start a proof inside of a proof, and we call the result `exists`, and we are trying to prove that `^X(P(X))`. To be clear, the
variable `X` inside of this statement is re-bound, so temporarily the `P(X)` in the parentheses is referring to the variable just bound, not the variable `X` we
already have.

```
choose X{
```

Anyways, we use `choose`, which is for when we are proving a statement of the form `^S(...)`. We give the language an object we already have, `X`, and in the
process we are "claiming" that the desired object (whose existence we are claiming) is in fact `X`. Inside of the new scope, our goal becomes to prove 
`P(X)`, where `X` now refers to the object we just passed to `choose`.

```
return P_X;
```

But notice that we already have that `P_X` stores literally `P(X)`. When we are in the situation where we already have a statement derived which exactly matches
the goal, we use the `return` command. In this case, after `return P_X;`, the language will check that the value we just passed to return does in fact match the
goal. Since it does, this completes the proof of `exists`!

```
return not_exists(exists);
```

Now that the proof of `exists` is complete, the language takes the statement we just proved (that `^X(P(X))`), and stores it's value into `exists` *as a variable*.
In other words, by completing the proof, we've just created a new variable `exists` which stores the opposite of what `not_exists` stores. Remember, we were doing
originally a proof by contradiction, so our goal right now is to prove `false`. In CORE, a statement which has been proven of the form `~A` is treated sort of as
a "function," which, when given the statement `A`, returns the statement `false`. So when we write `return not_exists(exists);`, we are "passing" the statement
`^X(P(X))` through `~^X(P(X))`. The expression `not_exists(exists)` then evaluates to `false`. When we return this value, the language checks that this indeed
matches the current goal of the proof, and when it sees that it does, it completes the proof. Now that the proof is complete, the variable `negate_exists`
(globally) stores the statement that for any proposition `P` depending on 1 variable, `~^X(P(X)) -> *X(~P(X))`!
