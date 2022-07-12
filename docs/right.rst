right
=====

The built-in function ``right`` accepts one :ref:`sentence expression value <sentencevalue>` and returns the right-hand side of the sentence. It's precise behavior depends on the input value.

Syntax
------

.. code-block::

	right([sentence argument expression])

Behavior
--------

If the argument does not evaluate to a sentence expression value or the argument sentence has unbound objects or propositions, an error is raised.

The following lists the behavior of the function depending on the argument sentence type:

- If the argument sentence is of the form ``A & B``, then the output is the sentence ``B``. The output is verified if and only if the argument sentence is verified.
- If the argument sentence is of the form ``A | B``, then the output is the sentence ``B``. The output is unverified.
- If the argument sentence is of the form ``A -> B``, then the output is the sentence ``B``. The output is unverified.
- If the argument sentence is of the form ``A <-> B``, then the output is the sentence ``A -> B``. The output is verified if and only if the argument sentence is verified.
- If the argument sentence is not any of these types, an error is raised

.. note::
	Since this function's output depends on the association of the operations composing the argument sentence which normally doesn't matter, this function's use is not recommended. Oftentimes this function's purpose is overshadowed by :ref:`trivial implication <trivialimplication>` and :doc:`variable assignment <assign>`.

Examples
--------

If ``X`` and ``Y`` are objects, ``<`` is a relation, the output of the following expression:

.. code-block::

	right(<: X < Y & Y < X>)

Is the unverified sentence:

.. code-block::

	Y < X

If ``X`` is an object, ``empty`` is a :doc:`definition <define>`, ``in`` is a :doc:`relation <relations>`, and ``test`` stores the verified sentence:

.. code-block::

	^E(empty(E) & E in X) & ~X in X

Then the output of the following expression:

.. code-block::

	right(test)

Is the verified sentence:

.. code-block::

	~X in X

