Variable Assignment
===================

Variable assignment stores one or more :ref:`sentence values <sentencevalue>` in :ref:`sentence variables <sentencevariable>`. Sentences connected using the :ref:`and <and>` connective can be split and stored using variable assignment.

Syntax
------

.. code-block::

	[identifier0], [identifier1], ... = [expression];

Variable assignment begins with a comma-separated list of one or more variable identifiers. This is followed with the ``=`` character and an expression. The command is terminated using the ``;`` character.

Behavior
--------

If there are n comma-separated variable identifiers, ``[expression]`` must evaluate to a :ref:`sentence value <sentencevalue>` which is n or more sentences connected with a top-level ``&`` connective, otherwise an error is raised.

Starting from the first identifier, the left-most sentence in ``[expression]`` connected using a top-level ``&`` is peeled from the sentence and stored in the identifier. This process is repeated for the new sentence and each of the following identifiers. The remainder of the sentence is stored for the last variable.

Examples
--------

In the following example, an object ``NAT`` is created and an axiom which claims that no object is ``in`` itself. This axiom is applied using :doc:`object substitution <objectsubstitution>` with ``NAT``, and the resulting sentence is *stored* in ``result`` for later use.

.. code-block::

	relation in;
	object NAT;
	axiom recursive_member: *X(~X in X);
	result = recursive_member(NAT);

The following example demonstrates how variable assignment can be used to unpack two or more sentences which are important on their own:

.. code-block::

	relation in;
	object A, B;
	axiom pair_member: *X*Y(~X in Y & ~Y in X);
	A_not_in_B, B_not_in_A = pair_member(A, B);

After execution, ``A_not_in_B`` stores the sentence ``~A in B``, and ``B_not_in_A`` stores the sentence ``~B in A``.

