Variables
=========

CORE has two types of variables: sentence variables and context variables. Variables share the same namespace as :doc:`objects <objects>`, so a variable and an object can't share the same name in the same scope.

.. _`sentencevariable`:

Sentence Variables
------------------

Sentence variables store either verified or unverified sentence expression values. Expressions which refer to a sentence variable by name evaluate to that variable's stored sentence value. For example, if ``result`` is a sentence variable, then ``result`` is an expression evaluating to the sentence value stored by the variable ``result``. Sentence variables stored in the current scope can be overwritten at any point in the current scope. When the current scope is destroyed, the sentence variables stored in the scope are destroyed with it.

Variable identifiers may begin with either a letter or an underscore, and may subsequently contain letters, digits, and underscores.

Context Variables
-----------------

Context variables store the contents of a scope created with the :doc:`context command <context>`. Sentence variables, :doc:`objects <objects>`, :doc:`definitions <define>`, and :doc:`relations <relation>` stored inside of a context scope can be referenced inside of sentence literals and expressions using the ``.`` operator. For example, if ``con`` is a context and ``name`` is a result, object, definition, or relation inside of the context, then ``con.name`` refers to it. Context variables can store scopes with more context variables. For instance, if ``con`` stores a context variable ``con2``, and ``con2`` stores ``name``, then this variable can be referenced as ``con.con2.name``.

Context variables cannot be overwritten. The scopes stored by a context variable are immutable.
