Object Substitution
===================

:doc:`Objects <objects>` accessible from the current scope can be substituted for an object bound by a :ref:`universal quantifier<universal>` using *object substitution*. Object substitution results in a new sentence expression value without the universal quantifier.

Syntax
------

.. code-block::

	[sentence expression]([object expression], [object expression], ...)

Object substitution begins with a *parent expression* evaluating to a :ref:`sentence value <sentencevalue>`. The sentence referenced by the sentence value must begin with a top level :ref:`universal quantifier <universal>`. In other words, the sentence must be of the form ``*X(P(X))`` for some sentence ``P`` with one unbound object. Following the parent expression is ``(`` and one or more expressions evaluating to :ref:`object expression values <objectvalue>`. The ``(`` is matched with a closing ``)``.

For each object expression value, a top-level universal quantifier is peeled from the parent expression sentence, and the unbound variable is replaced with the object.

Object substitution can only be applied when the parent expression evaluates to a sentence value with no unbound variables or predicates. If the parent expression evaluates to a :ref:`verified sentence value <sentencevalue>`, then the object substitution evaluates to a verified sentence value. Otherwise, the object substitution evaluates to an :ref:`unverified sentence value <unverifiedvalue>`. In any case, the new sentence value has no bound variables or predicates.

Examples
--------

If ``=`` and ``<=`` are :doc:`relations <relations>`, ``S`` and ``T`` are :doc:`objects <objects>`, ``subset_equal`` is a :ref:`sentence variable <sentencevariable>` storing the following sentence:

.. code-block::

	*X*Y(X <= Y & Y <= X -> X = Y)

Then the expression:

.. code-block::

	subset_equal(S, T)

Evaluates to the sentence value:

.. code-block::

	S <= T & T <= S -> S = T

If the sentence stored by ``subset_equal`` is verified, then this sentence value is also verified, otherwise it is unverified.

