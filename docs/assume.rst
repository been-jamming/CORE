assume
======
The ``assume`` command creates a new scope with a modified goal. It is used when the goal in the current scope is an implication. The ``assume`` command represents the action of "assuming the premises" in a proof.

Syntax
------
.. code-block::

	assume [identifier0], [identifier1], ...;
	assume [identifier0], [identifier1], ...{
		...
	}

The ``assume`` command begins with the keyword ``assume``, followed by a comma-separated list of variable identifiers. The command is terminated with either the ``;`` character or a new scope.

Behavior
--------
When the ``assume`` command is used, the current goal must have a top-level implication, otherwise an error is raised. Suppose the goal is of the form ``A -> B`` for sentences ``A`` and ``B``. A new scope is created, and the contents of ``A`` are unpacked into the variables with identifiers given by the identifier list in the new scope. In the new scope, the goal is the sentence ``B``.

Starting from the first identifier, the left-most sentence in ``A`` connected using a top-level ``&`` is peeled from the sentence and stored in the identifier. This process is repeated for the new sentence and each of the following identifiers. The remainder of the sentence is stored for the last variable. This unpacking process is the same as that for :doc:`assignment <assignment>`.

If the commands in the new scope are verified successfully and the goal is *proven*, then the goal is proven in the parent scope. No commands can follow the ``assume`` command in the parent scope.

If ``assume`` command is terminated with a ``;`` character, the new scope and parent scope end with the next unmatched ``}`` character. Otherwise, only the new scope ends with the next numatched ``}`` character.

Examples
--------

Using ``assume`` with the :doc:`return <return>` command, some simple proofs can be showcased. The following examples are valid:

.. code-block::

	prove implies_transitive[P, Q, R]: (P -> Q) & (Q -> R) -> (P -> R){
		assume P_implies_Q, Q_implies_R;
		assume P_true;
		return Q_implies_R(P_implies_Q(P_true));
	}

After execution, the sentence ``(P -> Q) & (Q -> R) -> (P -> R)`` depending on unbound predicates ``P``, ``Q``, and ``R`` is stored in the variable ``implies transitive`` in the example above.

.. code-block::

	relation in;
	object Z;
	object Y;
	object X;
	axiom transitive_Z: *V*W(V in Z & W in V -> W in Z);

	prove X_in_Z_if: X in Y & Y in Z -> X in Z{
		assume X_in_Y, Y_in_Z;
		return transitive_Z(Y, X)(X_in_Y, Y_in_Z);
	}

After execution, the sentence ``X in Z`` is stored in the variable ``X_in_Z`` in the example above.
