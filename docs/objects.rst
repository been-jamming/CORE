Objects
=======

An object is an symbol which may be referenced by :doc:`predicates <predicates>` or :doc:`relations <relations>` in a :doc:`sentence <sentences>`. Objects may be quantified, or created by the programmer. 

An object identifier must begin with a letter or an underscore, and may subsequently contain letters, digits, and underscores. By convention, object identifiers use only upper-case letters.

In the default library of proofs, objects represent sets in `ZFC set theory`_. However, objects may represent anything the axioms describe.

.. note::
	Objects are only labels. No internal information about an object is stored. Instead, :doc:`sentences` may describe objects.

.. _`ZFC set theory`: https://en.wikipedia.org/wiki/Zermelo%E2%80%93Fraenkel_set_theory

Creation
--------

Objects may be created locally by :doc:`quantifiers <quantifiers>`. Unquantified objects are created with either the :doc:`given <given>` command or the :doc:`pipe <pipe>` operator applied to a sentence beginning with an :ref:`existential <existential>` quantifier.

Examples
--------

The following are examples of *valid* object identifiers:

- ``NATURALS``
- ``X``
- ``_temp0``

The following are examples of *invalid* object identifiers:

- ``0_TEST``
- ``A<A``
- ``~OBJ~``

