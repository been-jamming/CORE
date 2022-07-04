Sentence Constants
==================

Sentence constants provide a way for the programmer to produce any :ref:`unverified sentence value <unverifiedvalue>` without proof. Sentence constants are most often used for :doc:`predicate substitution <predicatesubstitution>` of :ref:`axiom schema <axiomschema>` or :ref:`proof schema <proofschema>`.

Syntax
------

.. code-block::

	<[predicate identifier]([number of arguments]), ..., [object identifier], ... : [sentence]>

A sentence constant expression term begins with the character ``<``. What follows is a list of labels for unbound predicates and unbound objects. After this list is the character ``:``, and a sentence depending on the identifiers in the preceding list. The sentence constant expression is ended with the character ``>``

