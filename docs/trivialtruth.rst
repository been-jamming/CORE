Trivial Truth
=============

In order to balance convenience for the programmer with precision, CORE has a notion of when a sentence is *trivially true*

Trivially True
--------------

Suppose ``P`` is a sentence. To determine whether ``P`` is trivially true, search for the first applicable rule from the beginning in the following list:

#. If ``P`` is of the form ``A -> B`` for sentences ``A`` and ``B``, ``P`` is trivially true if ``A`` *trivially implies* ``B``.
#. If ``P`` is of the form ``A <-> B``, ``P`` is trivially true if ``A`` trivially implies ``B`` and ``B`` trivially implies ``A``
#. If ``P`` is of the form ``A & B``, ``P`` is trivially true if ``A`` is trivially true and ``B`` is trivially true.
#. If ``P`` is of the form ``A | B``, ``P`` is trivially true if ``A`` is trivially true or ``B`` is trivially true.
#. If ``P`` is of the form ``*X(A(X))``, ``P`` is trivially true if ``A`` is trivially true.
#. If ``P`` is of the form ``~A``, ``P`` is trivially true if ``A`` is trivially false.
#. If ``P`` is the sentence ``true``, ``P`` is trivially true.

One should interpret the trivial truth of a sentence with or without unbound variables to mean that no matter which objects are substituted for the unbound variables, the sentence can easily be prove true. A case for the trivial truth of a sentence beginning with an :ref:`existential quantifier <existential>` does not exist because not all models may have objects.

Trivially False
---------------

Suppose ``P`` is a sentence. To determine whether ``P`` is trivially false, search for the first applicable rule from the beginning in the following list:

#. If ``P`` is of the form ``A -> B`` for sentences ``A`` and ``B``, ``P`` is trivially false if ``A`` is trivially true and ``B`` is trivially false.
#. If ``P`` is of the form ``A <-> B``, ``P`` is trivially false if either ``A -> B`` or ``B -> A`` is trivially false.
#. If ``P`` is of the form ``A & B``, ``P`` is trivially false if either ``A`` is trivially false or ``B`` is trivially false.
#. If ``P`` is of the form ``A | B``, ``P`` is trivially false if ``A`` is trivially false and ``B`` is trivially false.
#. If ``P`` is of the form ``^X(A(X))`` or the form ``*X(A(X))``, ``P`` is trivially false if ``A`` is trivially false.
#. If ``P`` is of the form ``~A``, ``P`` is trivially false if ``A`` is trivially true.
#. If ``P`` is the sentence ``false``, ``P`` is trivially false.

One should interpret the trivial falsity of a sentence with or without unbound variables to mean that no matter which objects are substituted for the unbound variables, the sentence can easily be proven false.

.. _trivialimplication:

Trivial Implication
-------------------

Suppose ``P`` and ``Q`` are sentences. To determine whether ``P`` *trivially implies* ``Q``, search for the first applicable rule from the beginning of the following list. At most one rule is applied, so if a rule is applied and the rule does not give trivial implication, then ``P`` does not trivially imply ``Q``.

#. If ``P`` and ``Q`` have mismatching numbers of unbound variables or predicates, then ``P`` does not trivially imply ``Q``.
#. If ``P`` is of the form ``A | B``, ``P`` trivially implies ``Q`` if ``A`` trivially implies ``Q`` and ``B`` trivially implies ``Q``.
#. If ``Q`` is of the form ``A & B``, ``P`` trivially implies ``Q`` if ``P`` trivially implies ``A`` and ``P`` trivially implies ``B``.
#. If ``P`` is of the form ``~A`` and ``Q`` is of the form ``~B``, ``P`` trivially implies ``Q`` if ``B`` trivially implies ``A``.
#. If ``P`` is of the form ``A -> B`` and ``Q`` is of the form ``C -> D``, then ``P`` trivially implies ``Q`` if ``C`` trivially implies ``A`` and ``B`` trivially implies ``D``.
#. If ``P`` is of the form ``*X(A(X))`` and ``Q`` is of the form ``*X(B(X)``, then ``P`` trivially implies ``Q`` if ``A`` trivially implies ``B``.
#. If ``P`` is of the form ``^X(A(X))`` and ``Q`` is of the form ``^X(B(X))``, then ``P`` trivially implies ``Q`` if ``A`` trivially implies ``B``.
#. If ``P`` is of the form ``A <-> B`` and ``Q`` is of the form ``C <-> D``, then apply any of the following rules:
	- If ``A`` trivially implies ``C``, ``C`` trivially implies ``A``, ``B`` trivially implies ``D``, and ``D`` trivially implies ``B``, then ``P`` trivially implies ``Q``.
	- If ``A`` trivially implies ``D``, ``D`` trivially implies ``A``, ``B`` trivially implies ``C``, and ``C`` trivially implies ``B``, then ``P`` trivially implies ``Q``.
	- If ``C`` trivially implies ``D`` and ``D`` trivially implies ``C``, then ``P`` trivially implies ``Q``.
#. If ``Q`` is of the form ``A <-> B``, ``P`` trivially implies ``Q`` if ``A`` trivially implies ``B`` and ``B`` trivially implies ``A``.
#. If ``P`` and ``Q`` are identical :doc:`relation <relations>` :ref:`sentence terms <sentenceterm>`, then ``P`` trivially implies ``Q``.
#. If ``P`` and ``Q`` are identical :doc:`predicate <predicates>` :ref:`sentence terms <sentenceterm>`, then ``P`` trivially implies ``Q``.
#. If ``P`` is trivially false, then ``P`` trivially implies ``Q``.
#. If ``Q`` is trivially true, then ``P`` trivially implies ``Q``.
#. Otherwise, apply any of the following rules:
	- If ``P`` is of the form ``A & B`` and either ``A`` trivially implies ``Q`` or ``B`` trivially implies ``Q``, then ``P`` trivially implies ``Q``.
	- If ``Q`` is of the form ``A | B`` and either ``P`` trivially implies ``A`` or ``P`` trivially implies ``B``, then ``P`` trivially implies ``Q``.

Regardless of the number of unbound variables of ``A`` and ``B``, one should interpret "``A`` trivially implies ``B``" to mean that no matter which objects are substituted for the unbound variables, one can easily prove that ``A`` implies ``B``.

