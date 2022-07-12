or
===

The built-in function ``or`` accepts one or more :ref:`sentence expression values <sentencevalue>` and returns a sentence which links each argument sentence using the ``|`` :ref:`connective <or>`.

Syntax
------

.. code-block::

	or([sentence argument expression], ...)

Behavior
--------

``or`` accepts one or more argument expressions. If any argument expression does not evaluate to a :ref:`sentence expression value <sentencevalue>` or any argument sentence has unbound objects or predicates, then an error is raised. Otherwise, the output is a sentence composed of each argument sentence joined in order using the ``|`` :ref:`connective <and>`.

The output sentence value is verified if at least one argument sentence value is verified.

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

	or(arg0, arg1)

Evaluates to the following verified sentence:

.. code-block::

	*X*Y(~X in Y | ~Y in X) | ~^X(X in X)

Additionally, the following expression:

.. code-block::

	or(arg0, <:^X^Y(X in Y & Y in X)>, <:^E*X(~X in E)>)

Evaluates to the following verified sentence:

.. code-block::

	*X*Y(~X in Y | Y in X) | ^X^Y(X in Y & Y in X) | ^E*X(~X in E)
