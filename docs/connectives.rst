Connectives
===========

Logical connectives create new :doc:`sentences <sentences>` from two or more constituent sentences. There are five logical connectives in CORE: ``~``, ``&``, ``|``, ``->``, and ``<->``.

.. _negation:

Negation
--------

The logical connective ``~`` represents negation. Unlike the other four connectives, this is a unary connective. If ``A`` represents a well formed sentence, then ``~A`` is a well formed sentence. The sentence ``~A`` should be interpreted to mean that ``A`` is not true.

Sentences composed with the ``~`` connective can be proven using the :doc:`not <not>` command. A sentence of the form ``~A`` is logically equivalent to the sentence ``A -> false``. Given ``A`` and ``~A``, one can :ref:`deduce <deduction>` ``false``.

.. note::
	Since CORE uses a kind of `intuitionistic logic`_, it is not true in general that ``A | ~A`` for any sentence of the form ``A``. Additionally, ``~~A`` is generally weaker than the sentence ``A``. One can model classical logic by adding the law of excluded middle as an :doc:`axiom <axiom>`.

.. _`intuitionistic logic`: https://en.wikipedia.org/wiki/Intuitionistic_logic

And
---

The logical connective ``&`` represents "and." If ``{A}`` and ``{B}`` represent well formed sentences, then ``{A} & {B}`` is a well formed sentence. ``{A} & {B}`` is interpreted to be true if and only if ``{A}`` is true and ``{B}`` is true.

Sentences composed with the ``&`` logical connective can be constructed using the built-in function :doc:`and <andfunc>`. A sentence composed with the ``&`` connective can be deconstructed into its constituent sentences using :doc:`variable assignment <varassign>`. A sentence ``{C}`` is stronger than the sentence ``{A} & {B}`` if ``{C}`` is stronger than both ``{A}`` and ``{B}``. The sentence ``{A} & {B}`` is stronger than a sentence ``{C}`` if either ``{A}`` or ``{B}`` is stonger than ``{C}``. See :ref:`Trivial Implication <trivialimp>` for more information.

Or
---

The logical connective ``|`` represents "or." If ``{A}`` and ``{B}`` represent well formed sentences, then ``{A} | {B}`` is a well formed sentence. ``{A} | {B}`` is interpreted to be true if and only if ``{A}`` is true or ``{B}`` is true.

Sentences composed with the ``|`` logical connective can be constructed using the built-in function :doc:`or <orfunc>`. A sentence composed with the ``|`` connective can be used in a proof to :doc:`branch <branchfunc>` an argument. A sentence ``{C}`` is stronger than the sentence ``{A} | {B}`` if ``{C}`` is stronger than either ``{A}`` or ``{B}``. The sentence ``{A} | {B}`` is stronger than a sentence ``{C}`` if both ``{A}`` and ``{B}`` are stronger than ``{C}``. See :ref:`Trivial Implication <trivialimp>` for more information.

Implies
-------

The logical connective ``->`` represents implication. If ``{A}`` and ``{B}`` represent well formed sentences, then ``{A} -> {B}`` is a well formed sentence. ``{A} -> {B}`` is interpreted to be true if and only if whenever one can prove ``{A}``, one can prove ``{B}``.

Verified sentences ``{A} -> {B}`` and ``{A}`` can be used to :ref:`deduce <deduction>` the verified sentence ``{B}``.

Biconditional
-------------

The logical connective ``<->`` represents the biconditonal connective. If ``{A}`` and ``{B}`` represent well formed sentences, then ``{A} <-> {B}`` is a well formed sentences. ``{A} <-> {B}`` is logically equivalent to ``({A} -> {B}) & ({B} -> {A})``.

Sentences composed with the ``<->`` logical connective can be constructed from implications using the built-in function :doc:`iff <ifffunc>`. Verified sentences ``{A} <-> {B}`` and ``{A}`` can be used to :ref:`deduce <deduction>` the verified sentence ``{B}``. In addition, verified sentences ``{A} <-> {B}`` and ``{B}`` can be used to :ref:`deduce <deduction>` ``{A}``.

.. _`connective precedence`:

Connective Precedence
---------------------

Connectives are associated according to precedence. Connectives are in the following list in order from highest precedence to lowest precedence:

#. ``&``
#. ``|``
#. ``->``
#. ``<->``

For example, the sentence

.. code-block::

	A | B & C <-> D -> E

Is parsed as:

.. code-block::

	(A | (B & C)) <-> (D -> E)

Due to connective precedence.

