#include "predicate.h"

int sentence_trivially_true(sentence *s);
int sentence_trivially_false(sentence *s);
int sentence_stronger(sentence *s0, sentence *s1);
int sentence_equivalent(sentence *s0, sentence *s1);

