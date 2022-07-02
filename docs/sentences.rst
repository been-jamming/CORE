Sentences
=========

Sentences in first order `intuitionistic`_ logic are the main data type of expressions in CORE. In order to understand how the language works, it's important to understand how to read and interpret these sentences.

.. _`intuitionistic`: https://en.wikipedia.org/wiki/Intuitionistic_logic

.. _`sentenceterm`:

A *sentence term* is composed of either a :doc:`constant <constants>`, :doc:`predicate <predicates>`, :doc:`relation <relations>`, :doc:`quantifier <quantifiers>`, :ref:`negation <negation>`, or parentheses term. A sentence term is an example of a sentence. Sentences can be constructed from several sentence terms connected using the binary :doc:`logical connectives <connectives>`.

The ``(`` character marks the beginning of a parentheses term. The rest of the parentheses term is parsed as the sentence up to a closing ``)`` character. Parentheses terms may also be nested. Parentheses are used to pick the association of terms in a sentence, which is useful for overriding :ref:`connective precedence` as in the following example:

.. code-block::

	P(Z) & *X*Y(P(X) & P(Y) -> X = Y) -> *X(P(X) <-> X = Z)

Sentences may have any number of *unbound predicates* and *unbound variables*. These sentences have conditional truth values; the truth of the sentence depends on what propositions or variables are substituted.

