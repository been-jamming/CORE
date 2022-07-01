Objects
=======

An object is an symbol which may be referenced by :doc:`predicates` or :doc:`relations`. Objects may be quantified, or created by the programmer. An object identifier must begin with a letter or and underscore, and may subsequently contain letters, digits, and underscores. By convention, object identifiers use only upper-case letters.

.. note::
	Objects are only labels. No internal information about an object is stored. Instead, :doc:`sentences` may describe objects.

Creation
--------

Objects may be created locally by :doc:`quantifiers`. Unquantified objects are created with either the :doc:`given` command or the :doc:`pipe` operator applied to a sentence beginning with an :ref:`existential` quantifier.

Examples
--------

The following are examples of *valid* object identifiers:

- ``NATURALS``
- ``X``
- ``_TEMP0``

The following are examples of *invalid* object identifiers:

- ``0_TEST``
- ``A<A``
- ``~OBJ~``

