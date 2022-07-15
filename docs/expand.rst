expand
======

The built-in function ``expand`` accepts one :ref:`sentence expression value <sentencevalue>` and uses the predicate or relation's corresponding :doc:`sentence definition <predicates>` to expand the sentence.

Syntax
------

.. code-block::

	expand([sentence argument expression])

Behavior
--------

The argument must be an expression evaluating to a :ref:`sentence expression value <sentencevalue>`. If the argument is not a top-level :doc:`predicate <predicates>` or :doc:`relation <relations>`, then an error is raised. If the argument has any unbound objects or predicates, an error is raised. If the predicate or relation has no sentence definition, then an error is raised.

If the argument is a top-level predicate, then the return value is a copy of the sentence definition with substituted unbound objects. The n-th unbound object of the sentence definition is substituted with the n-th argument of the predicate. The resulting sentence is returned.

If the argument is a top-level relation, then the return value is a copy of the sentence definition with the two unbound objects substituted. The first unbound object of the sentence definition is substituted with the first object related. The second unbound object of the sentence definition is substituted with the second object related. The resulting sentence is returned.

The returned sentence value is verified if and only if the argument sentence value is verified.

Examples
--------

Suppose ``in`` is a relation with no sentence definition and ``=`` is defined so that for objects ``X`` and ``Y``, ``X = Y`` if and only if the following sentence is true:

.. code-block::

	*Z(Z in X <-> Z in Y)

In other words, the above is the sentence definition of ``=`` when viewing ``X`` and ``Y`` as the first and second unbound objects.

Now suppose that ``NAT`` and ``W`` are two objects, and the variable ``equal`` stores the verified sentence:

.. code-block::

	NAT = W

Then the following expression:

.. code-block::

	expand(equal)

Evaluates to the following verified sentence:

.. code-block::

	*Z(Z in NAT <-> Z in W)

Now suppose ``successor`` is a predicate defined so that for any two objects ``X`` and ``Y``, ``successor(X, Y)`` if and only if:

.. code-block::

	*Z(Z in X <-> Z in Y | Z = Y)

In other words, the above is the sentence definition of ``successor`` when regarding ``X`` and ``Y`` as the first and second unbound objects.

If ``A`` and ``B`` are objects, then the following expression:

.. code-block::

	expand(<: successor(A, B)>)

Evaluates to the unverified sentence:

.. code-block::

	*Z(Z in A <-> Z in B | Z = B)

