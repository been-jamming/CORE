#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "predicate.h"
#include "expression2.h"
#include "custom_malloc.h"

//CORE Expression Parser
//Ben Jones

//Parses and evaluates expressions in CORE

//Allocate an expression value with a given type
expr_value *create_expr_value(expr_value_type type){
	expr_value *output;

	output = malloc(sizeof(expr_value));
	output->type = type;

	return output;
}

//Free an expression value
void free_expr_value(expr_value *val){
	if(val->type == SENTENCE){
		free_sentence(val->sentence_data);
	}
	free(val);
}

//Free a definition
//Don't decrement outside references
void free_definition_independent(definition *def){
	if(def->sentence_data){
		free_sentence_independent(def->sentence_data);
	}
	free(def->name);
	free(def);
}

//Free a definition
//Version which accepts void pointer
void free_definition_void(void *v){
	free_definition_independent(v);
}

//Free a definition
//Decrement all outside references
void free_definition(definition *def){
	if(def->sentence_data){
		free_sentence(def->sentence_data);
	}
	free(def->name);
	free(def);
}

//Decrement all outside references of a definition
void decrement_definition(definition *def){
	if(def->sentence_data){
		decrement_references_sentence(def->sentence_data);
	}
}

//Decrement all outside references of a definition
//Version which accepts void pointer
void decrement_definition_void(void *v){
	decrement_definition(v);
}

//Free a relation
//Don't decrement outside references
void free_relation_independent(relation *rel){
	if(rel->sentence_data){
		free_sentence_independent(rel->sentence_data);
	}
	free(rel->name);
	free(rel);
}

//Free a relation
//Version which accepts void pointer
void free_relation_void(void *v){
	free_relation_independent(v);
}

//Free a relation
//Decrement all outside references
void free_relation(relation *rel){
	if(rel->sentence_data){
		free_sentence(rel->sentence_data);
	}
	free(rel->name);
	free(rel);
}

//Decrement outside references of a relation
void decrement_relation(relation *rel){
	if(rel->sentence_data){
		decrement_references_sentence(rel->sentence_data);
	}
}

//Decrement outside references of a relation
//Version which accepts void pointer
void decrement_relation_void(void *v){
	decrement_relation(v);
}

//Free a variable
//Don't decrement outside references
void free_variable_independent(variable *var){
	if(var->type == SENTENCE_VAR){
		free_sentence_independent(var->sentence_data);
	} else if(var->type == CONTEXT_VAR){
		free_context_independent(var->context_data);
	}
	free(var->name);
	free(var);
}

//Free a variable
//Version which accepts a void pointer
void free_variable_void(void *v){
	free_variable_independent(v);
}

//Free a variable
//Decrement all outside references
void free_variable(variable *var){
	if(var->type == SENTENCE_VAR){
		free_sentence(var->sentence_data);
	} else if(var->type == CONTEXT_VAR){
		free_context(var->context_data);
	}
	free(var->name);
	free(var);
}

//Decrement outside references of a variable
void decrement_variable(variable *var){
	if(var->type == SENTENCE_VAR){
		decrement_references_sentence(var->sentence_data);
	} else if(var->type == CONTEXT_VAR){
		decrement_context(var->context_data);
	}
}

//Decrement outside references of a variable
//Version which accepts a void pointer
void decrement_variable_void(void *v){
	decrement_variable(v);
}

//Free a context
//Frees all dependent variables, definitions, and relations
//Does not decrement any outside references
void free_context_independent(context *c){
	free_dictionary(&(c->variables), free_variable_void);
	free_dictionary(&(c->definitions), free_definition_void);
	free_dictionary(&(c->relations), free_relation_void);
	free(c);
}

//Decrement all outside references of a context
void decrement_context(context *c){
	iterate_dictionary(c->variables, decrement_variable_void);
	iterate_dictionary(c->definitions, decrement_definition_void);
	iterate_dictionary(c->relations, decrement_relation_void);
}

//Free a context
//Frees all dependent variables, definitions, and relations
//Decrements all outside references
void free_context(context *c){
	iterate_dictionary(c->variables, decrement_variable_void);
	iterate_dictionary(c->definitions, decrement_definition_void);
	iterate_dictionary(c->relations, decrement_relation_void);
	free_dictionary(&(c->variables), free_variable_void);
	free_dictionary(&(c->definitions), free_definition_void);
	free_dictionary(&(c->relations), free_relation_void);
	free(c);
}

//Convert a definition to the sentence describing it
expr_value *definition_to_value(char *name){
	definition *def;
	expr_value *output;
	sentence *sentence_data;
	sentence *new;
	context *current_context;
	int i;

	current_context = global_context;
	while(current_context){
		def = read_dictionary(current_context->definitions, name, 0);
		if(def){
			break;
		}
		current_context = current_context->parent;
	}

	if(!def){
		error(ERROR_DEFINITION_EXISTS);
	}
	if(!def->sentence_data){
		error(ERROR_DEFINITION_EMPTY);
	}

	sentence_data = create_sentence(BICOND, def->sentence_data->num_bound_vars, 0);
	sentence_data->child1 = create_sentence(PROPOSITION, def->sentence_data->num_bound_vars, 0);
	sentence_data->child1->is_bound = 0;
	sentence_data->child1->definition_data = def;
	def->num_references++;
	sentence_data->child1->num_args = def->num_args;
	sentence_data->child1->proposition_args = malloc(sizeof(proposition_arg)*sentence_data->child1->num_args);
	sentence_data->child1->parent = sentence_data;
	for(i = 0; i < sentence_data->child1->num_args; i++){
		sentence_data->child1->proposition_args[i].is_bound = 1;
		sentence_data->child1->proposition_args[i].var_id = i;
	}
	sentence_data->child0 = malloc(sizeof(sentence));
	copy_sentence(sentence_data->child0, def->sentence_data);
	sentence_data->child0->parent = sentence_data;
	for(i = def->sentence_data->num_bound_vars - 1; i >= 0; i--){
		new = create_sentence(FORALL, i, 0);
		new->child0 = sentence_data;
		new->child0->parent = new;
		sentence_data = new;
	}

	output = create_expr_value(SENTENCE);
	output->sentence_data = sentence_data;
	output->verified = 1;

	return output;
}

//Convert a relation to the sentence describing it
expr_value *relation_to_value(char *name){
	relation *rel;
	expr_value *output;
	sentence *sentence_data;
	sentence *new;
	context *current_context;
	int i;

	current_context = global_context;
	while(current_context){
		rel = read_dictionary(current_context->relations, name, 0);
		if(rel){
			break;
		}
		current_context = current_context->parent;
	}

	if(!rel){
		error(ERROR_RELATION_EXISTS);
	}
	if(!rel->sentence_data){
		error(ERROR_RELATION_EMPTY);
	}

	sentence_data = create_sentence(BICOND, rel->sentence_data->num_bound_vars, 0);
	sentence_data->child1 = create_sentence(RELATION, rel->sentence_data->num_bound_vars, 0);
	sentence_data->child1->relation_data = rel;
	sentence_data->child1->is_bound0 = 1;
	sentence_data->child1->var0_id = 0;
	sentence_data->child1->is_bound1 = 1;
	sentence_data->child1->var1_id = 1;
	sentence_data->child1->parent = sentence_data;
	sentence_data->child0 = malloc(sizeof(sentence));
	copy_sentence(sentence_data->child0, rel->sentence_data);
	sentence_data->child0->parent = sentence_data;
	new = create_sentence(FORALL, 1, 0);
	new->child0 = sentence_data;
	new->child0->parent = new;
	sentence_data = new;
	new = create_sentence(FORALL, 0, 0);
	new->child0 = sentence_data;
	new->child0->parent = new;

	output = create_expr_value(SENTENCE);
	output->sentence_data = new;
	output->verified = 1;

	return output;
}

//Parse an identifier
expr_value *parse_expr_identifier(char **c){
	variable *var = NULL;
	expr_value *output = NULL;
	definition *def = NULL;
	char name_buffer[256];
	char *beginning = NULL;
	context *current_context = NULL;

	skip_whitespace(c);
	beginning = *c;
	if(**c == '#'){
		++*c;
		skip_whitespace(c);
		if(**c == '['){
			++*c;
			skip_whitespace(c);
			if(get_relation_identifier(c, name_buffer, 256)){
				error(ERROR_IDENTIFIER_LENGTH);
			}
			if(name_buffer[0] == '\0'){
				error(ERROR_IDENTIFIER_EXPECTED);
			}
			skip_whitespace(c);
			if(**c != ']'){
				error(ERROR_BRACKET_EXPECTED);
			}
			++*c;
			output = relation_to_value(name_buffer);
		} else {
			if(get_identifier(c, name_buffer, 256)){
				error(ERROR_IDENTIFIER_LENGTH);
			}
			if(name_buffer[0] == '\0'){
				error(ERROR_IDENTIFIER_EXPECTED);
			}
			output = definition_to_value(name_buffer);
		}
		return output;
	} else {
		if(get_identifier(c, name_buffer, 256)){
			error(ERROR_IDENTIFIER_LENGTH);
		}
		if(name_buffer[0] == '\0'){
			*c = beginning;
			return NULL;
		}
		skip_whitespace(c);

		current_context = global_context;
		while(current_context){
			var = read_dictionary(current_context->variables, name_buffer, 0);
			if(var){
				break;
			}
			current_context = current_context->parent;
		}

		if(!var){
			*c = beginning;
			return NULL;
		}

		while(var->type == CONTEXT_VAR){
			if(**c != '.'){
				error(ERROR_DOT_EXPECTED);
			}
			++*c;
			skip_whitespace(c);
			current_context = var->context_data;
			if(get_identifier(c, name_buffer, 256)){
				error(ERROR_IDENTIFIER_LENGTH);
			}
			if(name_buffer[0] == '\0'){
				error(ERROR_IDENTIFIER_EXPECTED);
			}
			skip_whitespace(c);
			var = read_dictionary(current_context->variables, name_buffer, 0);
			if(!var){
				error(ERROR_CONTEXT_MEMBER);
			}
		}

		if(**c == '.'){
			error(ERROR_CANNOT_DOT);
		}

		if(var->type == SENTENCE_VAR){
			output = create_expr_value(SENTENCE);
			output->sentence_data = malloc(sizeof(sentence));
			copy_sentence(output->sentence_data, var->sentence_data);
			output->verified = var->verified;
		} else if(var->type == OBJECT_VAR){
			output = create_expr_value(OBJECT);
			output->var = var;
		}

		return output;
	}
}

//Add new bound variables to a sentence
void add_bound_variables(sentence *s, int increment){
	int i;

	s->num_bound_vars += increment;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
		case BICOND:
			add_bound_variables(s->child0, increment);
			add_bound_variables(s->child1, increment);
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			add_bound_variables(s->child0, increment);
			break;
		case RELATION:
			if(s->is_bound0){
				s->var0_id += increment;
			}
			if(s->is_bound1){
				s->var1_id += increment;
			}
			break;
		case PROPOSITION:
			for(i = 0; i < s->num_args; i++){
				if(s->proposition_args[i].is_bound){
					s->proposition_args[i].var_id += increment;
				}
			}
			break;
		case TRUE:
		case FALSE:
			//pass
			break;
	}
}

//Substitute a variable for the 0 bound variable in a sentence
void substitute_variable_recursive(sentence *s, int id, variable *v){
	int i;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
		case BICOND:
			substitute_variable_recursive(s->child0, id, v);
			substitute_variable_recursive(s->child1, id, v);
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			substitute_variable_recursive(s->child0, id, v);
			break;
		case RELATION:
			if(s->is_bound0 && s->var0_id == id){
				s->is_bound0 = 0;
				s->var0 = v;
				v->num_references++;
			}
			if(s->is_bound1 && s->var1_id == id){
				s->is_bound1 = 0;
				s->var1 = v;
				v->num_references++;
			}
			break;
		case PROPOSITION:
			for(i = 0; i < s->num_args; i++){
				if(s->proposition_args[i].is_bound && s->proposition_args[i].var_id == id){
					s->proposition_args[i].is_bound = 0;
					s->proposition_args[i].var = v;
					v->num_references++;
				}
			}
			break;
		case TRUE:
		case FALSE:
			//pass
			break;
	}
}

//Substitute a variable in a sentence for a bound variable
void substitute_variable(sentence *s, variable *v){
	if(s->num_bound_vars <= 0){
		error(ERROR_NO_BOUND_VARIABLES);
	}

	substitute_variable_recursive(s, 0, v);
	add_bound_variables(s, -1);
}

//Reset the replaced flag for each variable in a sentence
void reset_replaced(sentence *s){
	int i;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
		case BICOND:
			reset_replaced(s->child0);
			reset_replaced(s->child1);
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			reset_replaced(s->child0);
			break;
		case RELATION:
			s->replaced0 = 0;
			s->replaced1 = 0;
			break;
		case PROPOSITION:
			for(i = 0; i < s->num_args; i++){
				s->proposition_args[i].replaced = 0;
			}
			break;
		case TRUE:
		case FALSE:
			//pass
			break;
	}
}

//Set the number of bound propositions in a sentence
void set_num_bound_props(sentence *s, int num_props){
	s->num_bound_props = num_props;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
		case BICOND:
			set_num_bound_props(s->child0, num_props);
			set_num_bound_props(s->child1, num_props);
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			set_num_bound_props(s->child0, num_props);
			break;
		default:
			break;
	}
}

//Replace a bound variable id with another bound variable id
void substitute_variable_bound(sentence *s, int id, int new_id){
	int i;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
		case BICOND:
			substitute_variable_bound(s->child0, id, new_id);
			substitute_variable_bound(s->child1, id, new_id);
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			substitute_variable_bound(s->child0, id, new_id);
			break;
		case RELATION:
			if(s->is_bound0 && s->var0_id == id && !s->replaced0){
				s->var0_id = new_id;
				s->replaced0 = 1;
			}
			if(s->is_bound1 && s->var1_id == id && !s->replaced1){
				s->var1_id = new_id;
				s->replaced1 = 1;
			}
			break;
		case PROPOSITION:
			for(i = 0; i < s->num_args; i++){
				if(s->proposition_args[i].is_bound && s->proposition_args[i].var_id == id && !s->proposition_args[i].replaced){
					s->proposition_args[i].var_id = new_id;
					s->proposition_args[i].replaced = 1;
				}
			}
			break;
		case TRUE:
		case FALSE:
			//pass
			break;
	}
}

//Substitute a sentence for a bound proposition
void substitute_proposition(sentence *s, sentence *child){
	int i;
	int num_bound_vars;
	int num_bound_props;
	int num_args;
	proposition_arg *prop_args;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
		case BICOND:
			substitute_proposition(s->child0, child);
			substitute_proposition(s->child1, child);
			break;
		case NOT:
		case EXISTS:
		case FORALL:
			substitute_proposition(s->child0, child);
			break;
		case PROPOSITION:
			if(s->is_bound && s->definition_id == 0){
				if(s->num_args != child->num_bound_vars){
					error(ERROR_ARGUMENT_COUNT);
				}
				num_bound_vars = s->num_bound_vars;
				num_bound_props = s->num_bound_props;
				num_args = s->num_args;
				prop_args = s->proposition_args;
				copy_sentence(s, child);
				reset_replaced(s);
				set_num_bound_props(s, num_bound_props);
				for(i = 0; i < num_args; i++){
					if(prop_args[i].is_bound){
						substitute_variable_bound(s, i, prop_args[i].var_id + num_args - num_bound_vars);
					} else {
						substitute_variable_recursive(s, i, prop_args[i].var);
						prop_args[i].var->num_references--;
					}
				}
				add_bound_variables(s, num_bound_vars - num_args);
				free(prop_args);
			} else if(s->is_bound){
				s->definition_id--;
			}
			break;
		case RELATION:
		case TRUE:
		case FALSE:
			//pass
			break;
	}
}

//Create a variable storing an object
variable *create_object_variable(char *var_name, context *parent_context){
	variable *output;
	variable *var;

	var = read_dictionary(parent_context->variables, var_name, 0);
	if(var && !var->num_references){
		free_variable(var);
	} else if(var){
		error(ERROR_VARIABLE_OVERWRITE);
	}

	output = malloc(sizeof(variable));
	output->type = OBJECT_VAR;
	output->num_references = 0;
	output->name = malloc(sizeof(char)*(strlen(var_name) + 1));
	strcpy(output->name, var_name);
	output->parent_context = parent_context;

	write_dictionary(&(parent_context->variables), var_name, output, 0);

	return output;
}

//Create a variables storing a sentence
variable *create_sentence_variable(char *var_name, sentence *sentence_data, unsigned char verified, context *parent_context){
	variable *output;
	variable *var;

	var = read_dictionary(parent_context->variables, var_name, 0);
	if(var && !var->num_references){
		free_variable(var);
	} else if(var){
		error(ERROR_VARIABLE_OVERWRITE);
	}

	output = malloc(sizeof(variable));
	output->type = SENTENCE_VAR;
	output->num_references = 0;
	output->name = malloc(sizeof(char)*(strlen(var_name) + 1));
	strcpy(output->name, var_name);
	output->parent_context = parent_context;
	output->sentence_data = sentence_data;
	output->verified = verified;

	write_dictionary(&(parent_context->variables), var_name, output, 0);

	return output;
}

//Create a variable storing a context
variable *create_context_variable(char *var_name, context *context_data, context *parent_context){
	variable *output;
	variable *var;

	var = read_dictionary(parent_context->variables, var_name, 0);
	if(var && !var->num_references){
		free_variable(var);
	} else if(var){
		error(ERROR_VARIABLE_OVERWRITE);
	}

	output = malloc(sizeof(variable));
	output->type = CONTEXT_VAR;
	output->num_references = 0;
	output->name = malloc(sizeof(char)*(strlen(var_name) + 1));
	strcpy(output->name, var_name);
	output->parent_context = parent_context;
	output->context_data = context_data;

	write_dictionary(&(parent_context->variables), var_name, output, 0);

	return output;
}

//Retrieve a variable by name
variable *get_variable(char *var_name, context *parent_context){
	variable *output = NULL;

	while(parent_context){
		output = read_dictionary(parent_context->variables, var_name, 0);
		if(output){
			break;
		}
		parent_context = parent_context->parent;
	}

	return output;
}

//Parse the "left" command
expr_value *parse_left(char **c){
	expr_value *expr;
	expr_value *output;
	sentence *sentence_data;
	sentence *temp;

	expr = parse_expr_value(c);
	skip_whitespace(c);
	if(**c != ')'){
		error(ERROR_END_PARENTHESES);
	}
	++*c;

	if(expr->type != SENTENCE){
		error(ERROR_ARGUMENT_TYPE);
	}

	sentence_data = expr->sentence_data;
	if(sentence_data->num_bound_vars > 0){
		error(ERROR_ARGUMENT_BOUND_VARIABLES);
	}
	if(sentence_data->num_bound_props > 0){
		error(ERROR_ARGUMENT_BOUND_PROPOSITIONS);
	}

	if(sentence_data->type == AND || sentence_data->type == OR || sentence_data->type == IMPLIES){
		expr->verified = (sentence_data->type == AND) && expr->verified;
		expr->sentence_data = sentence_data->child0;
		free_sentence(sentence_data->child1);
		free(sentence_data);
		return expr;
	} else if(sentence_data->type == BICOND){
		temp = sentence_data->child0;
		sentence_data->child0 = sentence_data->child1;
		sentence_data->child1 = temp;
		sentence_data->type = IMPLIES;
		return expr;
	} else {
		error(ERROR_ARGUMENT_TYPE);
	}

	//Prevent GCC from giving a warning. NULL is never returned.
	return NULL;
}

//Parse the "right" command
expr_value *parse_right(char **c){
	expr_value *expr;
	expr_value *output;
	sentence *sentence_data;

	expr = parse_expr_value(c);
	skip_whitespace(c);
	if(**c != ')'){
		error(ERROR_END_PARENTHESES);
	}
	++*c;

	if(expr->type != SENTENCE){
		error(ERROR_ARGUMENT_TYPE);
	}

	sentence_data = expr->sentence_data;
	if(sentence_data->num_bound_vars > 0){
		error(ERROR_ARGUMENT_BOUND_VARIABLES);
	}
	if(sentence_data->num_bound_props > 0){
		error(ERROR_ARGUMENT_BOUND_PROPOSITIONS);
	}

	if(sentence_data->type == AND || sentence_data->type == OR || sentence_data->type == IMPLIES){
		expr->verified = (sentence_data->type == AND) && expr->verified;
		expr->sentence_data = sentence_data->child1;
		free_sentence(sentence_data->child0);
		free(sentence_data);
		return expr;
	} else if(sentence_data->type == BICOND){
		sentence_data->type = IMPLIES;
		return expr;
	} else {
		error(ERROR_ARGUMENT_TYPE);
	}

	//Prevent GCC from giving a warning. NULL is never returned.
	return NULL;
}

//Parse an "and" or "or" function call
//Reused for parentheses operation
expr_value *parse_and_or(char **c, unsigned char is_and){
	expr_value *output;
	expr_value *val;
	sentence *last_parent_sentence = NULL;
	sentence *new;
	sentence *output_sentence;
	sentence *s;
	unsigned char verified;

	val = parse_expr_value(c);
	if(val->type != SENTENCE){
		error(ERROR_ARGUMENT_TYPE);
	}
	output_sentence = val->sentence_data;
	if(output_sentence->num_bound_vars > 0){
		error(ERROR_ARGUMENT_BOUND_VARIABLES);
	}
	if(output_sentence->num_bound_props > 0){
		error(ERROR_ARGUMENT_BOUND_PROPOSITIONS);
	}

	verified = val->verified;
	skip_whitespace(c);
	if(**c != ')' && **c != ','){
		error(ERROR_PARENTHESES_OR_COMMA);
	}

	while(**c != ')'){
		++*c;
		skip_whitespace(c);
		val = parse_expr_value(c);
		if(val->type != SENTENCE){
			error(ERROR_ARGUMENT_TYPE);
		}
		s = val->sentence_data;
		if(s->num_bound_vars > 0){
			error(ERROR_ARGUMENT_BOUND_VARIABLES);
		}
		if(s->num_bound_props > 0){
			error(ERROR_ARGUMENT_BOUND_PROPOSITIONS);
		}

		if(is_and){
			verified = verified && val->verified;
			new = create_sentence(AND, 0, 0);
		} else {
			verified = verified || val->verified;
			new = create_sentence(OR, 0, 0);
		}

		if(!last_parent_sentence){
			last_parent_sentence = new;
			last_parent_sentence->child0 = output_sentence;
			last_parent_sentence->child1 = s;
			last_parent_sentence->child0->parent = last_parent_sentence;
			last_parent_sentence->child1->parent = last_parent_sentence;
			output_sentence = last_parent_sentence;
		} else {
			new->child0 = last_parent_sentence->child1;
			new->child1 = s;
			new->child0->parent = new;
			new->child1->parent = new;
			last_parent_sentence->child1 = new;
			last_parent_sentence->child1->parent = last_parent_sentence;
			last_parent_sentence = new;
		}

		skip_whitespace(c);
		if(**c != ')' && **c != ','){
			error(ERROR_PARENTHESES_OR_COMMA);
		}

		free(val);
	}
	
	++*c;
	output = create_expr_value(SENTENCE);
	output->sentence_data = output_sentence;
	output->verified = verified;

	return output;
}

//Parse "expand" command
expr_value *parse_expand(char **c){
	expr_value *output;
	sentence *output_sentence;
	expr_value *arg;
	sentence *arg_sentence;
	definition *def;
	relation *rel;
	unsigned char verified;
	int i;

	arg = parse_expr_value(c);
	verified = arg->verified;
	skip_whitespace(c);
	if(**c != ')'){
		error(ERROR_END_PARENTHESES);
	}
	++*c;

	if(arg->type != SENTENCE){
		error(ERROR_ARGUMENT_TYPE);
	}
	arg_sentence = arg->sentence_data;
	if(arg_sentence->num_bound_vars > 0){
		error(ERROR_ARGUMENT_BOUND_VARIABLES);
	}
	if(arg_sentence->num_bound_props > 0){
		error(ERROR_ARGUMENT_BOUND_PROPOSITIONS);
	}
	if(arg_sentence->type != PROPOSITION && arg_sentence->type != RELATION){
		error(ERROR_ARGUMENT_TYPE);
	}

	if(arg_sentence->type == PROPOSITION){
		def = arg_sentence->definition_data;

		if(!def->sentence_data){
			error(ERROR_DEFINITION_EMPTY);
		}

		output_sentence = malloc(sizeof(sentence));
		output_sentence->parent = NULL;
		copy_sentence(output_sentence, def->sentence_data);

		for(i = 0; i < arg_sentence->num_args; i++){
			substitute_variable(output_sentence, arg_sentence->proposition_args[i].var);
		}

		free_expr_value(arg);
	} else {
		rel = arg_sentence->relation_data;

		if(!rel->sentence_data){
			error(ERROR_RELATION_EMPTY);
		}

		output_sentence = malloc(sizeof(sentence));
		output_sentence->parent = NULL;
		copy_sentence(output_sentence, rel->sentence_data);

		substitute_variable(output_sentence, arg_sentence->var0);
		substitute_variable(output_sentence, arg_sentence->var1);

		free_expr_value(arg);
	}

	output = create_expr_value(SENTENCE);
	output->sentence_data = output_sentence;
	output->verified = verified;

	return output;
}

expr_value *parse_expr_value(char **c){
	return NULL;
}
