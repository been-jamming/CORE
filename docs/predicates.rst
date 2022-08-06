Predicates
==========

A predicate is a symbol which may reference :doc:`objects <objects>` in a :doc:`sentence <sentences>`. A predicate may depend on zero or more arguments (objects), and may or may not have a *sentence definition*. Predicates are a convenient tool for formally defining a concept and for hiding complicated sentences under a label.

A *sentence definition* is a sentence used to define a predicate. If a predicate depends on ``n`` arguments, the sentence definition has ``n`` unbound variables. A predicate applied to ``n`` objects is a sentence logically equivalent to substituting in order those ``n`` objects for the unbound variables in the sentence definition.

.. _predicateidentifier:

A predicate identifier must begin with either a letter or an underscore, and may subsequently contain letters, digits, and underscores.

Predicates are quantified only by :ref:`axiom schema <axiomschema>` and :ref:`proof schema <proofschema>`.

Creation
--------

Bound predicates are created in the scope of a :ref:`proof schema <proofschema>`. Bound predicates have no sentence definition.

Predicates can also be created in any scope using the :doc:`define <definh>` command.

Examples
--------

Suppose ``in`` is a :doc:`relation <relations>` representing set membership and ``subset`` is a predicate defined by the following sentence:

.. math::

	\forall X \forall Y (\text{subset}(X, Y) \leftrightarrow \forall Z (Z \in X \rightarrow Z \in Y)))

In CORE's syntax, this sentence would be written as:

.. code-block::
	
	*X*Y(subset(X, Y) <-> *Z(Z in X -> Z in Y))

Then the sentence

.. code-block::

	*X(subset(X, X))

would mean:

.. code-block::

	*X*Z(Z in X -> Z in X)

