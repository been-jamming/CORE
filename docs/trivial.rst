trivial
=======

The built-in function ``trivial`` accepts one :ref:`sentence expression value <sentencevalue>`, and if the sentence is :doc:`trivially true <trivialtruth>`, returns the same sentence value except verified.

Syntax
------

.. code-block::

	trivial([sentence argument expression])

Behavior
--------

The argument must be an expression evaluating to a :ref:`sentence expression value <sentencevalue>` with no unbound objects or predicates. If the argument sentence is not :doc:`trivially true <trivialtruth>`, then an error is raised. Otherwise, the output is a verified sentence value storing the same sentence as the argument.

.. note::

	Anything that can be proven using the ``trivial`` built-in command can be proven without it. ``trivial`` is usually used to save the programmer the time of having to prove simple statements that are obviously true. Due to the limited scope of what is defined as trivially true, there are other sentences that are just as obvious but must be proven manually.

Examples
--------

If ``in`` is a relation, the following expression:

.. code-block::

	trivial(<: *X*Y(X in Y -> ~~X in Y)>)

Returns the following verified sentence value:

.. code-block::

	*X*Y(X in Y -> ~~X in Y)

