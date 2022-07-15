Predicate Definitions
=====================

Predicate definitions can be used to obtain verified sentences. If a :doc:`predicate <predicates>` or :doc:`relation <relations>` has a sentence definition, then by definition the predicate or relation applied to objects makes a true sentence when the corresponding sentence definition with those objects substituted is true.

Syntax
------

.. code-block::

	#[identifier]

A predicate definition expression term begins with a ``#`` character followed by an identifier. The identifier may be either a :doc:`predicate identifier <predicates>` or a :doc:`relation identifier <relations>`.

Behavior
--------

There must be a predicate or relation which is accessible from the current :doc:`scope <scopes>` and shares the same identifier, otherwise an error is raised. If the predicate or relation does not have a sentence definition, then an error is raised.

If a predicate ``P`` depending on n objects shares the same identifier, denote the sentence definition as ``A(X1, X2, ..., Xn)`` for unbound objects ``X1``, ``X2``,..., ``Xn``. The output of the expression is a verified sentence of the following form:

.. code-block::

	*X1*X2*[...]*Xn(A(X1, X2,..., Xn) <-> P(X1, X2,..., Xn))

If a relation ``R`` shares the same identifier, denote the sentence definition as ``A(X1, X2)`` for unbound objects ``X1`` and ``X2``. The output of the expression is a verified sentence of the following form:

.. code-block::

	*X1*X2(A(X1, X2) <-> X1 R X2)

Examples
--------

Suppose ``in`` is a relation and ``=`` is a relation such that for unbound objects ``A`` and ``B``, ``A = B`` if and only if:

.. code-block::

	*X(X in A <-> X in B)

In other words, the above sentence is the sentence definition for ``=`` where ``A`` and ``B`` are the first and second unbound objects.

Suppose now that ``X``, ``A``, and ``B`` are objects, ``eq`` is a variable storing the following verified sentence:

.. code-block::

	A = B

And ``member`` is a variable storing the following verified sentence:

.. code-block::

	X in A

Then the following expression:

.. code-block::

	#=(A, B)(eq)(X)(member)

Evaluates to the verified sentence:

.. code-block::

	X in B

If ``successor`` is a predicate depending on two objects such that for unbound objects ``C`` and ``D``, ``successor(C, D)`` if and only if:

.. code-block::

	*Y(Y in C <-> Y in D | Y = D)

Then the following expression:

.. code-block::

	#successor

Evaluates to the following verified sentence:

.. code-block::

	*C*D(*Y(Y in C <-> Y in D | Y = D) <-> successor(C, D))

