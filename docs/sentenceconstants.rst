Sentence Constants
==================

Sentence constants provide a way for the programmer to produce any :ref:`unverified sentence value <unverifiedvalue>` without proof. Sentence constants are most often used for :doc:`predicate substitution <predicatesubstitution>` of :ref:`axiom schema <axiomschema>` or :ref:`proof schema <proofschema>`.

Syntax
------

.. code-block::

	<[predicate identifier]([number of arguments]), ..., [object identifier], ... : [sentence]>

A sentence constant expression term begins with the character ``<``. What follows is a list of labels for unbound predicates and unbound objects. After this list is the character ``:``, and a sentence depending on the identifiers in the preceding list. The sentence constant expression is ended with the character ``>``. The list of unbound predicate and object labels may optionally be empty, in which case the ``:`` character directly follows the ``<`` character. 

An unbound predicate in the list of identifiers is denoted using a :ref:`predicate identifier <predicateidentifier>`, parentheses, and the number of arguments. If the number of arguments is zero, then the number of arguments may be omitted; however, in this case the parentheses must still be present, otherwise the identifier will be added as an unbound object instead of an unbound predicate.

An unbound object in the list of identifiers is denoted using an :ref:`object identifier <objectidentifier>`.

Unbound objects and predicates may be listed in any order, but the n-th unbound predicate of the output sentence value is the n-th listed unbound predicate. Likewise, the n-th unbound object of the output sentence value is the n-th listed unbound object.

The output is an :ref:`unverified sentence value <unverifiedvalue>` storing the given sentence with the given unbound predicates and unbound objects.

Examples
--------

If ``in`` is a :doc:`relation <relations>`, then the following expression:

.. code-block::

	<A, B: A in B | B in A>

Evaluates to an unverified sentence with two unbound objects ``A`` and ``B`` storing the sentence ``A in B | B in A``.

The following expression:

.. code-block::

	<P(1): ^X(P(X) & ~P(X))>

Evaluates to an unverified sentence with no unbound objects and one unbound predicate ``P(1)`` storing the sentence ``^X(P(X) & ~P(X))``. It does not matter that the sentence is clearly not true since the output of the expression is an *unverified* sentence value.

