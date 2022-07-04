Deduction
=========

Deduction is applied to :ref:`sentence values <sentencevalue>` containing a top level :ref:`implication <implication>` to prove a conclusion given a premise.

Syntax
------

.. code-block::

	[expression value]([expression value], [expression value], ...)

A deduction sentence term begins with a *parent expression* evaluating to a :ref:`sentence value <sentencevalue>`. Following the parent expression is the character ``(``, one or more argument expressions evaluating to sentence values, and the character ``)``. The parent expression sentence value must begin with a top level :ref:`implies <implication>`, :ref:`biconditional <biconditional>`, or :ref:`not <not>` connective.

Each of the argument sentence values are linked together using the :ref:`and <and>` connective. The resulting sentence will be referred to as the *premise sentence*

If the parent expression sentence value is :ref:`verified <sentencevalue>` and every argument sentence value is verified, then the deduction sentence term evaluates to a verified sentence value. Otherwise, it evaluates to an :ref:`unverified sentence value<unverifiedvalue>`.

Implication Deduction
---------------------

If the parent expression sentence is of the form ``A -> B`` for sentences ``A`` and ``B``, then it is checked that the premise sentence :ref:`trivially implies <trivialimplication>` the sentence ``A``. If not, an error is raised. The deduction sentence term evaluates to the sentence value ``B``.

Biconditional Deduction
-----------------------

If the parent expression sentence is of the form ``A <-> B`` for sentences ``A`` and ``B``, then it is checked that the premise sentence either :ref:`trivially implies <trivialimplication>` ``A`` or ``B``. If not, an error is raised. If the premise sentence implies ``A``, then the deduction sentence term evaluates to the sentence ``B``, otherwise it evaluates to the sentence value ``A``.

Negation Deduction
------------------

If the parent expression sentence is of the form ``~A`` for the sentence ``A``, then it is checked that the premise sentence :ref:`trivially implies <trivialimplication>` ``A``. If not, an error is raised. The deduction sentence term evaluates to the sentence ``false``.

Examples
--------

Suppose ``in`` is a relation; ``A``, ``B``, ``C``, and ``D`` are objects; and ``union_D`` is a sentence variable storing the following sentence value:

.. code-block::

	A in B & B in C -> A in D

If ``A_in_B`` is a sentence variable storing ``A in B`` and ``B_in_C`` is a sentence variable storing ``B in C``, then the expression ``union_D(A_in_B, B_in_C)`` evaluates to the sentence value ``A in D``. 

Suppose ``equal`` is a sentence variable storing the following sentence value:

.. code-block::

	A in C | B in C -> A = B

If ``B_in_C`` is a sentence variable storing ``B in C``, then the expression ``equal(B_in_C)`` evaluates to the sentence value ``A = B``.

