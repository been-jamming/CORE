Expressions
===========

Expressions allow the programmer to combine expression values to obtain new expression values. Expression operations include deduction rules.

.. toctree::
	:titlesonly:

	Expression Values <self>
	Variables <variables>
	Object Substitution <objectsubstitution>
	Pipe Operator <pipe>
	Deduction <deduction>
	Sentence Constants <sentenceconstants>
	Predicate Definitions <predicatedefinitions>
	Expression Brackets <brackets>
	Built-in Functions <builtin>

.. _`expressionvalues`:

Expression Values
-----------------

Expressions evaluate to expression values. There are three types of expression values: *verified sentences*, *unverified sentences*, and *object values*.

.. _`sentencevalue`:

Verified sentences represent results that the programmer has proven to be true. Verified sentences can generally only be :doc:`deduced <deduction>` from other verified sentences. Verified sentences may have unbound predicates (see :ref:`Axiom Schema <axiomschema>` and :ref:`Proof Schema <proofschema>`), but will never have unbound variables.

.. _`unverifiedvalue`:

Unverified sentences are generally substituted as a predicate in a sentence with unbound predicates. An expression value containing any unverified sentence can be created using :doc:`sentence constants <sentenceconstants>`.

.. _`objectvalue`:

Object expression values are used for :doc:`object substitution <objectsubstitution>`. These expression values only store a reference to an object. The object refered by an object expression value will never be an object bound by a quantifier in a sentence. An object expression value is created by referring to the object by name. For example, if an object named ``OBJ`` belongs to a :ref:`context <contextcommand>` called ``con``, then the expression ``con.OBJ`` evaluates to an object expression value referencing that object.

