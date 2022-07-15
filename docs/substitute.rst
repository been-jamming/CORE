substitute
==========

The built-in function ``substitute`` allows for the replacement of a sub-sentence with a :ref:`biconditionally equivalent <biconditional>` sub-sentence.

Syntax
------

.. code-block::

	substitute([sentence argument expression])

Behavior
--------

The argument must be an expression evaluating to a :ref:`sentence expression value <sentencevalue>`. The argument may or may not be verified, but it must have no unbound objects and exactly one unbound predicate, otherwise an error is raised.

The output expression value is a verified sentence with no unbound objects and two unbound predicates. Let the argument sentence be denoted by ``A`` with unbound predicate ``P`` depending on n objects. Let ``Q`` be a new unbound predicate depending on ``n`` objects. Let ``B`` denote the sentence in which each occurence of ``P`` in ``A`` is replaced with ``Q``. Then the output sentence is the sentence of the following form:

.. code-block::

	*X1*X2*[...]*Xn(P(X1, X2, [...], Xn) <-> Q(X1, X2, [...], Xn)) -> (A <-> B)

The first ``[...]`` means that there are n :doc:`quantifiers <quantifiers>`, and the second ``[...]`` means that each of those n quantifiers are the arguments of ``P`` and ``Q``.

Examples
--------

Intuitively, the substitute command allows us to conclude that ``P`` can be replaced with ``Q`` if ``P`` and ``Q`` mean the same thing.

For example, suppose ``=`` is a relation, and ``symmetry`` is a variable storing the following verified sentence:

.. code-block::

	*X*Y(X = Y <-> Y = X)

Furthermore, suppose ``transitivity`` is a variable storing the following verified sentence:

.. code-block::

	*X*Y*Z(X = Y & Y = Z -> X = Z)

Then the following expression:

.. code-block::

	substitute(<P(2): *X*Y*Z(X = Y & P(Y, Z) -> P(X, Z))>)[<A, B: A = B>, <A, B: B = A>](symmetry)(transitivity)

Evaluates to the following verified sentence:

.. code-block::

	*X*Y*Z(X = Y & Z = Y -> Z = X)
