Quantifiers
===========

Quantifiers bind new :doc:`objects <objects>` in :doc:`sentences <sentences>`. CORE has two quantifiers: universal and existential. Objects bound by a quantifier mask existing objects with the same identifier in the following sentence term.

Syntax
------

.. code-block::

	*[object identifier] [sentence term]
	^[object identifier] [sentence term]

The universal and existential quantifiers are denoted by ``*`` and ``^`` respectively. After a quantifier follows a :ref:`sentence term <sentenceterm>` which may refer to the variable bound by the quantifier.

.. _universal:

Universal Quantifiers
---------------------

Universal quantifiers are denoted by ``*``. If ``P`` is a sentence with an unbound :doc:`object <objects>`, then the sentence ``*X(P(X))`` is interpreted to mean that for all objects ``X``, the sentence ``P(X)`` is true.

A sentence of the form ``*X(P(X))`` can be proven using the :doc:`given <given>` command. A proven sentence of the form ``*X(P(X))`` can be used to deduce ``P(Y)`` for any object with identifier ``Y`` using :doc:`object substitution <objectsubstitution>`.

.. _existential:

Existential Quantifiers
-----------------------

Existential quantifiers are denoted by ``^``. If ``P`` is a sentence with an unbound :doc:`object <objects>`, then the sentence ``^X(P(X))`` is interpreted to mean that there exists an object ``Y`` such that the sentence ``P(Y)`` is true.

A sentence of the form ``^X(P(X))`` can be proven using the :doc:`choose <choose>` command. A proven sentence of the form ``^X(P(X))`` can be used to construct an object ``Y`` in the current scope such that ``P(Y)`` is true. See :doc:`pipe` for more information.

Examples
--------

If ``=`` is a relation and ``subset`` is a predicate depending on two objects, then one can form the following sentence:

.. code-block::

	*X*Y subset(X, Y) & subset(Y, X) -> X = Y

If the objects ``X`` and ``Y`` do not exist in the current scope or any parent scope, then this sentence is semantically invalid. The reason is that ``subset(X, Y)`` is a :ref:`sentence term <sentenceterm>`, so the sentence is parsed as:

.. code-block::
	
	((*X*Y(subset(X, Y))) & subset(Y, X)) -> (X = Y)

In this example, the objects ``X`` and ``Y`` which are quantified are only accessible within the :ref:`sentence term <sentenceterm>` ``subset(X, Y)``. If ``X`` and ``Y`` are objects accessible in the current scope, however, then this is a valid sentence. To avoid ambiguity, it is best practice to add parentheses after a sequence of quantifiers, as in the following example:

.. code-block::

	*X*Y(subset(X, Y) & subset(Y, X) -> X = Y)
