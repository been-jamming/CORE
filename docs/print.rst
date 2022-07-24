print
=====

The ``print`` command allows the programmer to display an :ref:`expression value <expressionvalues>` in the console. The command is often used for debugging while developing a proof.

Syntax
------

.. code-block::

	print [expression];

The ``print`` command begins with the keyword ``print`` followed by a ``;`` terminated expression. 

Behavior
--------

If ``[expression]`` evaluates to an :ref:`object value <objectvalue>`, then the following is printed in the console:

.. code-block::

	'[file name]' line [line number]: [object identifier]

Where ``[object identifier]`` is the identifier of the object referred to by the object value.

If ``[expression]`` evaluates to a :ref:`verified sentence value <sentencevalue>`, then the following is printed in the console:

.. code-block::

	'[file name]' line [line number]: (verified)   [sentence]

Where ``[sentence]`` is the sentence referred to by the sentence value.

If ``[expression]`` evaluates to a :ref:`unverified sentence value <sentencevalue>`, then the following is printed in the console:

.. code-block::

	'[file name]' line [line number]: (unverified) [sentence]

Where ``[sentence]`` is the sentence referred to by the sentence value.

When a sentence is printed by the ``print`` command, the sentence is printed in *normal form*. In normal form, each quantified or unbound object is replaced with numbers 0, 1, 2, etc. The nth unbound object is always renamed to the object n in the printed sentence value. Then the remaining quantified variables' identifiers are replaced with the next available number.

Additionally, unbound predicates in normal form are denoted in order by ``P0``, ``P1``, ``P2``, etc.

Examples
--------

The following command:

.. code-block::

	print trivial(<: true>);

Prints the following line in the console:

.. code-block::

	'[file name]' line [line number]: (verified)   true

The following command:

.. code-block::

	print <Q(3), Z: *X*Y(Q(X, Y, Z) <-> Q(Z, X, Y))>

Prints the following line in the console:

.. code-block::

	'[file name]' line [line number]: (unverified) *1*2(P0(1, 2, 0) <-> P0(0, 1, 2))

