not
===
The ``not`` command creates a new scope with a modified goal. It is used when the goal in the current scope is a top-level negation. The ``not`` command represents the action of "assuming the premise" and "proving a contradiction" in a proof.

If the current goal is of the form ``~A`` for a sentence ``A``, using the ``not`` command operates as if the :doc:`assume <assume>` command was used with a goal of ``A -> false``.

Syntax
------
.. code-block::

	not [identifier];
	not [identifier]{
		...
	}

The ``not`` command begins with the keyword ``not``, followed by a variable identifier. The command is terminated with either the ``;`` character or a new scope.

Behavior
--------
When the ``not`` command is used, the current goal must have a top-level negation, otherwise an error is raised. Suppose the goal is of the form ``~A`` for the sentence ``A``. A new scope is created, and the contents of ``A`` are stored into the variable with identifier ``[identifier]``. In the new scope, the goal is the sentence ``false``. Proving ``false`` in the new scope represents finding a contradiction.

If the commands in the new scope are verified successfully and the goal is proven, then the goal is proven in the parent scope. No commands can follow the ``not`` command in the parent scope.

If ``not`` command is terminated with a ``;`` character, the new scope and parent scope end with the next unmatched ``}`` character. Otherwise, only the new scope ends with the next unmatched ``}`` character.

Examples
--------

The following examples showcase the ``not`` command:

.. code-block::

	prove demorgan0[P, Q]: ~P | ~Q -> ~(P & Q){
		assume one_false;
		not P_and_Q;
		return branch(one_false, not_P, not_Q){
			return not_P(P_and_Q);
		} or {
			return not_Q(P_and_Q);
		};
	}

	prove demorgan1[P, Q]: ~P & ~Q -> ~(P | Q){
		assume not_P, not_Q;
		not P_or_Q;
		return branch(P_or_Q, P_true, Q_true){
			return not_P(P_true);
		} or {
			return not_Q(Q_true);
		};
	}

	prove demorgan2[P, Q]: ~(P | Q) -> ~P & ~Q{
		assume not_either;
		prove not_P: ~P{
			not P_true;
			return not_either(P_true);
		}
		prove not_Q: ~Q{
			not Q_true;
			return not_either(Q_true);
		}
		return not_P, not_Q;
	}

.. note::

	The remaining version of De Morgan's law (``~(P & Q) -> ~P | ~Q``) cannot be proven without assuming the law of excluded middle.
