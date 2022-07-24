axiom
=====

The ``axiom`` command is used to assert the truth of a sentence and store the sentence in a :ref:`sentence variable <sentencevariable>` as a :ref:`verified sentence value <sentencevalue>`.

Syntax
------

.. code-block::

	axiom [axiom identifier]: [sentence];

The axiom command begins with the keyword ``axiom``, followed by a variable identifier. A ``:`` character denotes the beginning of a ``;`` terminated sentence.

Axiom Schema
------------

.. code-block::

	axiom [axiom identifier][[predicate0]([num args]), [predicate1]([num args]), ...]: [sentence];

Optionally, after the axiom identifier a list of unbound predicates may follow. The list of unbound predicates begins with a ``[`` character. This is followed by a comma-separated list of predicates. Each entry in the list begins with a predicate identifier. After the predicate identifier, the number of arguments of the predicate depends on is denoted with parentheses and 
