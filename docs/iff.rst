iff
===

The built-in function ``iff`` accepts two :ref:`sentence expression values <sentencevalue>`, each of which are converse implications, and returns the a sentence with a top-level :ref:`biconditional connective <biconditional>`.

Syntax
------

.. code-block::

	iff([sentence argument expression], [sentence argument expression])

Behavior
--------

Each argument must evaluate to a :ref:`sentence expression value <sentencevalue>` with no unbound objects or predicates. Both arguments must evaluate to a sentence of the form ``A -> B`` for some sentences ``A`` and ``B``, otherwise an error is raised. If the first argument is of the form ``A -> B`` and the second argument is of the form ``C -> D``, the verifier checks that ``A`` is trivially equivalent to ``D`` and that ``B`` is trivially equivalent to ``C``. If not, an error is raised. The output is the sentence ``A <-> B``.

The output sentence expression value is verified if both arguments are verified.

Examples
--------

If ``A``, ``B``, and ``C`` are objects and ``in`` is a relation, then the output of the following expression:

.. code-block::

	iff(<: A in C -> B in C>, <: B in C -> A in C>)

Is the unverified sentence value:

.. code-block::

	A in C <-> B in C

If ``arg0`` is a variable storing the verified sentence value ``B in C -> A in C`` and ``arg1`` is a variable storing the verified sentence value ``A in C -> B in C``, then the expression:

.. code-block::

	iff(arg0, arg1)

Returns the verified sentence value:

.. code-block::

	B in C <-> A in C

