Expression Brackets
===================

One must often apply :ref:`axiom schema <axiomschema>` and :ref:`proof schema <proofschema>` to specific predicates. This is accomplished using expression brackets. Expression brackets allow the substitution of :doc:`predicates <predicates>` for unbound predicates in verified and unverified sentences.

Syntax
------

.. code-block::

	[parent expression][[predicate expression], [predicate expression], ...]

An expression brackets term begins with a parent expression followed by a ``[`` character, a comma-separated list of predicate expressions, and a closing ``]`` character.

The parent expression value must evaluate to a sentence value with at least one unbound :doc:`predicate <predicates>`. Each predicate expression must evaluate to a sentence value. The n-th predicate sentence is substituted for the n-th unbound predicate in the parent sentence, and the expression brackets term evaluates to the result. The n-th predicate sentence must have the same number of unbound objects as the number of arguments of the n-th unbound predicate of the parent sentence.

The expression brackets term evaluates to a verified sentence if and only if the parent sentence is verified.

Examples
--------

Suppose ``excluded_middle`` is a variable storing the following verified sentence for unbound predicate ``P()``:

.. code-block::

	P | ~P

Then we can use expression brackets to apply this result to any predicate depending on no arguments. For example, suppose ``X`` and ``Y`` are objects, and ``<`` is a :doc:`relation <relations>`. Then the following expression:

.. code-block::

	excluded_middle[<: X < Y>]

Evaluates to the following verified sentence:

.. code-block::

	X < Y | ~X < Y

Suppose that ``=`` is a relation and that ``replace`` is a variable storing the following verified sentence for unbound predicate ``P(1)``:

.. code-block::

	*X*Y(X = Y -> (P(X) <-> P(Y)))

If ``Z``, ``W``, and ``V`` are objects and ``<`` is a :doc:`relation <relations>`, then the following expression:

.. code-block::

	replace[<Q: Q < V>](Z, W)

Evaluates to the following verified sentence:

.. code-block::

	Z = W -> (Z < V <-> W < V)

Note that :doc:`object substitution <objectsubstitution>` is applied to the result of the expression brackets term.

