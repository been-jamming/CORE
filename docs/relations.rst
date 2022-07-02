Relations
=========

A relation is a symbol which may reference exactly two :doc:`objects <objects>` in a :doc:`sentence <sentences>`. Functionally, relations serve the same role as :doc:`predicates <predicates>`. The advantage of a relation is that it provides a more convenient notation for concepts such as equality, set membership, and subsets.

Relations may or may not have a *sentence definition*. If a relation does have a sentence definition, the sentence has two unbound variables sharing the order in which the objects would appear when related by the relation.

Syntax
------

A relation's identifier may appear between two object identifiers in a sentence:

.. code-block::

	[object identifier] [relation identifier] [object identifier]

A sentence formed like the one above creates a :ref:`sentence term<sentenceterm>`.

Relation identifiers may either begin with a letter, underscore, or a character which is not one of ``;``, ``(``, or ``)``. If a relation begins with a letter or underscore, the subsequent characters of a relation identifier must be either letters, underscores, or digits. Otherwise, the subsequent characters must be any character which is not a letter, underscore, whitespace, ``;``, ``(``, or ``)``.

Creation
--------

Relations may be created in any scope using the :doc:`relation <relation>` command.

Examples
--------

The following are valid relation identifiers:

- ``in``
- ``subset``
- ``=``
- ``<=``
- ``<69``

While the following are invalid relation identifiers:

- ``hi<=``
- ``0=``
- ``< =``

Here are some example sentence terms and the corresponding object identifiers and relation identifiers:

- ``A<B`` object identifiers: ``A`` and ``B``, relation identifier: ``<``
- ``X_SET in C`` object identifiers: ``X_SET`` and ``C``, relation identifier: ``in``

