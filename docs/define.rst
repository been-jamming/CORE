define
======

The ``define`` command creates a new definition in the current :doc:`scope <scopes>` and a :doc:`predicate <predicates>` which may be referred to by :doc:`sentences <sentences>` in the current or children scopes.

Syntax
------

.. code-block::

	define [definition name]: [sentence definition];
	define [definition name]([arg0], [arg1], ...): [sentence definition];
	define [definition name];
	define [definition name]([arg0], [arg1], ...);
	dependent define [definition name];
	dependent define [definition name]([arg0], [arg1], ...);
	dependent define [definition name]([arg0], [arg1], ...): [sentence definition];

The command may optionally begin with the ``dependent`` keyword before the ``define`` keyword. The following identifier must be a valid :ref:`predicate identifier <predicateidentifier>`. An optional list of :ref:`object identifiers <objectidentifier>` may follow the definition name. The ``;`` character may terminate the command, or ``:`` marks the start of a ``;`` terminated sentence.

Behavior
--------

The list of object identifiers is a list of unbound objects in the sentence definition, if present. The number of arguments the predicate depends on is the number of identifiers in the list of object identifiers. If no list of object identifiers is given, the predicate created depends on no arguments. If ``[sentence definition]`` is present, then it is the sentence definition of the predicate created, otherwise the predicate has no sentence definition.

If the ``dependent`` keyword is present or ``[sentence definition]`` is not present, then the definition is added as a :ref:`definition dependency <definitiondependency>`. In these cases, the command must be called in a :ref:`global scope <globalscope>` or a :ref:`dependent scope <dependentscope>`.

Examples
--------

Suppose ``in`` is a :doc:`relation <relations>`. Then consider the following commands:

.. code-block::

	object NATURALS;
	define successor(A, B): *X(X in A -> X in B | X = B);
	axiom successor_natural: *N*M(M in NATURALS & successor(N, M) -> N in NATURALS);

An object ``NATURALS`` is created as well as a definition ``successor``. The sentence ``successor(A, B)`` should be interpreted as "``A`` is the successor of ``B``." Then the axiom ``successor_natural`` says that the successor of a member of ``NATURALS`` is a member of ``NATURALS``.

Now consider the following command:

.. code-block::

	dependent define related(A, B);

The command creates a definition ``related`` which has two arguments but no sentence definition. When used in a program which is :doc:`imported <import>`, it *asserts* that a definition with the identifier ``related`` exists in the importing program, but does not enforce any requirements for such a definition. Such a command is useful when one wants to assert that a definition which behaves in a certain way (specified by :doc:`axioms <axiom>`) exists but does not care how the definition is constructed.

