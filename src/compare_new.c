#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "custom_malloc.h"
#include "dictionary.h"
#include "predicate.h"
#include "compare_new.h"

enum map_type{
	//The variable cannot be mapped
	RESERVED,
	//The variable has not been mapped
	UNMAPPED,
	//The variable is mapped to a bound variable
	BOUND_VAR,
	//The variable is mapped to an object
	POINTER
};

//The data stores the status of a quantified variable during sentence comparison
struct map_entry{
	enum map_type type;
	union{
		struct {
			int range_lower;
			int range_upper;
		};
		int var_id;
		variable *var;
	};
};

enum seek_status{
	FAILURE = 0,
	SUCCESS = 1,
	CONTINUE = 2
};

//A linked list which stores a stack of searched subsentences
//The linked list is stored on the stack!
struct subsentence_stack{
	sentence *s0;
	sentence *s1;
	struct map_entry *source_map;
	struct map_entry *dest_map;
	struct subsentence_stack *parent;
};

int sentence_stronger_recursive(struct subsentence_stack *parent);
int seek_next(struct subsentence_stack *parent);

//Later will have better allocation routines
struct map_entry *get_map_list(int size){
	int i;
	struct map_entry *output;

	output = malloc(sizeof(struct map_entry)*size);
	for(i = 0; i < size; i++){
		output[i].type = RESERVED;
	}

	return output;
}

void free_map_list(struct map_entry *map){
	free(map);
}

int stronger_premise_or(struct subsentence_stack *parent){
	struct subsentence_stack entry;

	entry = (struct subsentence_stack) {.s0 = parent->s0->child0, .s1 = parent->s1, .parent = parent, .source_map = parent->source_map, .dest_map = parent->dest_map};

	return sentence_stronger_recursive(&entry);
}

int stronger_conclusion_and(struct subsentence_stack *parent){
	struct subsentence_stack entry;

	entry = (struct subsentence_stack) {.s0 = parent->s0, .s1 = parent->s1->child0, .parent = parent, .source_map = parent->source_map, .dest_map = parent->dest_map};

	return sentence_stronger_recursive(&entry);
}

int stronger_contrapositive(struct subsentence_stack *parent){
	struct subsentence_stack entry;

	entry = (struct subsentence_stack) {.s0 = parent->s1->child0, .s1 = parent->s0->child0, .parent = parent, .source_map = parent->dest_map, .dest_map = parent->source_map};

	return sentence_stronger_recursive(&entry);
}

int stronger_double_negation(struct subsentence_stack *parent){
	struct subsentence_stack entry;

	entry = (struct subsentence_stack) {.s0 = parent->s0, .s1 = parent->s1->child0->child0, .parent = parent, .source_map = parent->source_map, .dest_map = parent->dest_map};

	return sentence_stronger_recursive(&entry);
}

int stronger_implies(struct subsentence_stack *parent){
	struct subsentence_stack entry;

	entry = (struct subsentence_stack) {.s0 = parent->s1->child0, .s1 = parent->s0->child0, .parent = parent, .source_map = parent->dest_map, .dest_map = parent->source_map};

	return sentence_stronger_recursive(&entry);
}

int stronger_forall(struct subsentence_stack *parent){
	int i;
	int range_lower;
	int range_upper;
	int output;
	sentence *s1_child;
	struct subsentence_stack entry;
	struct map_entry *new_source_map;
	struct map_entry *new_dest_map;

	new_source_map = get_map_list(parent->s0->child0->num_bound_vars);
	memcpy(new_source_map, parent->source_map, sizeof(struct map_entry)*parent->s0->num_bound_vars);

	if(parent->s1->type == FORALL){
		s1_child = parent->s1->child0;
	} else {
		s1_child = parent->s1;
	}

	range_lower = parent->s1->num_bound_vars;
	range_upper = s1_child->num_bound_vars - 1;

	new_dest_map = get_map_list(s1_child->num_bound_vars);
	memcpy(new_dest_map, parent->dest_map, sizeof(struct map_entry)*parent->s1->num_bound_vars);
	for(i = range_lower; i <= range_upper; i++){
		new_dest_map[i].type = RESERVED;
	}

	for(i = parent->s0->num_bound_vars; i < parent->s0->child0->num_bound_vars; i++){
		new_source_map[i].type = UNMAPPED;
		new_source_map[i].range_lower = range_lower;
		new_source_map[i].range_upper = range_upper;
	}

	entry = (struct subsentence_stack) {.s0 = parent->s0->child0, .s1 = s1_child, .parent = parent, .source_map = new_source_map, .dest_map = new_dest_map};
	output = sentence_stronger_recursive(&entry);
	free_map_list(new_source_map);
	free_map_list(new_dest_map);

	return output;
}

int stronger_exists(struct subsentence_stack *parent){
	int i;
	int range_lower;
	int range_upper;
	int output;
	sentence *s0_child;
	struct subsentence_stack entry;
	struct map_entry *new_source_map;
	struct map_entry *new_dest_map;

	new_dest_map = get_map_list(parent->s1->child0->num_bound_vars);
	memcpy(new_dest_map, parent->dest_map, sizeof(struct map_entry)*parent->s1->num_bound_vars);

	if(parent->s0->type == EXISTS){
		s0_child = parent->s0->child0;
	} else {
		s0_child = parent->s0;
	}

	range_lower = parent->s0->num_bound_vars;
	range_upper = s0_child->num_bound_vars - 1;

	new_source_map = get_map_list(s0_child->num_bound_vars);
	memcpy(new_source_map, parent->source_map, sizeof(struct map_entry)*parent->s0->num_bound_vars);
	for(i = range_lower; i <= range_upper; i++){
		new_source_map[i].type = RESERVED;
	}

	for(i = parent->s1->num_bound_vars; i < parent->s1->child0->num_bound_vars; i++){
		new_dest_map[i].type = UNMAPPED;
		new_dest_map[i].range_lower = range_lower;
		new_dest_map[i].range_upper = range_upper;
	}

	entry = (struct subsentence_stack) {.s0 = s0_child, .s1 = parent->s1->child0, .parent = parent, .source_map = new_source_map, .dest_map = new_dest_map};
	output = sentence_stronger_recursive(&entry);
	free_map_list(new_dest_map);
	free_map_list(new_source_map);

	return output;
}

int stronger_bicond_both(struct subsentence_stack *parent){
	struct subsentence_stack entry0;
	struct subsentence_stack entry1;

	entry0 = (struct subsentence_stack) {.s0 = parent->s0->child0, .s1 = parent->s1->child0, .parent = parent, .source_map = parent->source_map, .dest_map = parent->dest_map};
	entry1 = (struct subsentence_stack) {.s0 = parent->s0->child0, .s1 = parent->s1->child1, .parent = parent, .source_map = parent->source_map, .dest_map = parent->dest_map};

	//Remember, these aren't the final checks
	//We seek back up the tree and check the other implications once we reach a leaf node
	return sentence_stronger_recursive(&entry0) || sentence_stronger_recursive(&entry1);
}

int bind_arguments(struct map_entry entry0, struct map_entry entry1, struct map_entry *source_map){
	struct map_entry source_entry;

	if(entry0.type == BOUND_VAR){
		source_entry = source_map[entry0.var_id];
		if(source_entry.type == BOUND_VAR){
			return entry1.type == BOUND_VAR && entry1.var_id == source_entry.var_id;
		} else if(source_entry.type == POINTER){
			return entry1.type == POINTER && entry1.var == source_entry.var;
		} else if(source_entry.type == UNMAPPED){
			if(entry1.type == BOUND_VAR && entry1.var_id >= source_entry.range_lower && entry1.var_id <= source_entry.range_upper){
				source_map[entry0.var_id].type = BOUND_VAR;
				source_map[entry0.var_id].var_id = entry1.var_id;
				return 1;
			} else if(entry1.type == POINTER){
				source_map[entry0.var_id].type = POINTER;
				source_map[entry0.var_id].var = entry1.var;
				return 1;
			} else {
				return 0;
			}
		} else {
			return 0;
		}
	} else if(entry0.type == POINTER && entry1.type == POINTER){
		return entry0.var == entry1.var;
	} else {
		return 0;
	}
}

int stronger_relation(struct subsentence_stack *parent){
	struct map_entry s0_entry;
	struct map_entry s1_entry;

	if(parent->s0->relation_data != parent->s1->relation_data){
		return 0;
	}

	if(parent->s0->is_bound0){
		s0_entry = (struct map_entry) {.type = BOUND_VAR, .var_id = parent->s0->var0_id};
	} else {
		s0_entry = (struct map_entry) {.type = POINTER, .var = parent->s0->var0};
	}

	if(parent->s1->is_bound0){
		s1_entry = (struct map_entry) {.type = BOUND_VAR, .var_id = parent->s1->var0_id};
	} else {
		s1_entry = (struct map_entry) {.type = POINTER, .var = parent->s1->var0};
	}

	if(!bind_arguments(s0_entry, s1_entry, parent->source_map) && !bind_arguments(s1_entry, s0_entry, parent->dest_map)){
		return 0;
	}

	if(parent->s0->is_bound1){
		s0_entry = (struct map_entry) {.type = BOUND_VAR, .var_id = parent->s0->var1_id};
	} else {
		s0_entry = (struct map_entry) {.type = POINTER, .var = parent->s0->var1};
	}

	if(parent->s1->is_bound1){
		s1_entry = (struct map_entry) {.type = BOUND_VAR, .var_id = parent->s1->var1_id};
	} else {
		s1_entry = (struct map_entry) {.type = POINTER, .var = parent->s1->var1};
	}

	if(!bind_arguments(s0_entry, s1_entry, parent->source_map) && !bind_arguments(s1_entry, s0_entry, parent->dest_map)){
		return 0;
	}

	return seek_next(parent);
}

int stronger_proposition(struct subsentence_stack *parent){
	struct map_entry s0_entry;
	struct map_entry s1_entry;
	int i;

	if(parent->s0->is_bound != parent->s1->is_bound){
		return 0;
	}
	if(parent->s0->is_bound && parent->s0->definition_id != parent->s1->definition_id){
		return 0;
	}
	if(!parent->s0->is_bound && parent->s0->definition_data != parent->s1->definition_data){
		return 0;
	}
	if(parent->s0->num_args != parent->s1->num_args){
		return 0;
	}

	for(i = 0; i < parent->s0->num_args; i++){
		if(parent->s0->proposition_args[i].is_bound){
			s0_entry = (struct map_entry) {.type = BOUND_VAR, .var_id = parent->s0->proposition_args[i].var_id};
		} else {
			s0_entry = (struct map_entry) {.type = POINTER, .var = parent->s0->proposition_args[i].var};
		}

		if(parent->s1->proposition_args[i].is_bound){
			s1_entry = (struct map_entry) {.type = BOUND_VAR, .var_id = parent->s1->proposition_args[i].var_id};
		} else {
			s1_entry = (struct map_entry) {.type = POINTER, .var = parent->s1->proposition_args[i].var};
		}

		if(!bind_arguments(s0_entry, s1_entry, parent->source_map) && !bind_arguments(s1_entry, s0_entry, parent->dest_map)){
			return 0;
		}
	}

	return seek_next(parent);
}

int stronger_and_or(struct subsentence_stack *parent){
	struct subsentence_stack entry;
	struct map_entry *new_source_map;
	struct map_entry *new_dest_map;

	new_source_map = get_map_list(parent->s0->num_bound_vars);
	new_dest_map = get_map_list(parent->s1->num_bound_vars);

	if(parent->s0->type == AND){

		memcpy(new_source_map, parent->source_map, sizeof(struct map_entry)*parent->s0->num_bound_vars);
		memcpy(new_dest_map, parent->dest_map, sizeof(struct map_entry)*parent->s1->num_bound_vars);

		entry = (struct subsentence_stack) {.s0 = parent->s0->child0, .s1 = parent->s1, .parent = parent, .source_map = new_source_map, .dest_map = new_dest_map};

		if(sentence_stronger_recursive(&entry)){
			free_map_list(new_source_map);
			free_map_list(new_dest_map);
			return 1;
		}

		memcpy(new_source_map, parent->source_map, sizeof(struct map_entry)*parent->s0->num_bound_vars);
		memcpy(new_dest_map, parent->dest_map, sizeof(struct map_entry)*parent->s1->num_bound_vars);

		entry = (struct subsentence_stack) {.s0 = parent->s0->child1, .s1 = parent->s1, .parent = parent, .source_map = new_source_map, .dest_map = new_dest_map};
		if(sentence_stronger_recursive(&entry)){
			free_map_list(new_source_map);
			free_map_list(new_dest_map);
			return 1;
		}
	}

	if(parent->s1->type == OR){

		memcpy(new_source_map, parent->source_map, sizeof(struct map_entry)*parent->s0->num_bound_vars);
		memcpy(new_dest_map, parent->dest_map, sizeof(struct map_entry)*parent->s1->num_bound_vars);

		entry = (struct subsentence_stack) {.s0 = parent->s0, .s1 = parent->s1->child0, .parent = parent, .source_map = new_source_map, .dest_map = new_dest_map};
		if(sentence_stronger_recursive(&entry)){
			free_map_list(new_source_map);
			free_map_list(new_dest_map);
			return 1;
		}

		memcpy(new_source_map, parent->source_map, sizeof(struct map_entry)*parent->s0->num_bound_vars);
		memcpy(new_dest_map, parent->dest_map, sizeof(struct map_entry)*parent->s1->num_bound_vars);

		entry = (struct subsentence_stack) {.s0 = parent->s0, .s1 = parent->s1->child1, .parent = parent, .source_map = new_source_map, .dest_map = new_dest_map};
		if(sentence_stronger_recursive(&entry)){
			free_map_list(new_source_map);
			free_map_list(new_dest_map);
			return 1;
		}
	}

	free_map_list(new_source_map);
	free_map_list(new_dest_map);
	return 0;
}

//Recursively check if parent->s0 is stronger than parent->s1
int sentence_stronger_recursive(struct subsentence_stack *parent){
	if(parent->s0->type == OR){
		if(stronger_premise_or(parent)) return 1;
	} else if(parent->s1->type == AND){
		if(stronger_conclusion_and(parent)) return 1;
	} else if(sentence_trivially_false(parent->s0)){
		if(seek_next(parent)) return 1;
	} else if(sentence_trivially_true(parent->s1)){
		if(seek_next(parent)) return 1;
	} else if(parent->s0->type == NOT && parent->s1->type == NOT){
		if(stronger_contrapositive(parent)) return 1;
	} else if(parent->s1->type == NOT && parent->s1->child0->type == NOT){
		if(stronger_double_negation(parent)) return 1;
	} else if(parent->s0->type == IMPLIES && parent->s1->type == IMPLIES){
		if(stronger_implies(parent)) return 1;
	} else if(parent->s0->type == FORALL){
		if(stronger_forall(parent)) return 1;
	} else if(parent->s1->type == EXISTS){
		if(stronger_exists(parent)) return 1;
	} else if(parent->s0->type == BICOND && parent->s1->type == BICOND){
		if(stronger_bicond_both(parent)) return 1;
	} else if(parent->s0->type == RELATION && parent->s1->type == RELATION){
		if(stronger_relation(parent)) return 1;
	} else if(parent->s0->type == PROPOSITION && parent->s1->type == PROPOSITION){
		if(stronger_proposition(parent)) return 1;
	}
	if(parent->s0->type == AND || parent->s1->type == OR){
		return stronger_and_or(parent);
	} else {
		return 0;
	}
}

int seek_premise_or(struct subsentence_stack *parent){
	struct subsentence_stack entry;

	entry = (struct subsentence_stack) {.s0 = parent->s0->child1, .s1 = parent->s1, .parent = parent, .source_map = parent->source_map, .dest_map = parent->dest_map};

	return sentence_stronger_recursive(&entry);
}

int seek_conclusion_and(struct subsentence_stack *parent){
	struct subsentence_stack entry;

	entry = (struct subsentence_stack) {.s0 = parent->s0, .s1 = parent->s1->child1, .parent = parent, .source_map = parent->source_map, .dest_map = parent->dest_map};

	return sentence_stronger_recursive(&entry);
}

int seek_implies(struct subsentence_stack *parent){
	struct subsentence_stack entry;

	entry = (struct subsentence_stack) {.s0 = parent->s0->child1, .s1 = parent->s1->child1, .parent = parent, .source_map = parent->source_map, .dest_map = parent->dest_map};

	return sentence_stronger_recursive(&entry);
}

enum seek_status seek_bicond_both(struct subsentence_stack *parent, struct subsentence_stack *child){
	struct subsentence_stack entry;

	if(child->s0 == parent->s0->child0 && child->s1 == parent->s1->child0){
		entry = (struct subsentence_stack) {.s0 = parent->s1->child0, .s1 = parent->s0->child0, .parent = parent, .source_map = parent->dest_map, .dest_map = parent->source_map};
		return sentence_stronger_recursive(&entry);
	} else if(child->s0 == parent->s1->child0 && child->s1 == parent->s0->child0){
		//We must take care to swap the source and destination maps back to
		//what they were before. This is because they were switched by the
		//above case which was invoked earlier in the tree
		entry = (struct subsentence_stack) {.s0 = parent->s0->child1, .s1 = parent->s1->child1, .parent = parent, .source_map = parent->dest_map, .dest_map = parent->source_map};
		return sentence_stronger_recursive(&entry);
	} else if(child->s0 == parent->s0->child1 && child->s1 == parent->s1->child1){
		//Once again, we must swap the source and destination
		entry = (struct subsentence_stack) {.s0 = parent->s1->child1, .s1 = parent->s0->child1, .parent = parent, .source_map = parent->dest_map, .dest_map = parent->source_map};
		return sentence_stronger_recursive(&entry);
	} else if(child->s0 == parent->s0->child0 && child->s1 == parent->s1->child1){
		entry = (struct subsentence_stack) {.s0 = parent->s1->child1, .s1 = parent->s0->child0, .parent = parent, .source_map = parent->dest_map, .dest_map = parent->source_map};
		return sentence_stronger_recursive(&entry);
	} else if(child->s0 == parent->s1->child1 && child->s1 == parent->s0->child0){
		entry = (struct subsentence_stack) {.s0 = parent->s0->child1, .s1 = parent->s1->child0, .parent = parent, .source_map = parent->dest_map, .dest_map = parent->source_map};
		return sentence_stronger_recursive(&entry);
	} else if(child->s0 == parent->s0->child1 && child->s1 == parent->s1->child0){
		entry = (struct subsentence_stack) {.s0 = parent->s1->child0, .s1 = parent->s0->child1, .parent = parent, .source_map = parent->dest_map, .dest_map = parent->source_map};
		return sentence_stronger_recursive(&entry);
	}

	return CONTINUE;
}

int seek_next(struct subsentence_stack *parent){
	struct subsentence_stack entry;
	struct subsentence_stack *child;
	struct map_entry *leaf_source_map;
	struct map_entry *leaf_dest_map;
	struct map_entry *temp_map;
	int i;
	enum seek_status output;

	leaf_source_map = parent->source_map;
	leaf_dest_map = parent->dest_map;

	while(parent->parent != NULL){
		child = parent;
		parent = child->parent;

		//We must return to a node in the tree while keeping track of previously
		//bound variables
		entry = *parent;
		entry.source_map = leaf_source_map;
		entry.dest_map = leaf_dest_map;

		if(parent->s0->type == OR && parent->s0->child0 == child->s0){
			return seek_premise_or(&entry);
		} else if(parent->s1->type == AND && parent->s1->child0 == child->s1){
			return seek_conclusion_and(&entry);
		} else if(parent->s0->type == NOT && parent->s1->type == NOT){
			//Evaluating contrapositives swaps the source and
			//destination maps, so we must swap them back
			//before continuing up the tree
			temp_map = leaf_source_map;
			leaf_source_map = leaf_dest_map;
			leaf_dest_map = temp_map;
		} else if(parent->s0->type == IMPLIES && parent->s1->type == IMPLIES && child->s0 == parent->s1->child0 && child->s1 == parent->s0->child0){
			//While evaluating the implication, the source and
			//destination maps get swapped. We swap them back
			//before continuing down the tree
			temp_map = entry.source_map;
			entry.source_map = entry.dest_map;
			entry.dest_map = temp_map;

			return seek_implies(&entry);
		//If we seek past a quantifier, we need to check all quantifiers got bound
		//Otherwise the user could prove the existence of an object
		} else if(parent->s0->type == FORALL && child->s0 == parent->s0->child0){
			for(i = parent->s0->num_bound_vars; i < child->s0->num_bound_vars; i++){
				if(leaf_source_map[i].type == UNMAPPED){
					return 0;
				}
				if(leaf_source_map[i].type == RESERVED){
					custom_error(1, "Internal Error: The source and destination maps are mismatched\n", 1);
				}
			}
		} else if(parent->s1->type == EXISTS && child->s1 == parent->s1->child0){
			for(i = parent->s1->num_bound_vars; i < child->s1->num_bound_vars; i++){
				if(leaf_dest_map[i].type == UNMAPPED){
					return 0;
				}
				if(leaf_dest_map[i].type == RESERVED){
					custom_error(1, "Internal Error: The source and destination maps are mismatched\n", 1);
				}
			}
		} else if(parent->s0->type == BICOND && parent->s1->type == BICOND){
			output = seek_bicond_both(&entry, child);
			if(output != CONTINUE){
				return output;
			} else {
				//While evaluating the biconditional implication, the source and
				//destination maps get swapped. We swap them back before seeking
				//farther up the tree.
				temp_map = leaf_source_map;
				leaf_source_map = leaf_dest_map;
				leaf_dest_map = temp_map;
			}
		}
	}

	//We made it all the way through the tree! Yay!
	//This means that the implication has been verified
	return 1;
}

int sentence_stronger(sentence *s0, sentence *s1){
	struct subsentence_stack root;
	struct map_entry *source_map;
	struct map_entry *dest_map;
	int i;
	int output;

	if(s0->num_bound_vars != s1->num_bound_vars){
		return 0;
	}

	source_map = get_map_list(s0->num_bound_vars);
	dest_map = get_map_list(s1->num_bound_vars);

	for(i = 0; i < s0->num_bound_vars; i++){
		source_map[i] = (struct map_entry) {.type = BOUND_VAR, .var_id = i};
		dest_map[i] = (struct map_entry) {.type = RESERVED};
	}

	root = (struct subsentence_stack) {.s0 = s0, .s1 = s1, .parent = NULL, .source_map = source_map, .dest_map = dest_map};
	output = sentence_stronger_recursive(&root);

	free_map_list(source_map);
	free_map_list(dest_map);
	return output;
}

int sentence_equivalent(sentence *s0, sentence *s1){
	return sentence_stronger(s0, s1) && sentence_stronger(s1, s0);
}

int sentence_trivially_true(sentence *s){
	if(s->type == IMPLIES){
		return sentence_stronger(s->child0, s->child1);
	} else if(s->type == BICOND){
		return sentence_equivalent(s->child0, s->child1);
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
		false0 = sentence_trivially_false(s->child0);
		true1 = sentence_trivially_true(s->child1);
		false1 = sentence_trivially_false(s->child1);
		return (true0 && false1) || (false0 && true1);
	} else if(s->type == AND){
		return sentence_trivially_false(s->child0) || sentence_trivially_false(s->child1);
	} else if(s->type == OR){
		return sentence_trivially_false(s->child0) && sentence_trivially_false(s->child1);
	} else if(s->type == EXISTS){
		return sentence_trivially_false(s->child0);
	} else if(s->type == NOT){
		return sentence_trivially_true(s->child0);
	} else if(s->type == FALSE){
		return 1;
	} else {
		return 0;
	}
}

