Scopes
======

Scopes are environments in which commands run. Scopes are used to separate the program's states. For example, :doc:`variable assignment <assignment>` in one scope can't overwrite variables inside of another scope.

Syntax
------

.. code-block::

	{
		[one or more commands]
	}

.. code-block::

	{
		[one or more commands]
		return [expression];
	}

Each scope begins with a ``{`` character and ends with a ``}`` character. Inside each scope is one or more commands, most of which must be terminated with a ``;`` character. Optionally, a scope may end with a :doc:`return <return>` command.

Behavior
--------

Each scope creates a new *namespace* for variables, objects, definitions, and relations. 

Scopes can be created inside of scopes, in which case the new scope becomes a *child scope* of the current scope. The *parent scopes* of the child scope include the current scope and any of the current scope's parent scopes. A variable, object, definition, or relation is accessible inside of a scope if and only if that entity is in the current namespace or any parent scope's namespace.

All variables, objects, definitions, and relations in a scopes namespace are destroyed at the end of the scope except in the case in which the scope was created by the :doc:`context <context>` command.

.. _`globalscope`:

*Global scopes* are the top-level scopes created at the beginning of program execution. Dependencies can be added within a global scope.

.. _`dependentscope`:

*Dependent scopes* are child scopes created by the ``dependent context`` command. Besides global scopes, dependent scopes are the only scopes in which dependencies can be added.
