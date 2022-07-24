object
======

The ``object`` command is used to assert that an object exists. 

Syntax
------

.. code-block::

	object [object identifier];
	object [object identifier], [object identifier], ...;

The object identifier begins with the keyword ``object`` followed by one or more comma-separated :ref:`object identifiers <objectidentifier>`. The ``object`` command is terminated with a ``;`` character.

Behavior
--------

For each object identifier, an :doc:`object <objects>` is created in the current scope with the corresponding object identifier. For each such object, an :ref:`object dependency <objectdependency>` is created in the current scope.

Because the ``object`` command creates :doc:`dependencies <dependencies>`, the ``object`` command may only be used in a :ref:`global scope <globalscope>` or a :ref:`dependent scope <dependentscope>`.

Examples
--------

In the default proofs library, the program ``naturals.core`` creates two objects ``NATURALS`` and ``ZERO`` using the commands:

.. code-block::

	object NATURALS;
	object ZERO;

Which can be condensed into the single command:

.. code-block::

	object NATURALS, ZERO;

