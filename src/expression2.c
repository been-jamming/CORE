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

//Allocate and initialize a context
context *create_context(context *parent){
	context *output;

	output = malloc(sizeof(context));
	output->variables = create_dictionary(NULL);
	output->definitions = create_dictionary(NULL);
	output->relations = create_dictionary(NULL);
	output->parent = parent;

	return output;
}

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

//Parse "iff" command
expr_value *parse_iff(char **c){
	expr_value *arg0;
	expr_value *arg1;
	expr_value *output;
	sentence *arg0_sentence;
	sentence *arg1_sentence;
	sentence *output_sentence;
	unsigned char verified;

	arg0 = parse_expr_value(c);
	skip_whitespace(c);
	if(**c != ','){
		error(ERROR_COMMA);
	}
	if(arg0->type != SENTENCE){
		error(ERROR_ARGUMENT_TYPE);
	}
	if(arg0->sentence_data->type != IMPLIES){
		error(ERROR_ARGUMENT_TYPE);
	}

	++*c;
	skip_whitespace(c);
	arg1 = parse_expr_value(c);
	skip_whitespace(c);
	if(**c != ')'){
		error(ERROR_PARENTHESES);
	}
	if(arg1->type != SENTENCE){
		error(ERROR_ARGUMENT_TYPE);
	}
	if(arg1->sentence_data->type != IMPLIES){
		error(ERROR_ARGUMENT_TYPE);
	}
	++*c;

	verified = arg0->verified && arg1->verified;
	arg0_sentence = arg0->sentence_data;
	arg1_sentence = arg1->sentence_data;

	if(!sentence_equivalent(arg0_sentence->child0, arg1_sentence->child1) || !sentence_equivalent(arg0_sentence->child1, arg1_sentence->child0)){
		error(ERROR_MISMATCHED_IMPLICATIONS);
	}

	free_expr_value(arg1);
	arg0_sentence->type = BICOND;
	arg0->verified = verified;

	return arg0;
}

//Parse "branch" command
expr_value *parse_branch(char **c){
	expr_value *output;
	expr_value *or_arg;
	expr_value *returned_val;
	sentence *or_sentence;
	sentence *peeled;
	char (*var_names)[256];
	context *first_context;
	context *next_context;
	int max_vars;
	int num_vars;
	int current_arg = 1;
	unsigned char verified;

	or_arg = parse_expr_value(c);
	skip_whitespace(c);
	if(**c != ','){
		error(ERROR_COMMA);
	}
	if(or_arg->type != SENTENCE){
		error(ERROR_ARGUMENT_TYPE);
	}
	if(!or_arg->verified){
		error(ERROR_ARGUMENT_VERIFY);
	}
	or_sentence = or_arg->sentence_data;
	if(or_sentence->num_bound_vars > 0){
		error(ERROR_ARGUMENT_BOUND_VARIABLES);
	}
	if(or_sentence->num_bound_props > 0){
		error(ERROR_ARGUMENT_BOUND_PROPOSITIONS);
	}
	++*c;
	skip_whitespace(c);

	max_vars = count_or(or_sentence);
	var_names = malloc(sizeof(char [256])*max_vars);

	if(get_identifier(c, var_names[0], 256)){
		error(ERROR_IDENTIFIER_LENGTH);
	}
	if(var_names[0][0] == '\0'){
		error(ERROR_IDENTIFIER_EXPECTED);
	}
	skip_whitespace(c);
	num_vars = 1;

	if(**c != ','){
		error(ERROR_COMMA);
	}

	while(**c != ')'){
		if(**c != ','){
			error(ERROR_PARENTHESES_OR_COMMA);
		}
		++*c;
		skip_whitespace(c);
		if(num_vars >= max_vars){
			error(ERROR_TOO_MANY_UNPACK);
		}
		if(get_identifier(c, var_names[num_vars], 256)){
			error(ERROR_IDENTIFIER_LENGTH);
		}
		if(var_names[num_vars][0] == '\0'){
			error(ERROR_IDENTIFIER_EXPECTED);
		}
		skip_whitespace(c);
		num_vars++;
	}

	++*c;
	skip_whitespace(c);

	if(**c != '{'){
		error(ERROR_BEGIN_BRACE);
	}
	++*c;
	skip_whitespace(c);
	first_context = create_context(global_context);
	peeled = peel_or_left(&or_sentence);
	create_sentence_variable(var_names[0], peeled, 1, first_context);
	global_context = first_context;
	output = parse_context(c);
	if(!output){
		error(ERROR_RETURN_EXPECTED);
	}
	if(**c != '}'){
		error(ERROR_END_BRACE);
	}
	++*c;
	skip_whitespace(c);

	global_context = global_context->parent;

	if(strncmp(*c, "or", 2) || is_alphanumeric((*c)[2])){
		error(ERROR_OR_EXPECTED);
	}
	*c += 2;
	skip_whitespace(c);
	while(current_arg < num_vars){
		if(**c != '{'){
			error(ERROR_BEGIN_BRACE);
		}
		++*c;
		skip_whitespace(c);
		next_context = create_context(global_context);
		if(current_arg < num_vars - 1){
			peeled = peel_or_left(&or_sentence);
		} else {
			peeled = or_sentence;
		}
		create_sentence_variable(var_names[current_arg], peeled, 1, next_context);
		global_context = next_context;
		returned_val = parse_context(c);
		if(!returned_val){
			error(ERROR_RETURN_EXPECTED);
		}
		if(**c != '}'){
			error(ERROR_END_BRACE);
		}
		++*c;
		skip_whitespace(c);

		global_context = global_context->parent;

		if(!sentence_equivalent(output->sentence_data, returned_val->sentence_data)){
			error(ERROR_MISMATCHED_RETURN);
		}
		free_expr_value(returned_val);
		free_context(next_context);
		current_arg++;

		if(current_arg < num_vars){
			if(strncmp(*c, "or", 2) || is_alphanumeric((*c)[2])){
				error(ERROR_OR_EXPECTED);
			}
			*c += 2;
			skip_whitespace(c);
		}
	}

	free_context(first_context);

	return output;
}

//Parse all built-in functions
expr_value *parse_expr_value_builtin(char **c){
	unsigned char is_and;

	if(!strncmp(*c, "left", 4) && !is_alphanumeric((*c)[4])){
		*c += 4;
		skip_whitespace(c);
		if(**c != '('){
			error(ERROR_BEGIN_PARENTHESES);
		}
		++*c;
		return parse_left(c);
	} else if(!strncmp(*c, "right", 5) && !is_alphanumeric((*c)[5])){
		*c += 5;
		skip_whitespace(c);
		if(**c != '('){
			error(ERROR_BEGIN_PARENTHESES);
		}
		++*c;
		return parse_right(c);
	} else if((!strncmp(*c, "and", 3) && !is_alphanumeric((*c)[3])) || (!strncmp(*c, "or", 2) && !is_alphanumeric((*c)[2]))){
		if(**c == 'a'){
			*c += 3;
			is_and = 1;
		} else {
			*c += 2;
			is_and = 0;
		}
		skip_whitespace(c);
		if(**c != '('){
			error(ERROR_BEGIN_PARENTHESES);
		}
		++*c;
		parse_and_or(c, is_and);
	} else if(!strncmp(*c, "expand", 6) && !is_alphanumeric((*c)[6])){
		*c += 6;
		skip_whitespace(c);
		if(**c != '('){
			error(ERROR_BEGIN_PARENTHESES);
		}
		++*c;
		return parse_expand(c);
	} else if(!strncmp(*c, "iff", 3) && !is_alphanumeric((*c)[3])){
		*c += 3;
		skip_whitespace(c);
		if(**c != '('){
			error(ERROR_BEGIN_PARENTHESES);
		}
		++*c;
		return parse_iff(c);
	} else if(!strncmp(*c, "branch", 6) && !is_alphanumeric((*c)[6])){
		*c += 6;
		skip_whitespace(c);
		if(**c != '('){
			error(ERROR_BEGIN_PARENTHESES);
		}
		++*c;
		return parse_branch(c);
	}

	return NULL;
}

//Parse the '|' operation
expr_value *parse_expr_value_pipe(char **c, expr_value *input){
	expr_value *output;
	sentence *input_sentence;
	sentence *next_input_sentence;
	variable *var;
	char var_name[256];

	if(input->type != SENTENCE){
		error(ERROR_OPERAND_TYPE);
	}
	input_sentence = input->sentence_data;
	if(input_sentence->num_bound_vars){
		error(ERROR_OPERAND_BOUND_VARIABLES);
	}
	if(input_sentence->num_bound_props){
		error(ERROR_OPERAND_BOUND_PROPOSITIONS);
	}
	if(!input->verified){
		error(ERROR_OPERAND_VERIFY);
	}
	skip_whitespace(c);
	if(**c != '|'){
		error(ERROR_IDENTIFIER_EXPECTED);
	}
	while(**c != '|'){
		if(input_sentence->type != EXISTS){
			error(ERROR_TOO_MANY_UNPACK);
		}
		if(get_identifier(c, var_name, 256)){
			error(ERROR_IDENTIFIER_LENGTH);
		}
		if(var_name[0] == '\0'){
			error(ERROR_IDENTIFIER_EXPECTED);
		}
		skip_whitespace(c);
		var = create_object_variable(var_name, global_context);
		next_input_sentence = input_sentence->child0;
		free(input_sentence);
		input_sentence = next_input_sentence;
		input_sentence->parent = NULL;
		substitute_variable(input_sentence, var);
		if(**c != '|' && **c != ','){
			error(ERROR_PIPE_OR_COMMA);
		}
		if(**c == ','){
			++*c;
			skip_whitespace(c);
			if(**c == '|'){
				error(ERROR_IDENTIFIER_EXPECTED);
			}
		}
	}

	++*c;
	skip_whitespace(c);
	free(input);
	output = create_expr_value(SENTENCE);
	output->verified = 1;
	output->sentence_data = input_sentence;

	return output;
}

//Parse the '(' operation
expr_value *parse_expr_value_parentheses(char **c, expr_value *input){
	expr_value *output;
	expr_value *arg;
	sentence_type compare_type;
	sentence *input_sentence;
	sentence *next_input_sentence;
	sentence *arg_sentence;
	variable *var;
	char var_name[256];
	unsigned char is_child0;
	unsigned char verified;

	if(input->type != SENTENCE){
		error(ERROR_OPERAND_TYPE);
	}
	input_sentence = input->sentence_data;
	if(input_sentence->num_bound_vars){
		error(ERROR_OPERAND_BOUND_VARIABLES);
	}
	if(input_sentence->num_bound_props){
		error(ERROR_OPERAND_BOUND_PROPOSITIONS);
	}

	compare_type = input_sentence->type;
	skip_whitespace(c);
	if(**c == ')'){
		error(ERROR_EXPRESSION_OR_VARIABLE);
	}
	if(compare_type == IMPLIES || compare_type == BICOND || compare_type == NOT){
		arg = parse_and_or(c, 1);
		arg_sentence = arg->sentence_data;
		skip_whitespace(c);
		if(compare_type == BICOND){
			is_child0 = sentence_equivalent(arg_sentence, input_sentence->child0);
			if(!is_child0 && !sentence_equivalent(arg_sentence, input_sentence->child1)){
				error(ERROR_MISMATCHED_ARGUMENT);
			}
		} else {
			if(!sentence_equivalent(arg_sentence, input_sentence->child0)){
				error(ERROR_MISMATCHED_ARGUMENT);
			}
		}
		verified = input->verified && arg->verified;
		free_expr_value(arg);
		if(compare_type == IMPLIES || (compare_type == BICOND && is_child0)){
			free_sentence(input_sentence->child0);
			next_input_sentence = input_sentence->child1;
			free(input_sentence);
			input_sentence = next_input_sentence;
			input_sentence->parent = NULL;
		} else if(compare_type == BICOND && !is_child0){
			free_sentence(input_sentence->child1);
			next_input_sentence = input_sentence->child0;
			free(input_sentence);
			input_sentence = next_input_sentence;
			input_sentence->parent = NULL;
		} else if(compare_type == NOT){
			free_sentence(input_sentence);
			input_sentence = create_sentence(FALSE, 0, 0);
		}
	} else if(compare_type == FORALL){
		verified = input->verified;
		while(**c != ')'){
			if(input_sentence->type != FORALL){
				error(ERROR_TOO_MANY_UNPACK);
			}
			if(get_identifier(c, var_name, 256)){
				error(ERROR_IDENTIFIER_LENGTH);
			}
			if(var_name[0] == '\0'){
				error(ERROR_IDENTIFIER_EXPECTED);
			}
			skip_whitespace(c);
			var = get_variable(var_name, global_context);
			if(!var){
				error(ERROR_VARIABLE_NOT_FOUND);
			}
			if(var->type != OBJECT_VAR){
				error(ERROR_ARGUMENT_TYPE);
			}
			next_input_sentence = input_sentence->child0;
			free(input_sentence);
			input_sentence = next_input_sentence;
			input_sentence->parent = NULL;
			substitute_variable(input_sentence, var);
			if(**c != ')' && **c != ','){
				error(ERROR_PARENTHESES_OR_COMMA);
			}
			if(**c == ','){
				++*c;
				skip_whitespace(c);
				if(**c == ')'){
					error(ERROR_EXPRESSION);
				}
			}
		}
		++*c;
		skip_whitespace(c);
	} else {
		error(ERROR_OPERAND_TYPE);
	}

	free(input);
	output = create_expr_value(SENTENCE);
	output->verified = verified;
	output->sentence_data = input_sentence;

	return output;
}

expr_value *parse_expr_value(char **c){
	return NULL;
}

expr_value *parse_context(char **c){
	return NULL;
}
