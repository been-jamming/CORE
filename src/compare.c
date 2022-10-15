#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "custom_malloc.h"
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
	//The variable cannot be mapped
	RESERVED,
	//The variable has not yet been mapped
	UNMAPPED,
	//The variable is mapped to a bound variable
	BOUND_VAR,
	//The variable is mapped to an object
	POINTER
};

struct map_entry{
	enum map_status status;
	union{
		struct{
			int range_lower;
			int range_upper;
		};
		int var_id;
		variable *var;
	};
};

static struct map_entry **s0_map;
static struct map_entry **s1_map;
static unsigned int max_quantifiers = QUANTIFIER_DEFAULT;
static unsigned int stack_size = 1;
static unsigned int max_stack = QUANTIFIER_STACK_DEFAULT;

static int sentence_stronger_recursive(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map);
static int sentence_equivalent_recursive(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map);

static int sentence_stronger_exact(sentence *s0, sentence *s1);
static int sentence_equivalent_exact(sentence *s0, sentence *s1);
static int sentence_trivially_true_exact(sentence *s0);
static int sentence_trivially_false_exact(sentence *s1);

extern int debug_flag;

void init_quantifier_map(){
	int i;

	s0_map = malloc(sizeof(struct map_entry *)*max_stack);
	s1_map = malloc(sizeof(struct map_entry *)*max_stack);
	for(i = 0; i < max_stack; i++){
		s0_map[i] = malloc(sizeof(struct map_entry)*max_quantifiers);
		s1_map[i] = malloc(sizeof(struct map_entry)*max_quantifiers);
	}
}

void deinit_quantifier_map(){
	int i;

	for(i = 0; i < max_stack; i++){
		free(s0_map[i]);
		free(s1_map[i]);
	}
	free(s0_map);
	free(s1_map);
}

static void resize_quantifiers(unsigned int new_max_quantifiers){
	int i;

	for(i = 0; i < max_stack; i++){
		s0_map[i] = realloc(s0_map[i], sizeof(struct map_entry)*new_max_quantifiers);
		s1_map[i] = realloc(s1_map[i], sizeof(struct map_entry)*new_max_quantifiers);
	}

	max_quantifiers = new_max_quantifiers;
}

static void trivialize_quantifiers(struct map_entry ***map, int num_vars){
	int i;

	if(num_vars > max_quantifiers){
		resize_quantifiers(num_vars + QUANTIFIER_INCREMENT - num_vars%QUANTIFIER_INCREMENT);
	}

	for(i = 0; i < num_vars; i++){
		(*map)[stack_size - 1][i].status = BOUND_VAR;
		(*map)[stack_size - 1][i].var_id = i;
	}
}

static void resize_stack(unsigned int new_max_stack){
	int i;

	s0_map = realloc(s0_map, sizeof(struct map_entry *)*new_max_stack);
	s1_map = realloc(s1_map, sizeof(struct map_entry *)*new_max_stack);
	for(i = max_stack; i < new_max_stack; i++){
		s0_map[i] = malloc(sizeof(struct map_entry)*max_quantifiers);
		s1_map[i] = malloc(sizeof(struct map_entry)*max_quantifiers);
	}

	max_stack = new_max_stack;
}

static void add_quantifiers(struct map_entry ***source_map, struct map_entry ***dest_map, int source_lower, int source_upper, int dest_lower, int dest_upper){
	int i;
	int max;

	if(source_upper > dest_upper){
		max = source_upper;
	} else {
		max = dest_upper;
	}

	if(max >= max_quantifiers){
		resize_quantifiers(max + QUANTIFIER_INCREMENT - max%QUANTIFIER_INCREMENT);
	}

	for(i = source_lower; i <= source_upper; i++){
		(*source_map)[stack_size - 1][i].status = UNMAPPED;
		(*source_map)[stack_size - 1][i].range_lower = dest_lower;
		(*source_map)[stack_size - 1][i].range_upper = dest_upper;
	}

	for(i = dest_lower; i <= dest_upper; i++){
		(*dest_map)[stack_size - 1][i].status = RESERVED;
	}
}

static void save_quantifiers(){
	stack_size++;

	if(stack_size >= max_stack){
		resize_stack(max_stack + QUANTIFIER_STACK_INCREMENT);
	}

	memcpy(s0_map[stack_size - 1], s0_map[stack_size - 2], sizeof(struct map_entry)*max_quantifiers);
	memcpy(s1_map[stack_size - 1], s1_map[stack_size - 2], sizeof(struct map_entry)*max_quantifiers);
}

static void restore_quantifiers(){
	stack_size--;
}

//Copies the current quantifiers into the previous stack frame
static void copy_quantifiers(){
	memcpy(s0_map[stack_size - 2], s0_map[stack_size - 1], sizeof(struct map_entry)*max_quantifiers);
	memcpy(s1_map[stack_size - 2], s1_map[stack_size - 1], sizeof(struct map_entry)*max_quantifiers);
}

//Checks whether entry0 can be mapped to entry1
static int check_arguments(struct map_entry entry0, struct map_entry entry1, struct map_entry ***source_map){
	struct map_entry *source_entry;

	if(entry0.status == BOUND_VAR){
		source_entry = (*source_map)[stack_size - 1] + entry0.var_id;
	}

	if(entry0.status == BOUND_VAR && entry1.status == POINTER){
		if(source_entry->status == UNMAPPED){
			source_entry->status = POINTER;
			source_entry->var = entry1.var;
			return 1;
		} else if(source_entry->status == POINTER && source_entry->var == entry1.var){
			return 1;
		} else {
			return 0;
		}
	} else if(entry0.status == BOUND_VAR && entry1.status == BOUND_VAR){
		if(source_entry->status == UNMAPPED && entry1.var_id >= source_entry->range_lower && entry1.var_id <= source_entry->range_upper){
			source_entry->status = BOUND_VAR;
			source_entry->var_id = entry1.var_id;
			return 1;
		} else if(source_entry->status == BOUND_VAR && source_entry->var_id == entry1.var_id){
			return 1;
		} else {
			return 0;
		}
	} else if(entry0.status == POINTER && entry1.status == POINTER && entry0.var == entry1.var){
		return 1;
	} else {
		return 0;
	}
}

static int stronger_premise_or(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map){
	return sentence_stronger_recursive(s0->child0, s1, source_map, dest_map) && sentence_stronger_recursive(s0->child1, s1, source_map, dest_map);
}

static int stronger_conclusion_and(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map){
	return sentence_stronger_recursive(s0, s1->child0, source_map, dest_map) && sentence_stronger_recursive(s0, s1->child1, source_map, dest_map);
}

static int stronger_premise_and(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map){
	save_quantifiers();
	if(sentence_stronger_recursive(s0->child0, s1, source_map, dest_map)){
		copy_quantifiers();
		return 1;
	}
	restore_quantifiers();

	save_quantifiers();
	if(sentence_stronger_recursive(s0->child1, s1, source_map, dest_map)){
		copy_quantifiers();
		return 1;
	}
	restore_quantifiers();

	return 0;
}

static int stronger_conclusion_or(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map){
	save_quantifiers();
	if(sentence_stronger_recursive(s0, s1->child0, source_map, dest_map)){
		copy_quantifiers();
		return 1;
	}
	restore_quantifiers();

	save_quantifiers();
	if(sentence_stronger_recursive(s0, s1->child1, source_map, dest_map)){
		copy_quantifiers();
		return 1;
	}
	restore_quantifiers();

	return 0;
}

static int stronger_implies(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map){
	return sentence_stronger_recursive(s1->child0, s0->child0, dest_map, source_map) && sentence_stronger_recursive(s0->child1, s1->child1, source_map, dest_map);
}

static int stronger_forall(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map){
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

	add_quantifiers(source_map, dest_map, s0->num_bound_vars, s0->child0->num_bound_vars - 1, quantifiers_before, quantifiers_after - 1);

	output = sentence_stronger_recursive(s0->child0, child1, source_map, dest_map);

	for(i = s0->num_bound_vars; i < s0->child0->num_bound_vars; i++){
		//The quantifiers must have been bound
		if((*source_map)[stack_size - 1][i].status == UNMAPPED){
			output = 0;
			break;
		}
	}

	return output;
}

static int stronger_exists(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map){
	int quantifiers_before;
	int quantifiers_after;
	int output;
	int i;
	sentence *child0;

	quantifiers_before = s0->num_bound_vars;
	if(s0->type == EXISTS){
		quantifiers_after = s0->child0->num_bound_vars;
		child0 = s0->child0;
	} else {
		quantifiers_after = s0->num_bound_vars;
		child0 = s0;
	}

	add_quantifiers(dest_map, source_map, s1->num_bound_vars, s1->child0->num_bound_vars - 1, quantifiers_before, quantifiers_after - 1);

	output = sentence_stronger_recursive(child0, s1->child0, source_map, dest_map);

	for(i = s1->num_bound_vars; i < s1->child0->num_bound_vars; i++){
		if((*dest_map)[stack_size - 1][i].status == UNMAPPED){
			output = 0;
			break;
		}
	}

	return output;
}

static int stronger_bicond_both(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map){
	save_quantifiers();
	if(sentence_equivalent_recursive(s0->child0, s1->child0, source_map, dest_map) && sentence_equivalent_recursive(s0->child1, s1->child1, source_map, dest_map)){
		copy_quantifiers();
		return 1;
	}
	restore_quantifiers();

	save_quantifiers();
	if(sentence_equivalent_recursive(s0->child0, s1->child1, source_map, dest_map) && sentence_equivalent_recursive(s0->child1, s1->child0, source_map, dest_map)){
		copy_quantifiers();
		return 1;
	}
	restore_quantifiers();

	save_quantifiers();
	trivialize_quantifiers(source_map, s1->num_bound_vars);
	trivialize_quantifiers(dest_map, s1->num_bound_vars);
	if(sentence_equivalent_recursive(s1->child0, s1->child1, source_map, dest_map)){
		restore_quantifiers();
		return 1;
	}
	restore_quantifiers();

	return 0;
}

static int stronger_bicond_conclusion(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map){
	save_quantifiers();
	trivialize_quantifiers(source_map, s1->num_bound_vars);
	trivialize_quantifiers(dest_map, s1->num_bound_vars);
	if(sentence_equivalent_recursive(s1->child0, s1->child1, source_map, dest_map)){
		restore_quantifiers();
		return 1;
	}
	restore_quantifiers();

	return 0;
}

static int stronger_relation(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map){
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

	if(!check_arguments(s0_entry, s1_entry, source_map) && !check_arguments(s1_entry, s0_entry, dest_map)){
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

	return check_arguments(s0_entry, s1_entry, source_map) || check_arguments(s1_entry, s0_entry, dest_map);
}

static int stronger_proposition(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map){
	struct map_entry s0_entry;
	struct map_entry s1_entry;
	int i;

	if(s0->is_bound != s1->is_bound){
		return 0;
	}
	if(s0->is_bound && s0->definition_id != s1->definition_id){
		return 0;
	}
	if(!s0->is_bound && s0->definition_data != s1->definition_data){
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
			s1_entry.status = POINTER;;
			s1_entry.var = s1->proposition_args[i].var;
		}

		if(!check_arguments(s0_entry, s1_entry, source_map) && !check_arguments(s1_entry, s0_entry, dest_map)){
			return 0;
		}
	}

	return 1;
}

static int sentence_equivalent_recursive(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map){
	return sentence_stronger_recursive(s0, s1, source_map, dest_map) && sentence_stronger_recursive(s1, s0, dest_map, source_map);
}

int sentence_trivially_true(sentence *s){
	if(s->type == IMPLIES){
		save_quantifiers();
		trivialize_quantifiers(&s0_map, s->num_bound_vars);
		trivialize_quantifiers(&s1_map, s->num_bound_vars);
		if(sentence_stronger_recursive(s->child0, s->child1, &s0_map, &s1_map)){
			restore_quantifiers();
			return 1;
		}
		restore_quantifiers();
		return 0;
	} else if(s->type == BICOND){
		save_quantifiers();
		trivialize_quantifiers(&s0_map, s->num_bound_vars);
		trivialize_quantifiers(&s1_map, s->num_bound_vars);
		if(sentence_equivalent_recursive(s->child0, s->child1, &s0_map, &s1_map)){
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

static int sentence_stronger_recursive(sentence *s0, sentence *s1, struct map_entry ***source_map, struct map_entry ***dest_map){
	if(s0->type == OR){
		return stronger_premise_or(s0, s1, source_map, dest_map);
	} else if(s1->type == AND){
		return stronger_conclusion_and(s0, s1, source_map, dest_map);
	} else if(sentence_trivially_false(s0)){
		return 1;
	} else if(sentence_trivially_true(s1)){
		return 1;
	} else if(s0->type == NOT && s1->type == NOT){
		return sentence_stronger_recursive(s1->child0, s0->child0, dest_map, source_map);
	} else if(s1->type == NOT && s1->child0->type == NOT){
		return sentence_stronger_recursive(s0, s1->child0->child0, source_map, dest_map);
	} else if(s0->type == IMPLIES && s1->type == IMPLIES){
		return stronger_implies(s0, s1, source_map, dest_map);
	} else if(s0->type == FORALL){
		return stronger_forall(s0, s1, source_map, dest_map);
	} else if(s1->type == EXISTS){
		return stronger_exists(s0, s1, source_map, dest_map);
	} else if(s0->type == BICOND && s1->type == BICOND){
		return stronger_bicond_both(s0, s1, source_map, dest_map);
	} else if(s1->type == BICOND){
		return stronger_bicond_conclusion(s0, s1, source_map, dest_map);
	} else if(s0->type == RELATION && s1->type == RELATION){
		return stronger_relation(s0, s1, source_map, dest_map);
	} else if(s0->type == PROPOSITION && s1->type == PROPOSITION){
		return stronger_proposition(s0, s1, source_map, dest_map);
	} else {
		if(s0->type == AND && stronger_premise_and(s0, s1, source_map, dest_map)){
			return 1;
		}
		if(s1->type == OR && stronger_conclusion_or(s0, s1, source_map, dest_map)){
			return 1;
		}
		return 0;
	}
}

int sentence_stronger(sentence *s0, sentence *s1){
	if(s0->num_bound_vars != s1->num_bound_vars || s0->num_bound_props != s1->num_bound_props){
		return 0;
	}

	trivialize_quantifiers(&s0_map, s0->num_bound_vars);
	trivialize_quantifiers(&s1_map, s1->num_bound_vars);
	return sentence_stronger_exact(s0, s1) || sentence_stronger_recursive(s0, s1, &s0_map, &s1_map);
}

int sentence_equivalent(sentence *s0, sentence *s1){
	return sentence_equivalent_exact(s0, s1) || (sentence_stronger(s0, s1) && sentence_stronger(s1, s0));
}

static int sentence_stronger_exact(sentence *s0, sentence *s1){
	int i;

	if(s0->type == OR){
		return sentence_stronger_exact(s0->child0, s1) && sentence_stronger_exact(s0->child1, s1);
	} else if(s1->type == AND){
		return sentence_stronger_exact(s0, s1->child0) && sentence_stronger_exact(s0, s1->child1);
	} else if(sentence_trivially_false_exact(s0)){
		return 1;
	} else if(sentence_trivially_true_exact(s1)){
		return 1;
	} else if(s0->type == NOT && s1->type == NOT){
		return sentence_stronger_exact(s1->child0, s0->child0);
	} else if(s1->type == NOT && s1->child0->type == NOT){
		return sentence_stronger_exact(s0, s1->child0->child0);
	} else if(s0->type == IMPLIES && s1->type == IMPLIES){
		return sentence_stronger_exact(s1->child0, s0->child0) && sentence_stronger_exact(s0->child1, s1->child1);
	} else if(s0->type == FORALL && s1->type == FORALL){
		return sentence_stronger_exact(s0->child0, s1->child0);
	} else if(s0->type == EXISTS && s1->type == EXISTS){
		return sentence_stronger_exact(s0->child0, s1->child0);
	} else if(s0->type == BICOND && s1->type == BICOND){
		if(sentence_equivalent_exact(s0->child0, s1->child0) && sentence_equivalent_exact(s0->child1, s1->child1)){
			return 1;
		}
		if(sentence_equivalent_exact(s0->child0, s1->child1) && sentence_equivalent_exact(s0->child1, s1->child0)){
			return 1;
		}
		//Used to also check here if s0->child0 and s0->child1 are equivalent
		//But since it's an implication, we only care about s1
		if(sentence_equivalent_exact(s1->child0, s1->child1)){
			return 1;
		}
		return 0;
	} else if(s1->type == BICOND){
		return sentence_equivalent_exact(s1->child0, s1->child1);
	} else if(s0->type == RELATION && s1->type == RELATION){
		if(s0->relation_data != s1->relation_data){
			return 0;
		}
		if(s0->is_bound0 != s1->is_bound0){
			return 0;
		}
		if(s0->is_bound1 != s1->is_bound1){
			return 0;
		}
		if(s0->is_bound0){
			if(s0->var0_id != s1->var0_id){
				return 0;
			}
		} else {
			if(s0->var0 != s1->var0){
				return 0;
			}
		}
		if(s0->is_bound1){
			if(s0->var1_id != s1->var1_id){
				return 0;
			}
		} else {
			if(s0->var1 != s1->var1){
				return 0;
			}
		}
		return 1;
	} else if(s0->type == PROPOSITION && s1->type == PROPOSITION){
		if(s0->is_bound != s1->is_bound){
			return 0;
		}
		if(s0->is_bound){
			if(s0->definition_id != s1->definition_id){
				return 0;
			}
		} else {
			if(s0->definition_data != s1->definition_data){
				return 0;
			}
		}
		if(s0->num_args != s1->num_args){
			return 0;
		}
		for(i = 0; i < s0->num_args; i++){
			if(s0->proposition_args[i].is_bound != s1->proposition_args[i].is_bound){
				return 0;
			}
			if(s0->proposition_args[i].is_bound){
				if(s0->proposition_args[i].var_id != s1->proposition_args[i].var_id){
					return 0;
				}
			} else {
				if(s0->proposition_args[i].var != s1->proposition_args[i].var){
					return 0;
				}
			}
		}
		return 1;
	} else {
		if(s0->type == AND && (sentence_stronger_exact(s0->child0, s1) || sentence_stronger_exact(s0->child1, s1))){
			return 1;
		}
		if(s1->type == OR && (sentence_stronger_exact(s0, s1->child0) || sentence_stronger_exact(s0, s1->child1))){
			return 1;
		}
		return 0;
	}
}

//Determine if two sentences imply each other
static int sentence_equivalent_exact(sentence *s0, sentence *s1){
	return sentence_stronger_exact(s0, s1) && sentence_stronger_exact(s1, s0);
}

//Determine if a sentence is trivially true
static int sentence_trivially_true_exact(sentence *s){
	if(s->type == IMPLIES){
		return sentence_stronger_exact(s->child0, s->child1);
	} else if(s->type == BICOND){
		return sentence_equivalent_exact(s->child0, s->child1);
	} else if(s->type == AND){
		return sentence_trivially_true_exact(s->child0) && sentence_trivially_true_exact(s->child1);
	} else if(s->type == OR){
		return sentence_trivially_true_exact(s->child0) || sentence_trivially_true_exact(s->child1);
	} else if(s->type == FORALL){
		return sentence_trivially_true_exact(s->child0);
	} else if(s->type == NOT){
		return sentence_trivially_false_exact(s->child0);
	} else if(s->type == TRUE){
		return 1;
	} else {
		return 0;
	}
}

//Determine if a sentence is trivially false
static int sentence_trivially_false_exact(sentence *s){
	int true0;
	int true1;
	int false0;
	int false1;

	if(s->type == IMPLIES){
		return sentence_trivially_true_exact(s->child0) && sentence_trivially_false_exact(s->child1);
	} else if(s->type == BICOND){
		true0 = sentence_trivially_true_exact(s->child0);
		false0 = sentence_trivially_false_exact(s->child0);
		true1 = sentence_trivially_true_exact(s->child1);
		false1 = sentence_trivially_false_exact(s->child1);
		return (true0 && false1) || (false0 && true1);
	} else if(s->type == AND){
		return sentence_trivially_false_exact(s->child0) || sentence_trivially_false_exact(s->child1);
	} else if(s->type == OR){
		return sentence_trivially_false_exact(s->child0) && sentence_trivially_false_exact(s->child1);
	} else if(s->type == EXISTS || s->type == FORALL){
		return sentence_trivially_false_exact(s->child0);
	} else if(s->type == NOT){
		return sentence_trivially_true_exact(s->child0);
	} else if(s->type == FALSE){
		return 1;
	} else {
		return 0;
	}
}
