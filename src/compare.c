#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "predicate.h"
#include "compare.h"

#ifndef QUANTIFIER_INCREMENT
#define QUANTIFIER_INCREMENT 4
#endif

#ifndef QUANTIFIER_DEFAULT
#define QUANTIFIER_DEFAULT 4
#endif

#ifndef QUANTIFIER_STACK_INCREMENT
#define QUANTIFIER_STACK_INCREMENT 4
#endif

#ifndef QUANTIFIER_STACK_DEFAULT
#define QUANTIFIER_STACK_DEFAULT 4
#endif

enum map_status{
	UNMAPPED,
	BOUND_VAR,
	POINTER
};

struct map_entry{
	enum map_status status;
	union{
		int var_id;
		variable *var;
	};
};

//-1 means not mapped
static struct map_entry **forall_map;
static struct map_entry **exists_map;
static int *forall_range_lower;
static int *forall_range_upper;
static int *exists_range_lower;
static int *exists_range_upper;
//Tells whether the quantifiers have been mapped in any stack frame
static unsigned char *forall_mapped;
static unsigned char *exists_mapped;
static unsigned int num_quantifiers = 0;
static unsigned int max_quantifiers = QUANTIFIER_DEFAULT;
static unsigned int stack_size = 1;
static unsigned int max_stack = QUANTIFIER_STACK_DEFAULT;

static int sentence_stronger_recursive(sentence *s0, sentence *s1, int reversed);
static int sentence_equivalent_recursive(sentence *s0, sentence *s1, int reversed);

void init_quantifier_map(){
	int i;
	int j;

	forall_map = malloc(sizeof(struct map_entry *)*max_stack);
	exists_map = malloc(sizeof(struct map_entry *)*max_stack);
	for(i = 0; i < max_stack; i++){
		forall_map[i] = malloc(sizeof(struct map_entry)*max_quantifiers);
		exists_map[i] = malloc(sizeof(struct map_entry)*max_quantifiers);
		for(j = 0; j < max_quantifiers; j++){
			forall_map[i][j].status = UNMAPPED;
			exists_map[i][j].status = UNMAPPED;
		}
	}

	forall_range_lower = malloc(sizeof(int)*max_quantifiers);
	forall_range_upper = malloc(sizeof(int)*max_quantifiers);
	forall_mapped = malloc(sizeof(unsigned char)*max_quantifiers);
	exists_mapped = malloc(sizeof(unsigned char)*max_quantifiers);
	exists_range_lower = malloc(sizeof(int)*max_quantifiers);
	exists_range_upper = malloc(sizeof(int)*max_quantifiers);

	for(i = 0; i < max_quantifiers; i++){
		forall_range_lower[i] = -1;
		forall_range_upper[i] = -1;
		forall_mapped[i] = 0;
		exists_range_lower[i] = -1;
		exists_range_upper[i] = -1;
		exists_mapped[i] = 0;
	}
}

static void trivialize_quantifiers(int num_vars){
	int i;

	for(i = 0; i < num_vars; i++){
		forall_map[stack_size - 1][i].status = BOUND_VAR;
		forall_map[stack_size - 1][i].var_id = i;
		exists_map[stack_size - 1][i].status = BOUND_VAR;
		exists_map[stack_size - 1][i].var_id = i;
	}

	for(i = num_vars; i < max_quantifiers; i++){
		forall_map[stack_size - 1][i].status = UNMAPPED;
		exists_map[stack_size - 1][i].status = UNMAPPED;
	}
}

static void resize_quantifiers(unsigned int new_max_quantifiers){
	int i;
	int j;

	for(i = 0; i < max_stack; i++){
		forall_map[i] = realloc(forall_map[i], sizeof(int)*new_max_quantifiers);
		exists_map[i] = realloc(exists_map[i], sizeof(int)*new_max_quantifiers);
		for(j = max_quantifiers; j < new_max_quantifiers; j++){
			forall_map[i][j].status = UNMAPPED;
			exists_map[i][j].status = UNMAPPED;
		}
	}

	forall_range_lower = realloc(forall_range_lower, sizeof(int)*new_max_quantifiers);
	forall_range_upper = realloc(forall_range_upper, sizeof(int)*new_max_quantifiers);
	forall_mapped = realloc(forall_mapped, sizeof(unsigned char)*new_max_quantifiers);
	exists_range_lower = realloc(exists_range_lower, sizeof(int)*new_max_quantifiers);
	exists_range_upper = realloc(exists_range_upper, sizeof(int)*new_max_quantifiers);
	exists_mapped = realloc(exists_mapped, sizeof(unsigned char)*new_max_quantifiers);

	for(i = max_quantifiers; i < new_max_quantifiers; i++){
		forall_range_lower[i] = -1;
		forall_range_upper[i] = -1;
		forall_mapped[i] = 0;
		exists_range_lower[i] = -1;
		exists_range_upper[i] = -1;
		exists_mapped[i] = 0;
	}

	max_quantifiers = new_max_quantifiers;
}

static void resize_stack(unsigned int new_max_stack){
	int i;
	int j;

	forall_map = realloc(forall_map, sizeof(int *)*new_max_stack);
	exists_map = realloc(exists_map, sizeof(int *)*new_max_stack);
	for(i = max_stack; i < new_max_stack; i++){
		forall_map[i] = malloc(sizeof(int)*max_quantifiers);
		exists_map[i] = malloc(sizeof(int)*max_quantifiers);
		for(j = 0; j < max_quantifiers; j++){
			forall_map[i][j].status = UNMAPPED;
			exists_map[i][j].status = UNMAPPED;
		}
	}

	max_stack = new_max_stack;
}

static void add_forall(unsigned int increment, int lower, int upper){
	int i;

	num_quantifiers += increment;

	if(num_quantifiers > max_quantifiers){
		resize_quantifiers(num_quantifiers + QUANTIFIER_INCREMENT - num_quantifiers%QUANTIFIER_INCREMENT);
	}

	for(i = num_quantifiers - increment; i < num_quantifiers; i++){
		forall_range_lower[i] = lower;
		forall_range_upper[i] = upper;
		forall_map[stack_size - 1][i].status = UNMAPPED;
		forall_mapped[i] = 0;
	}
}

static void add_exists(unsigned int increment, int lower, int upper){
	int i;

	num_quantifiers += increment;

	if(num_quantifiers > max_quantifiers){
		resize_quantifiers(num_quantifiers + QUANTIFIER_INCREMENT - num_quantifiers%QUANTIFIER_INCREMENT);
	}

	for(i = num_quantifiers - increment; i < num_quantifiers; i++){
		exists_range_lower[i] = lower;
		exists_range_upper[i] = upper;
		exists_map[stack_size - 1][i].status = UNMAPPED;
		exists_mapped[i] = 0;
	}
}

static void remove_forall(unsigned int decrement){
	int i;

	num_quantifiers -= decrement;

	for(i = num_quantifiers; i < num_quantifiers + decrement; i++){
		forall_range_lower[i] = -1;
		forall_range_upper[i] = -1;
	}
}

static void remove_exists(unsigned int decrement){
	int i;

	num_quantifiers -= decrement;

	for(i = num_quantifiers; i < num_quantifiers + decrement; i++){
		exists_range_lower[i] = -1;
		exists_range_upper[i] = -1;
	}
}

/*
static struct map_entry bound_forall(unsigned int index){
	return forall_map[stack_size - 1][index];
}

static struct map_entry bound_exists(unsigned int index){
	return exists_map[stack_size - 1][index];
}
*/

static void save_quantifiers(){
	stack_size++;

	if(stack_size > max_stack){
		resize_stack(max_stack + QUANTIFIER_STACK_INCREMENT);
	}

	memcpy(forall_map[stack_size - 1], forall_map[stack_size - 2], sizeof(struct map_entry)*max_quantifiers);
	memcpy(exists_map[stack_size - 1], exists_map[stack_size - 2], sizeof(struct map_entry)*max_quantifiers);
}

static void restore_quantifiers(){
	stack_size--;
}

static int check_arguments(struct map_entry entry0, struct map_entry entry1, int reversed){
	struct map_entry *s0_entry;
	struct map_entry *s1_entry;
	unsigned char *s0_mapped;
	unsigned char *s1_mapped;
	int s0_range_lower;
	int s0_range_upper;
	int s1_range_lower;
	int s1_range_upper;

	if(entry0.status == BOUND_VAR){
		if(reversed){
			s0_entry = exists_map[stack_size - 1] + entry0.var_id;
			s0_mapped = exists_mapped + entry0.var_id;
			s0_range_lower = exists_range_lower[entry0.var_id];
			s0_range_upper = exists_range_upper[entry0.var_id];
		} else {
			s0_entry = forall_map[stack_size - 1] + entry0.var_id;
			s0_mapped = forall_mapped + entry0.var_id;
			s0_range_lower = forall_range_lower[entry0.var_id];
			s0_range_upper = forall_range_upper[entry0.var_id];
		}
	}

	if(entry1.status == BOUND_VAR){
		if(reversed){
			s1_entry = forall_map[stack_size - 1] + entry1.var_id;
			s1_mapped = forall_mapped + entry1.var_id;
			s1_range_lower = forall_range_lower[entry1.var_id];
			s1_range_upper = forall_range_upper[entry1.var_id];
		} else {
			s1_entry = exists_map[stack_size - 1] + entry1.var_id;
			s1_mapped = exists_mapped + entry1.var_id;
			s1_range_lower = exists_range_lower[entry1.var_id];
			s1_range_upper = exists_range_lower[entry1.var_id];
		}
	}

	if(entry0.status == BOUND_VAR && entry1.status == POINTER){
		if(s0_entry->status == UNMAPPED){
			s0_entry->status = POINTER;
			s0_entry->var = entry1.var;
			*s0_mapped = 1;
		} else if(s0_entry->status == POINTER && s0_entry->var != entry1.var){
			return 0;
		} else if(s0_entry->status == BOUND_VAR){
			return 0;
		}
	} else if(entry0.status == BOUND_VAR && entry1.status == BOUND_VAR){
		if(s0_entry->status == UNMAPPED && entry1.var_id >= s0_range_lower && entry1.var_id <= s0_range_upper){
			s0_entry->status = BOUND_VAR;
			s0_entry->var_id = entry1.var_id;
			*s0_mapped = 1;
		} else if(s1_entry->status == UNMAPPED && entry0.var_id >= s1_range_lower && entry0.var_id <= s1_range_upper){
			s1_entry->status = BOUND_VAR;
			s1_entry->var_id = entry0.var_id;
			*s1_mapped = 1;
		} else if(s0_entry->status == BOUND_VAR && s0_entry->var_id == entry1.var_id){
			return 1;
		} else if(s1_entry->status == BOUND_VAR && s1_entry->var_id == entry0.var_id){
			return 1;
		} else {
			return 0;
		}
	} else if(entry0.status == POINTER && entry1.status == BOUND_VAR){
		if(s1_entry->status == UNMAPPED){
			s1_entry->status = POINTER;
			s1_entry->var = entry0.var;
			*s1_mapped = 1;
		} else if(s1_entry->status == POINTER && s1_entry->var != entry0.var){
			return 0;
		} else if(s1_entry->status == BOUND_VAR){
			return 0;
		}
	} else if(entry0.status == POINTER && entry1.status == POINTER && entry0.var != entry1.var){
		return 0;
	}

	return 1;
}

static int stronger_premise_or(sentence *s0, sentence *s1, int reversed){
	return sentence_stronger_recursive(s0->child0, s1, reversed) && sentence_stronger_recursive(s0->child1, s1, reversed);
}

static int stronger_conclusion_and(sentence *s0, sentence *s1, int reversed){
	return sentence_stronger_recursive(s0, s1->child0, reversed) && sentence_stronger_recursive(s0, s1->child1, reversed);
}

static int stronger_premise_and(sentence *s0, sentence *s1, int reversed){
	save_quantifiers();
	if(sentence_stronger_recursive(s0->child0, s1, reversed)){
		restore_quantifiers();
		return 1;
	}
	restore_quantifiers();

	save_quantifiers();
	if(sentence_stronger_recursive(s0->child1, s1, reversed)){
		restore_quantifiers();
		return 1;
	}
	restore_quantifiers();

	return 0;
}

static int stronger_conclusion_or(sentence *s0, sentence *s1, int reversed){
	save_quantifiers();
	if(sentence_stronger_recursive(s0, s1->child0, reversed)){
		restore_quantifiers();
		return 1;
	}
	restore_quantifiers();

	save_quantifiers();
	if(sentence_stronger_recursive(s0, s1->child1, reversed)){
		restore_quantifiers();
		return 1;
	}
	restore_quantifiers();

	return 0;
}

static int stronger_implies(sentence *s0, sentence *s1, int reversed){
	return sentence_stronger_recursive(s1->child0, s0->child0, !reversed) && sentence_stronger_recursive(s0->child1, s1->child1, reversed);
}

static int stronger_forall(sentence *s0, sentence *s1, int reversed){
	int quantifiers_before;
	int quantifiers_after;
	int output;
	int i;
	sentence *child1;

	quantifiers_before = s1->num_bound_vars;
	if(s1->type == FORALL){
		quantifiers_after = s1->child0->num_bound_vars;
		child1 = s1->child0;
	} else {
		quantifiers_after = s1->num_bound_vars;
		child1 = s1;
	}

	if(reversed){
		add_exists(s0->child0->num_bound_vars - s0->num_bound_vars, quantifiers_before, quantifiers_after - 1);
	} else {
		add_forall(s0->child0->num_bound_vars - s0->num_bound_vars, quantifiers_before, quantifiers_after - 1);
	}
	output = sentence_stronger_recursive(s0->child0, child1, reversed);
	for(i = s0->num_bound_vars; i < s0->child0->num_bound_vars; i++){
		//A forall statement can never imply an existence statement
		//This is because there may be no objects in the model
		if(reversed && !exists_mapped[i]){
			output = 0;
		} else if(!reversed && !forall_mapped[i]){
			output = 0;
		}
	}
	if(reversed){
		remove_exists(s0->child0->num_bound_vars - s0->num_bound_vars);
	} else {
		remove_forall(s0->child0->num_bound_vars - s0->num_bound_vars);
	}

	return output;
}

static int stronger_exists(sentence *s0, sentence *s1, int reversed){
	int quantifiers_before;
	int quantifiers_after;
	int output;
	sentence *child0;

	quantifiers_before = s0->num_bound_vars;
	if(s0->type == EXISTS){
		quantifiers_after = s0->child0->num_bound_vars;
		child0 = s0->child0;
	} else {
		quantifiers_after = s0->num_bound_vars;
		child0 = s0;
	}

	if(reversed){
		add_forall(s1->child0->num_bound_vars - s1->num_bound_vars, quantifiers_before, quantifiers_after - 1);
	} else {
		add_exists(s1->child0->num_bound_vars - s1->num_bound_vars, quantifiers_before, quantifiers_after - 1);
	}
	output = sentence_stronger_recursive(child0, s1->child0, reversed);
	if(reversed){
		remove_forall(s1->child0->num_bound_vars - s1->num_bound_vars);
	} else {
		remove_exists(s1->child0->num_bound_vars - s1->num_bound_vars);
	}

	return output;
}

static int stronger_bicond_both(sentence *s0, sentence *s1, int reversed){
	save_quantifiers();
	if(sentence_equivalent_recursive(s0->child0, s1->child0, reversed) && sentence_equivalent_recursive(s0->child1, s1->child1, reversed)){
		restore_quantifiers();
		return 1;
	}
	restore_quantifiers();

	save_quantifiers();
	if(sentence_equivalent_recursive(s0->child0, s1->child1, reversed) && sentence_equivalent_recursive(s0->child1, s1->child0, reversed)){
		restore_quantifiers();
		return 1;
	}
	restore_quantifiers();

	save_quantifiers();
	trivialize_quantifiers(s1->num_bound_vars);
	if(sentence_equivalent_recursive(s1->child0, s1->child1, 0)){
		restore_quantifiers();
		return 1;
	}
	restore_quantifiers();

	return 0;
}

static int stronger_bicond_conclusion(sentence *s0, sentence *s1, int reversed){
	save_quantifiers();
	trivialize_quantifiers(s1->num_bound_vars);
	if(sentence_equivalent_recursive(s1->child0, s1->child1, 0)){
		restore_quantifiers();
		return 1;
	}
	restore_quantifiers();
	return 0;
}

static int stronger_relation(sentence *s0, sentence *s1, int reversed){
	struct map_entry s0_entry;
	struct map_entry s1_entry;

	if(s0->relation_data != s1->relation_data){
		return 0;
	}

	if(s0->is_bound0){
		s0_entry.status = BOUND_VAR;
		s0_entry.var_id = s0->var0_id;
	} else {
		s0_entry.status = POINTER;
		s0_entry.var = s0->var0;
	}

	if(s1->is_bound0){
		s1_entry.status = BOUND_VAR;
		s1_entry.var_id = s1->var0_id;
	} else {
		s1_entry.status = POINTER;
		s1_entry.var = s1->var0;
	}

	if(!check_arguments(s0_entry, s1_entry, reversed)){
		return 0;
	}

	if(s0->is_bound1){
		s0_entry.status = BOUND_VAR;
		s0_entry.var_id = s0->var1_id;
	} else {
		s0_entry.status = POINTER;
		s0_entry.var = s0->var1;
	}

	if(s1->is_bound1){
		s1_entry.status = BOUND_VAR;
		s1_entry.var_id = s1->var1_id;
	} else {
		s1_entry.status = POINTER;
		s1_entry.var = s1->var1;
	}

	return check_arguments(s0_entry, s1_entry, reversed);
}

static int stronger_proposition(sentence *s0, sentence *s1, int reversed){
	struct map_entry s0_entry;
	struct map_entry s1_entry;
	int i;

	if(s0->is_bound != s1->is_bound){
		return 0;
	}
	if(s0->is_bound && s0->definition_id != s1->definition_id){
		return 0;
	} else if(!s0->is_bound && s0->definition_data != s1->definition_data){
		return 0;
	}
	if(s0->num_args != s1->num_args){
		return 0;
	}

	for(i = 0; i < s0->num_args; i++){
		if(s0->proposition_args[i].is_bound){
			s0_entry.status = BOUND_VAR;
			s0_entry.var_id = s0->proposition_args[i].var_id;
		} else {
			s0_entry.status = POINTER;
			s0_entry.var = s0->proposition_args[i].var;
		}

		if(s1->proposition_args[i].is_bound){
			s1_entry.status = BOUND_VAR;
			s1_entry.var_id = s1->proposition_args[i].var_id;
		} else {
			s1_entry.status = POINTER;
			s1_entry.var = s1->proposition_args[i].var;
		}

		if(!check_arguments(s0_entry, s1_entry, reversed)){
			return 0;
		}
	}

	return 1;
}

static int sentence_equivalent_recursive(sentence *s0, sentence *s1, int reversed){
	return sentence_stronger_recursive(s0, s1, reversed) && sentence_stronger_recursive(s1, s0, !reversed);
}

int sentence_trivially_true(sentence *s){
	if(s->type == IMPLIES){
		save_quantifiers();
		trivialize_quantifiers(s->num_bound_vars);
		if(sentence_stronger_recursive(s->child0, s->child1, 0)){
			restore_quantifiers();
			return 1;
		}
		restore_quantifiers();
		return 0;
	} else if(s->type == BICOND){
		save_quantifiers();
		trivialize_quantifiers(s->num_bound_vars);
		if(sentence_equivalent_recursive(s->child0, s->child1, 0)){
			restore_quantifiers();
			return 1;
		}
		restore_quantifiers();
		return 0;
	} else if(s->type == AND){
		return sentence_trivially_true(s->child0) && sentence_trivially_true(s->child1);
	} else if(s->type == OR){
		return sentence_trivially_true(s->child0) || sentence_trivially_true(s->child1);
	} else if(s->type == FORALL){
		return sentence_trivially_true(s->child0);
	} else if(s->type == NOT){
		return sentence_trivially_false(s->child0);
	} else if(s->type == TRUE){
		return 1;
	} else {
		return 0;
	}
}

int sentence_trivially_false(sentence *s){
	int true0;
	int true1;
	int false0;
	int false1;

	if(s->type == IMPLIES){
		return sentence_trivially_true(s->child0) && sentence_trivially_false(s->child1);
	} else if(s->type == BICOND){
		true0 = sentence_trivially_true(s->child0);
		true1 = sentence_trivially_true(s->child1);
		false0 = sentence_trivially_false(s->child0);
		false1 = sentence_trivially_false(s->child1);
		return (true0 && false1) || (false0 && true1);
	} else if(s->type == AND){
		return sentence_trivially_false(s->child0) || sentence_trivially_false(s->child1);
	} else if(s->type == OR){
		return sentence_trivially_false(s->child0) && sentence_trivially_false(s->child1);
	} else if(s->type == FORALL || s->type == EXISTS){
		return sentence_trivially_false(s->child0);
	} else if(s->type == NOT){
		return sentence_trivially_true(s->child0);
	} else if(s->type == FALSE){
		return 1;
	} else {
		return 0;
	}
}

static int sentence_stronger_recursive(sentence *s0, sentence *s1, int reversed){
	if(s0->num_bound_vars != s1->num_bound_vars || s0->num_bound_props != s1->num_bound_props){
		return 0;
	} else if(s0->type == OR){
		return stronger_premise_or(s0, s1, reversed);
	} else if(s1->type == AND){
		return stronger_conclusion_and(s0, s1, reversed);
	} else if(sentence_trivially_false(s0)){
		return 1;
	} else if(sentence_trivially_true(s1)){
		return 1;
	} else if(s0->type == NOT && s1->type == NOT){
		return sentence_stronger_recursive(s1->child0, s0->child0, !reversed);
	} else if(s1->type == NOT && s1->child0->type == NOT){
		return sentence_stronger_recursive(s0, s1->child0->child0, reversed);
	} else if(s0->type == IMPLIES && s1->type == IMPLIES){
		return stronger_implies(s0, s1, reversed);
	//Think about changing this later so that only s0 has to be forall
	//problem happens on line 247 of zfc.core
	} else if(s0->type == FORALL && s1->type == FORALL){
		return stronger_forall(s0, s1, reversed);
	} else if(s0->type == EXISTS && s1->type == EXISTS){
		return stronger_exists(s0, s1, reversed);
	} else if(s0->type == BICOND && s1->type == BICOND){
		return stronger_bicond_both(s0, s1, reversed);
	} else if(s1->type == BICOND){
		return stronger_bicond_conclusion(s0, s1, reversed);
	} else if(s0->type == RELATION && s1->type == RELATION){
		return stronger_relation(s0, s1, reversed);
	} else if(s0->type == PROPOSITION && s1->type == PROPOSITION){
		return stronger_proposition(s0, s1, reversed);
	} else {
		if(s0->type == AND && stronger_premise_and(s0, s1, reversed)){
			return 1;
		}
		if(s1->type == OR && stronger_conclusion_or(s0, s1, reversed)){
			return 1;
		}
		return 0;
	}
}

int sentence_stronger(sentence *s0, sentence *s1){
	return sentence_stronger_recursive(s0, s1, 0);
}

int sentence_equivalent(sentence *s0, sentence *s1){
	return sentence_stronger(s0, s1) && sentence_stronger(s1, s0);
}

