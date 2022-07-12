and
===

The built-in function ``and`` accepts one or more :ref:`sentence expression values <sentencevalue>` and returns a sentence which links each argument sentence using the ``&`` :ref:`connective <and>`.

Syntax
------

.. code-block::

	and([sentence argument expression], ...)

Behavior
--------

``and`` accepts one or more argument expressions. If any argument expression does not evaluate to a :ref:`sentence expression value <sentencevalue>` or any argument sentence has unbound objects or predicates, then an error is raised. Otherwise, the output is a sentence composed of each argument sentence joined in order using the ``&`` :ref:`connective <and>`.

The output sentence value is verified if every argument sentence value is verified.

Examples
--------

If ``in`` is a :doc:`relation <relations>`, ``arg0`` stores the verified sentence:

.. code-block::

	*X*Y(~X in Y | ~Y in X)

And ``arg1`` stores the verified sentence:

.. code-block::

	~^X(X in X)

Then the following expression:

.. code-block::

	and(arg0, arg1)

Evaluates to the following verified sentence:

.. code-block::

	*X*Y(~X in Y | ~Y in X) & ~^X(X in X)

Additionally, the following expression:

.. code-block::

	and(arg0, <:^X^Y(X in Y & Y in X)>, <:^E*X(~X in E)>)

Evaluates to the following unverified sentence:

.. code-block::

	*X*Y(~X in Y | Y in X) & ^X^Y(X in Y & Y in X) & ^E*X(~X in E)
