axiom
=====

The ``axiom`` command is used to assert the truth of a sentence and store the sentence in a :ref:`sentence variable <sentencevariable>` as a :ref:`verified sentence value <sentencevalue>`.

Syntax
------

.. code-block::

	axiom [axiom identifier]: [sentence];

The axiom command begins with the keyword ``axiom``, followed by a variable identifier. A ``:`` character denotes the beginning of a ``;`` terminated sentence.

.. _`axiomschema`:

Axiom Schema
------------

.. code-block::

	axiom [axiom identifier][[predicate0]([num args]), [predicate1]([num args]), ...]: [sentence];

Optionally, after the axiom identifier a list of unbound predicates may follow. The list of unbound predicates begins with a ``[`` character. This is followed by a comma-separated list of predicates. Each entry in the list begins with a predicate identifier. After the predicate identifier, the number of arguments of the predicate depends on is denoted with parentheses and 

Behavior
--------

The sentence specified in the axiom command, which may depend on the optional list of unbound predicates, is stored as a :ref:`verified sentence value <sentencevalue>` into the :doc:`variable <variables>` with the identifier ``[axiom identifier]``.

For each axiom, an :ref:`axiom dependency <axiomdependency>` is created in the current scope. Since the ``axiom`` command creates a dependency, the command can only be used in a :ref:`global scope <globalscope>` or a :ref:`dependent scope <dependentscope>`.

Axioms with unbound predicates act as axiom schema through the use of :doc:`expression brackets <brackets>`.

Examples
--------

Suppose ``<`` is a relation. Then the following commands:

.. code-block::

	object ZERO;
	axiom zero_minimal: *X(~X < ZERO);

Create an object named ``ZERO`` and an axiom which asserts that no object ``X`` is related to ``ZERO`` by ``<``, or with the usual interpretation of ``<``, no object is less than ``ZERO``.

The following command:

.. code-block::

	axiom strong_induction[P(1)]: *N(*M(M < N -> P(M)) -> P(N)) -> *N(P(N));

Encodes second-order strong induction as an axiom schema. The axiom can be applied to any predicate depending on one argument.
