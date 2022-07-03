Pipe Operator
=============

New :ref:`objects <objects>` can be constructed from a verified sentence value beginning with a top-level :ref:`existential quantifier <existential>` using the *pipe operator*.

Syntax
------

.. code-block::

	[sentence expression]|[object identifier], [object identifier], ...|

A pipe operator sentence term begins with a *parent expression* evaluating to a :ref:`verified sentence value <sentencevalue>`. The sentence referenced by the sentence value must begin with a top level :ref:`existential quantifier <existential>`. In other words, the sentence must be of the form ``^X(P(X))`` for some sentence ``P`` with one unbound object. Following the parent expression is the character ``|``, one or more :ref:`object identifiers <objectidentifier>`, and a final ``|`` character.

For each object identifier, a top level existential quantifier is peeled from the parent expression sentence. A new object in the current scope is created with the object identifier, overwriting any variables or objects sharing the same identifier (This raises an error if the object or variable cannot be overwritten). Then, this new object is substituted for the unbound variable in the sentence. After doing this for each object identifier, the pipe operator sentence term evaluates to the remaining sentence.

The pipe operator can only be applied to a parent expression evaluating to a :ref:`verified sentence value <sentencevalue>` with no unbound predicates. Consequently, a pipe operator sentence term always evaluates to a verified sentence value with no unbound predicates.

Examples
--------

If ``in`` is a :doc:`relation <relations>`, ``S`` and ``T`` are :doc:`objects <objects>`, and ``axiom_pairing`` is a :ref:`sentence variable <sentencevariable>` storing the following verified sentence:

.. code-block::

	*X*Y^Z(X in Z & Y in Z)

Then the expression ``axiom_pairing(S, T)|P|`` creates an object ``P`` and evaluates to the following verified sentence:

.. code-block::

	S in P & T in P

Working in set theory where ``in`` means set membership, ``P`` is now a set which contains both ``S`` and ``T``. This new sentence value can act as a definition for ``P``.

