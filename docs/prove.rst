prove
=====

The ``prove`` command verifies a sentence and stores it as a :ref:`verified sentence value <sentencevalue>` into a :ref:`sentence variable <sentencevariable>`.

.. _`proofschema`:

Syntax
------

.. code-block::

	prove [identifier]: [sentence]{
		...
	}
	prove [identifier][[predicate0](num_args), [predicate1], ...]: [sentence]{
		...
	}

The ``prove`` command begins with the keyword ``prove`` followed by a variable identifier. Optionally, a list of predicate identifiers may be present. The list begins with the ``[`` character, and ends with the ``]`` character. Each entry in the list is a predicate identifier, followed by an optional number of arguments. If present, the number of arguments is denoted by the ``(`` character, a non-negative integer, and the ``)`` character. Entries in the predicate list are separated with the ``,`` character.

Following the variable identifier and optional predicate identifiers is the ``:`` character and a sentence. The command is terminated with a new :doc:`scope <scopes>`.

Behavior
--------

The sentence ``[sentence]`` may depend on the bound :doc:`predicates <predicates>` specified in the predicate list. If the code in the scope successfully executes, this sentence is stored as a verified :ref:`sentence value <sentencevalue>` into the :ref:`sentence variable <sentencevariable>` ``[identifier]``.

.. _`goal`:

The execution of the code inside of the new scope depends on sentence specified by ``[sentence]``. This sentence is called the *goal* of the scope.

The following commands only run in a scope with a *goal*:

.. toctree::
	:titlesonly:

	assume <assume>
	not <not>
	given <given>
	choose <choose>
