branch
======

The built-in function ``branch`` returns a sentence of the form ``C`` if ``A | B`` is true, ``C`` can be proven from ``A``, and ``C`` can be proven from ``B``.

Syntax
------

.. code-block::

	branch([sentence argument expression], [variable identifier], [variable identifier], ...){
		...
		return [return expression];
	} or {
		...
		return [return expression];
	} or ...
	}

Behavior
--------

``branch`` accepts one :ref:`sentence expression value <sentencevalue>` and two or more :doc:`variable identifiers <variables>`. The argument sentence must be verified, have a top-level ``|`` :doc:`connective <connectives>`, and not have an unbound object or predicate, otherwise an error is raised.

After the ``)`` character is one or more :doc:`scopes <scopes>`. Between consecutive scopes is ``or``. The number of scopes must be the same as the number of variable identifiers.

In the n-th scope, a variable with the n-th variable identifier is created.

Inside of the first scope, the argument sentence up to the first top-level ``|`` is stored as a verified sentence for the first variable identifier. At the end of the scope, a verified sentence must be returned. The return value of ``branch`` is the return value of the first scope.

For n > 1, inside of the n-th scope the argument sentence between the (n-1)-th top-level ``|`` and the n-th top-level ``|`` (or the end of the sentence if the scope is the last scope) is stored as a verified sentence for the n-th variable identifier. At the end of the scope, a verified sentence must be returned. If the return value of the scope is not :ref:`trivially equivalent <trivialequivalence>` to the return value of the first scope, an error is raised.

Examples
--------

Suppose ``=`` and ``in`` are relations, and that ``successor`` is a predicate such that ``successor(A, B)`` if and only if:

.. code-block::

	*X(X in A <-> X in B | X = B)

In other words, suppose the above sentence is the sentence definition of ``successor`` when ``A`` and ``B`` are considered unbound objects.

Suppose ``X``, ``A``, and ``B`` are objects, ``result`` is a predicate depending on one object, ``lem0`` stores the verified sentence:

.. code-block::

	*Y(Y in B -> result(B))

And ``lem1`` stores the verified sentence:

.. code-block::

	X = B -> result(B)

Finally, suppose ``arg0`` stores the verified sentence:

.. code-block::

	successor(A, B)

And ``arg1`` stores the verified sentence:

.. code-block::

	X in A

Then the following expression:

.. code-block::

	branch(expand(arg0)(X)(arg1), X_in_B, X_eq_B){
		return lem0(X)(X_in_B);
	} or {
		return lem1(X_eq_B);
	}

Evaluates to the verified sentence:

.. code-block::

	result(B)
