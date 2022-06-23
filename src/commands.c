#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <libgen.h>
#include "dictionary.h"
#include "predicate.h"
#include "expression.h"
#include "imports.h"
#include "custom_malloc.h"

//CORE Command Parser
//Ben Jones

void new_scope(void){
	global_context = create_context(global_context);
}

void drop_scope(void){
	context *next_context;

	next_context = global_context->parent;
	if(!next_context){
		error(ERROR_PARENT_CONTEXT);
	}
	free_context(global_context);
	global_context = next_context;
}

definition *create_definition(char *name, sentence *sentence_data, int num_args, context *parent_context){
	definition *def;

	if((def = read_dictionary(parent_context->definitions, name, 0))){
		if(def->num_references == 0){
			free_definition(def);
		} else {
			error(ERROR_DUPLICATE_DEFINITION);
		}
	}
	def = malloc(sizeof(definition));
	def->sentence_data = sentence_data;
	def->name = malloc(sizeof(char)*(strlen(name) + 1));
	strcpy(def->name, name);
	def->num_args = num_args;
	def->num_references = 0;
	def->destination = NULL;
	def->parent_context = parent_context;
	write_dictionary(&(parent_context->definitions), name, def, 0);

	return def;
}

relation *create_relation(char *name, sentence *sentence_data, context *parent_context){
	relation *rel;

	if((rel = read_dictionary(parent_context->relations, name, 0))){
		if(rel->num_references == 0){
			free_relation(rel);
		} else {
			error(ERROR_DUPLICATE_RELATION);
		}
	}
	rel = malloc(sizeof(relation));
	rel->sentence_data = sentence_data;
	rel->name = malloc(sizeof(char)*(strlen(name) + 1));
	strcpy(rel->name, name);
	rel->num_references = 0;
	rel->destination = NULL;
	rel->parent_context = parent_context;
	write_dictionary(&(parent_context->relations), name, rel, 0);

	return rel;
}

char *load_file(char *file_name){
	FILE *fp;
	size_t size;
	size_t read_size;
	char *output;

	fp = fopen(file_name, "rb");
	if(!fp){
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	output = malloc(sizeof(char)*(size + 1));
	read_size = fread(output, sizeof(char), size, fp);
	if(read_size < size){
		free(output);
		return NULL;
	}
	fclose(fp);
	output[size] = '\0';

	return output;
}

int print_command(char **c){
	expr_value *val;

	if(strncmp(*c, "print", 5) || is_alphanumeric((*c)[5])){
		return 0;
	}
	*c += 5;
	skip_whitespace(c);

	if(global_context->dependent){
		error(ERROR_DEPENDENT_SCOPE);
	}

	val = parse_expr_value(c);
	printf("'%s' line %d: ", global_file_name, global_line_number);
	print_expr_value(val);
	printf("\n");
	free_expr_value(val);
	skip_whitespace(c);
	if(**c != ';'){
		error(ERROR_SEMICOLON);
	}
	++*c;

	return 1;
}

int object_command(char **c){
	char var_name[256];
	variable *var;

	skip_whitespace(c);
	if(strncmp(*c, "object", 6) || is_alphanumeric((*c)[6])){
		return 0;
	}
	*c += 6;
	skip_whitespace(c);

	if(global_context->parent && !global_context->dependent){
		error(ERROR_GLOBAL_SCOPE);
	}

	if(get_identifier(c, var_name, 256)){
		error(ERROR_IDENTIFIER_LENGTH);
	}
	if(var_name[0] == '\0'){
		error(ERROR_IDENTIFIER_EXPECTED);
	}

	var = create_object_variable(var_name, global_context);
	add_object_dependency(var);

	skip_whitespace(c);
	while(**c == ','){
		++*c;
		skip_whitespace(c);
		if(get_identifier(c, var_name, 256)){
			error(ERROR_IDENTIFIER_LENGTH);
		}
		if(var_name[0] == '\0'){
			error(ERROR_IDENTIFIER_EXPECTED);
		}

		var = create_object_variable(var_name, global_context);
		add_object_dependency(var);
		skip_whitespace(c);
	}

	if(**c != ';'){
		error(ERROR_SEMICOLON);
	}

	++*c;
	return 1;
}

definition *define_command(char **c){
	int num_bound_vars = 0;
	int *new_int;
	char var_name[256];
	char def_name[256];
	sentence *s;
	definition *output;
	char *beginning;
	unsigned char is_dependent;

	skip_whitespace(c);
	beginning = *c;
	if(strncmp(*c, "dependent", 9) || is_alphanumeric((*c)[9])){
		is_dependent = 0;
		if(strncmp(*c, "define", 6) || is_alphanumeric((*c)[6])){
			return NULL;
		}
	} else {
		is_dependent = 1;
		*c += 9;
		skip_whitespace(c);
		if(strncmp(*c, "define", 6) || is_alphanumeric((*c)[6])){
			*c = beginning;
			return NULL;
		}
	}
	*c += 6;
	skip_whitespace(c);

	if(get_identifier(c, def_name, 256)){
		error(ERROR_IDENTIFIER_LENGTH);
	}
	if(def_name[0] == '\0'){
		error(ERROR_IDENTIFIER_EXPECTED);
	}

	clear_bound_variables();
	skip_whitespace(c);

	if(**c == '('){
		++*c;
		skip_whitespace(c);
		while(**c != ')'){
			if(get_identifier(c, var_name, 256)){
				error(ERROR_IDENTIFIER_LENGTH);
			}
			if(var_name[0] == '\0'){
				error(ERROR_IDENTIFIER_EXPECTED);
			}
			if(read_dictionary(global_bound_variables, var_name, 0)){
				error(ERROR_DUPLICATE_IDENTIFIER);
			}
			new_int = malloc(sizeof(int));
			*new_int = num_bound_vars;
			write_dictionary(&global_bound_variables, var_name, new_int, 0);
			num_bound_vars++;
			skip_whitespace(c);
			if(**c == ','){
				++*c;
				skip_whitespace(c);
				if(**c == ')'){
					error(ERROR_IDENTIFIER_EXPECTED);
				}
			} else if(**c != ')'){
				error(ERROR_PARENTHESES_OR_COMMA);
			}
		}
		++*c;
	}
	skip_whitespace(c);
	if(is_dependent && global_context->parent && !global_context->dependent){
		error(ERROR_GLOBAL_SCOPE);
	}
	if(**c == ';'){
		if(global_context->parent && !global_context->dependent){
			error(ERROR_GLOBAL_SCOPE);
		}
		++*c;
		output = create_definition(def_name, NULL, num_bound_vars, global_context);
		add_definition_dependency(output);
	} else if(**c == ':'){
		++*c;
		s = parse_sentence(c, num_bound_vars, 0);
		if(**c != ';'){
			error(ERROR_SEMICOLON);
		}
		++*c;
		output = create_definition(def_name, s, num_bound_vars, global_context);
		if(!is_dependent && global_context->dependent){
			error(ERROR_DEPENDENT_SCOPE);
		}
		if(is_dependent){
			add_definition_dependency(output);
		}
	} else {
		error(ERROR_COLON_OR_SEMICOLON);
	}

	clear_bound_variables();
	return output;
}

variable *axiom_command(char **c){
	int num_bound_props = 0;
	int num_args;
	bound_proposition *new_bound_prop;
	char name_buffer[256];
	char axiom_name[256];
	char *int_end;
	variable *output;
	sentence *s;

	skip_whitespace(c);
	if(strncmp(*c, "axiom", 5) || is_alphanumeric((*c)[5])){
		return NULL;
	}
	*c += 5;
	skip_whitespace(c);

	if(global_context->parent && !global_context->dependent){
		error(ERROR_GLOBAL_SCOPE);
	}
	clear_bound_propositions();
	if(get_identifier(c, axiom_name, 256)){
		error(ERROR_IDENTIFIER_LENGTH);
	}
	if(axiom_name[0] == '\0'){
		error(ERROR_IDENTIFIER_EXPECTED);
	}

	skip_whitespace(c);
	if(**c == '['){
		++*c;
		skip_whitespace(c);
		while(**c != ']'){
			if(get_identifier(c, name_buffer, 256)){
				error(ERROR_IDENTIFIER_LENGTH);
			}
			if(name_buffer[0] == '\0'){
				error(ERROR_IDENTIFIER_EXPECTED);
			}
			if(read_dictionary(global_bound_propositions, name_buffer, 0)){
				error(ERROR_DUPLICATE_IDENTIFIER);
			}
			new_bound_prop = malloc(sizeof(bound_proposition));
			skip_whitespace(c);
			if(**c == '('){
				++*c;
				skip_whitespace(c);
				if(**c != ')' && !is_digit(**c)){
					error(ERROR_NUM_ARGS);
				} else if(is_digit(**c)){
					num_args = strtol(*c, &int_end, 10);
					*c = int_end;
					if(num_args < 0){
						error(ERROR_NUM_ARGS);
					}
					skip_whitespace(c);
					if(**c != ')'){
						error(ERROR_PARENTHESES);
					}
					++*c;
				} else {
					++*c;
					num_args = 0;
				}
				new_bound_prop->num_args = num_args;
			} else {
				new_bound_prop->num_args = 0;
			}
			new_bound_prop->prop_id = num_bound_props;
			write_dictionary(&global_bound_propositions, name_buffer, new_bound_prop, 0);
			num_bound_props++;
			skip_whitespace(c);
			if(**c == ','){
				++*c;
				skip_whitespace(c);
				if(**c == ']'){
					error(ERROR_IDENTIFIER_EXPECTED);
				}
			} else if(**c != ']'){
				error(ERROR_BRACKET_OR_COMMA);
			}
		}
		++*c;
	}
	skip_whitespace(c);
	if(**c != ':'){
		error(ERROR_COLON);
	}
	++*c;
	skip_whitespace(c);
	s = parse_sentence(c, 0, num_bound_props);
	skip_whitespace(c);
	if(**c != ';'){
		error(ERROR_SEMICOLON);
	}
	++*c;

	clear_bound_propositions();
	output = create_sentence_variable(axiom_name, s, 1, global_context);
	add_axiom_dependency(output);
	return output;
}

static unsigned char get_assign_vars(char **c, char ***var_names, unsigned int *current_var_name){
	unsigned int var_names_size = 16;
	unsigned int i;

	skip_whitespace(c);
	if(**c == ','){
		return 1;
	}

	*current_var_name = 0;
	*var_names = malloc(sizeof(char *)*var_names_size);
	
	do{
		if(**c == ','){
			++*c;
		}
		skip_whitespace(c);
		if(*current_var_name >= var_names_size){
			var_names_size += 16;
			*var_names = realloc(*var_names, sizeof(char *)*var_names_size);
		}
		(*var_names)[*current_var_name] = malloc(sizeof(char)*256);
		if(get_identifier(c, (*var_names)[*current_var_name], 256)){
			for(i = 0; i <= *current_var_name; i++){
				free((*var_names)[i]);
			}
			free(*var_names);
			return 1;
		}
		(*var_names)[*current_var_name] = realloc((*var_names)[*current_var_name], sizeof(char)*(strlen((*var_names)[*current_var_name]) + 1));
		skip_whitespace(c);
		if(!(*var_names)[*current_var_name][0]){
			for(i = 0; i <= *current_var_name; i++){
				free((*var_names)[i]);
			}
			free(*var_names);
			return 1;
		}
		(*current_var_name)++;
	} while(**c == ',');

	if(**c != '='){
		for(i = 0; i < *current_var_name; i++){
			free((*var_names)[i]);
		}
		free(*var_names);
		return 1;
	}

	++*c;
	skip_whitespace(c);

	return 0;
}

int assign_command(char **c){
	expr_value *val;
	char *beginning;
	char **var_names;
	unsigned int num_vars;
	unsigned int i;
	sentence *s;
	sentence *extracted;

	beginning = *c;
	if(get_assign_vars(c, &var_names, &num_vars)){
		*c = beginning;
		return 0;
	}

	val = parse_expr_value(c);
	if(val->type != SENTENCE){
		error(ERROR_ARGUMENT_TYPE);
	}
	s = val->sentence_data;
	skip_whitespace(c);
	if(**c != ';'){
		error(ERROR_SEMICOLON);
	}
	++*c;

	for(i = 0; i < num_vars - 1; i++){
		if(!s){
			error(ERROR_TOO_MANY_UNPACK);
		}
		extracted = peel_and_left(&s);
		extracted->parent = NULL;
		create_sentence_variable(var_names[i], extracted, val->verified, global_context);
	}

	if(!s){
		error(ERROR_TOO_MANY_UNPACK);
	}

	if(global_context->dependent){
		error(ERROR_DEPENDENT_SCOPE);
	}

	create_sentence_variable(var_names[num_vars - 1], s, val->verified, global_context);

	for(i = 0; i < num_vars; i++){
		free(var_names[i]);
	}
	free(var_names);
	free(val);

	return 1;
}

int relation_command(char **c){
	char identifier0[256];
	char identifier1[256];
	char identifier2[256];
	relation *rel;
	int *new_int;
	sentence *s;
	char *beginning;
	unsigned char is_dependent;

	skip_whitespace(c);
	beginning = *c;
	if(strncmp(*c, "dependent", 9) || is_alphanumeric((*c)[9])){
		is_dependent = 0;
		if(strncmp(*c, "relation", 8) || is_alphanumeric((*c)[8])){
			return 0;
		}
	} else {
		is_dependent = 1;
		*c += 9;
		skip_whitespace(c);
		if(strncmp(*c, "relation", 8) || is_alphanumeric((*c)[8])){
			*c = beginning;
			return 0;
		}
	}
	*c += 8;
	skip_whitespace(c);

	if(get_relation_identifier(c, identifier0, 256)){
		error(ERROR_IDENTIFIER_LENGTH);
	}
	skip_whitespace(c);
	if(is_dependent && global_context->parent && !global_context->dependent){
		error(ERROR_GLOBAL_SCOPE);
	}
	if(**c == ';'){
		if(identifier0[0] == '\0'){
			error(ERROR_RELATION_IDENTIFIER);
		}
		++*c;
		if(global_context->parent && !global_context->dependent){
			error(ERROR_GLOBAL_SCOPE);
		}

		if(read_dictionary(global_context->relations, identifier0, 0)){
			error(ERROR_DUPLICATE_RELATION);
		}
		rel = create_relation(identifier0, NULL, global_context);
		add_relation_dependency(rel);
		return 1;
	} else {
		if(!is_alpha(identifier0[0])){
			error(ERROR_IDENTIFIER_EXPECTED);
		}
		if(get_relation_identifier(c, identifier1, 256)){
			error(ERROR_IDENTIFIER_LENGTH);
		}
		if(identifier1[0] == '\0'){
			error(ERROR_RELATION_IDENTIFIER);
		}
		skip_whitespace(c);
		if(get_identifier(c, identifier2, 256)){
			error(ERROR_IDENTIFIER_LENGTH);
		}
		if(identifier2[0] == '\0'){
			error(ERROR_IDENTIFIER_EXPECTED);
		}
		if(!strcmp(identifier0, identifier2)){
			error(ERROR_DUPLICATE_IDENTIFIER);
		}
		skip_whitespace(c);
		if(**c != ':'){
			error(ERROR_COLON);
		}
		++*c;
		new_int = malloc(sizeof(int));
		*new_int = 0;
		write_dictionary(&global_bound_variables, identifier0, new_int, 0);
		new_int = malloc(sizeof(int));
		*new_int = 1;
		write_dictionary(&global_bound_variables, identifier2, new_int, 0);
		s = parse_sentence(c, 2, 0);
		if(**c != ';'){
			error(ERROR_SEMICOLON);
		}
		++*c;

		clear_bound_variables();
		rel = create_relation(identifier1, s, global_context);
		if(!is_dependent && global_context->dependent){
			error(ERROR_DEPENDENT_SCOPE);
		}
		if(is_dependent){
			add_relation_dependency(rel);
		}
		return 1;
	}
}

expr_value *return_command(char **c){
	expr_value *output;
	expr_value *arg;
	sentence *arg_sentence;
	sentence *output_sentence;
	sentence *last_parent = NULL;
	sentence *new;

	skip_whitespace(c);
	if(strncmp(*c, "return", 6) || is_alphanumeric((*c)[6])){
		return NULL;
	}
	*c += 6;
	skip_whitespace(c);

	if(global_context->dependent){
		error(ERROR_DEPENDENT_SCOPE);
	}

	arg = parse_expr_value(c);
	if(arg->type != SENTENCE){
		error(ERROR_ARGUMENT_TYPE);
	}
	output_sentence = arg->sentence_data;
	if(output_sentence->num_bound_vars){
		error(ERROR_ARGUMENT_BOUND_VARIABLES);
	}
	if(output_sentence->num_bound_props){
		error(ERROR_ARGUMENT_BOUND_PROPOSITIONS);
	}
	if(!arg->verified){
		error(ERROR_ARGUMENT_VERIFY);
	}
	free(arg);
	skip_whitespace(c);

	if(**c != ';' && **c != ','){
		error(ERROR_SEMICOLON_OR_COMMA);
	}

	while(**c != ';'){
		++*c;
		skip_whitespace(c);
		arg = parse_expr_value(c);
		if(arg->type != SENTENCE){
			error(ERROR_ARGUMENT_TYPE);
		}
		arg_sentence = arg->sentence_data;
		if(arg_sentence->num_bound_vars){
			error(ERROR_ARGUMENT_BOUND_VARIABLES);
		}
		if(arg_sentence->num_bound_props){
			error(ERROR_ARGUMENT_BOUND_PROPOSITIONS);
		}
		if(!arg->verified){
			error(ERROR_ARGUMENT_VERIFY);
		}
		free(arg);
		skip_whitespace(c);
		new = create_sentence(AND, 0, 0);
		if(!last_parent){
			last_parent = new;
			last_parent->child0 = output_sentence;
			last_parent->child1 = arg_sentence;
			last_parent->child0->parent = last_parent;
			last_parent->child1->parent = last_parent;
			output_sentence = last_parent;
		} else {
			new->child0 = last_parent->child1;
			new->child1 = arg_sentence;
			new->child0->parent = new;
			new->child1->parent = new;
			last_parent->child1 = new;
			last_parent->child1->parent = last_parent;
			last_parent = new;
		}

		if(**c != ';' && **c != ','){
			error(ERROR_SEMICOLON_OR_COMMA);
		}
	}

	++*c;
	output = create_expr_value(SENTENCE);
	output->sentence_data = output_sentence;
	output->verified = 1;

	return output;
}

void evaluate_command(char **c){
	expr_value *val;

	val = parse_expr_value(c);
	free_expr_value(val);
	if(**c != ';'){
		error(ERROR_SEMICOLON);
	}
	++*c;
}

int debug_command(char **c){
	expr_value *val;
	expr_value *val2;

	skip_whitespace(c);
	if(strncmp(*c, "debug", 5) || is_alphanumeric((*c)[5])){
		return 0;
	}
	*c += 5;
	skip_whitespace(c);

	if(global_context->dependent){
		error(ERROR_DEPENDENT_SCOPE);
	}

	val = parse_expr_value(c);
	if(**c != ','){
		error(ERROR_COMMA);
	}
	++*c;
	skip_whitespace(c);
	val2 = parse_expr_value(c);
	if(val->type == SENTENCE && val2->type == SENTENCE){
		printf("DEBUG: %d\n", sentence_equivalent(val->sentence_data, val2->sentence_data));
	} else if(val->type == OBJECT && val2->type == OBJECT){
		printf("DEBUG: %d\n", val->var == val2->var);
	} else {
		printf("DEBUG: 0\n");
	}
	free_expr_value(val);
	free_expr_value(val2);
	skip_whitespace(c);
	if(**c != ';'){
		error(ERROR_SEMICOLON);
	}
	++*c;
	return 1;
}

variable *prove_command(char **c){
	int num_bound_props = 0;
	int num_args;
	int i, j;
	bound_proposition *new_bound_prop;
	definition **definitions = NULL;
	unsigned int definitions_size;
	char name_buffer[256];
	char proof_name[256];
	char *int_end;
	variable *output;
	expr_value *returned;
	sentence *result;
	sentence *goal;
	sentence prop_sentence;

	skip_whitespace(c);
	if(strncmp(*c, "prove", 5) || is_alphanumeric((*c)[5])){
		return NULL;
	}
	*c += 5;
	skip_whitespace(c);

	if(global_context->dependent){
		error(ERROR_DEPENDENT_SCOPE);
	}

	clear_bound_propositions();
	if(get_identifier(c, proof_name, 256)){
		error(ERROR_IDENTIFIER_LENGTH);
	}
	if(proof_name[0] == '\0'){
		error(ERROR_IDENTIFIER_EXPECTED);
	}

	new_scope();

	skip_whitespace(c);

	if(**c == '['){
		++*c;
		skip_whitespace(c);
		definitions = malloc(sizeof(definition *)*8);
		definitions_size = 8;
		while(**c != ']'){
			if(get_identifier(c, name_buffer, 256)){
				error(ERROR_IDENTIFIER_LENGTH);
			}
			if(name_buffer[0] == '\0'){
				error(ERROR_IDENTIFIER_EXPECTED);
			}
			if(read_dictionary(global_bound_propositions, name_buffer, 0)){
				error(ERROR_DUPLICATE_IDENTIFIER);
			}
			if(num_bound_props >= definitions_size){
				definitions_size += 8;
				definitions = realloc(definitions, sizeof(definition *)*definitions_size);
			}
			definitions[num_bound_props] = malloc(sizeof(definition));
			new_bound_prop = malloc(sizeof(bound_proposition));
			skip_whitespace(c);
			if(**c == '('){
				++*c;
				skip_whitespace(c);
				if(**c != ')' && !is_digit(**c)){
					error(ERROR_NUM_ARGS);
				} else if(is_digit(**c)){
					num_args = strtol(*c, &int_end, 10);
					*c = int_end;
					if(num_args < 0){
						error(ERROR_NUM_ARGS);
					}
					skip_whitespace(c);
					if(**c != ')'){
						error(ERROR_END_PARENTHESES);
					}
					++*c;
				} else {
					++*c;
					num_args = 0;
				}
			} else {
				num_args = 0;
			}
			definitions[num_bound_props]->name = malloc(sizeof(char)*(strlen(name_buffer) + 1));
			strcpy(definitions[num_bound_props]->name, name_buffer);
			definitions[num_bound_props]->sentence_data = NULL;
			definitions[num_bound_props]->num_references = 1;
			definitions[num_bound_props]->num_args = num_args;
			definitions[num_bound_props]->parent_context = global_context;
			definitions[num_bound_props]->destination = NULL;

			new_bound_prop->prop_id = num_bound_props;
			new_bound_prop->num_args = num_args;
			write_dictionary(&global_bound_propositions, name_buffer, new_bound_prop, 0);

			num_bound_props++;
			skip_whitespace(c);
			if(**c == ','){
				++*c;
				skip_whitespace(c);
				if(**c == ']'){
					error(ERROR_IDENTIFIER_EXPECTED);
				}
			} else if(**c != ']'){
				error(ERROR_BRACKET_OR_COMMA);
			}
		}
		++*c;
	} else {
		definitions_size = 0;
	}

	for(i = 0; i < num_bound_props; i++){
		write_dictionary(&(global_context->definitions), definitions[i]->name, definitions[i], 0);
	}
	skip_whitespace(c);
	if(**c != ':'){
		error(ERROR_COLON);
	}
	++*c;
	skip_whitespace(c);
	result = parse_sentence(c, 0, num_bound_props);
	skip_whitespace(c);
	if(**c != '{'){
		error(ERROR_BEGIN_BRACE);
	}
	++*c;
	skip_whitespace(c);

	goal = malloc(sizeof(sentence));
	copy_sentence(goal, result);
	goal->parent = NULL;
	for(i = 0; i < num_bound_props; i++){
		prop_sentence.type = PROPOSITION;
		prop_sentence.num_bound_vars = definitions[i]->num_args;
		prop_sentence.num_bound_props = 0;
		prop_sentence.parent = NULL;
		prop_sentence.is_bound = 0;
		prop_sentence.definition_data = definitions[i];
		prop_sentence.num_args = definitions[i]->num_args;
		prop_sentence.proposition_args = malloc(sizeof(proposition_arg)*(definitions[i]->num_args));
		for(j = 0; j < definitions[i]->num_args; j++){
			prop_sentence.proposition_args[j].is_bound = 1;
			prop_sentence.proposition_args[j].var_id = j;
		}
		substitute_proposition(goal, &prop_sentence);
		free(prop_sentence.proposition_args);
	}

	clear_bound_propositions();
	create_sentence_variable("goal", goal, 0, global_context);
	global_context->goal = malloc(sizeof(sentence));
	copy_sentence(global_context->goal, goal);
	global_context->goal->parent = NULL;
	returned = parse_context(c);

	if(**c != '}'){
		error(ERROR_END_BRACE);
	}
	++*c;

	if(!returned){
		error(ERROR_RETURN_EXPECTED);
	}
	if(returned->type != SENTENCE){
		error(ERROR_RETURN_TYPE);
	}
	if(!returned->verified){
		error(ERROR_RETURN_VERIFIED);
	}
	if(!sentence_stronger(returned->sentence_data, goal)){
		error(ERROR_MISMATCHED_RETURN);
	}
	free_expr_value(returned);
	drop_scope();
	free(definitions);

	output = create_sentence_variable(proof_name, result, 1, global_context);
	return output;
}

expr_value *given_command(char **c){
	unsigned char explicit_scope;
	char name_buffer[256];
	sentence *next_goal = NULL;
	sentence *temp_goal;
	variable *new_var;
	expr_value *return_value;
	expr_value *output_value;

	skip_whitespace(c);
	if(strncmp(*c, "given", 5) || is_alphanumeric((*c)[5])){
		return NULL;
	}
	*c += 5;
	skip_whitespace(c);

	if(!global_context->goal){
		error(ERROR_GOAL);
	}

	if(**c != '|'){
		error(ERROR_PIPE);
	}
	++*c;
	skip_whitespace(c);

	if(!is_alpha(**c)){
		error(ERROR_IDENTIFIER_EXPECTED);
	}

	new_scope();
	do{
		if(**c == ','){
			++*c;
			skip_whitespace(c);
		}
		if((next_goal && next_goal->type != FORALL) || global_context->parent->goal->type != FORALL){
			error(ERROR_GOAL_TYPE);
		}
		if(get_identifier(c, name_buffer, 256)){
			error(ERROR_IDENTIFIER_LENGTH);
		}
		if(name_buffer[0] == '\0'){
			error(ERROR_IDENTIFIER_EXPECTED);
		}

		skip_whitespace(c);
		if(**c != '|' && **c != ','){
			error(ERROR_PIPE_OR_COMMA);
		}
		if(next_goal){
			temp_goal = next_goal->child0;
			free(next_goal);
			next_goal = temp_goal;
			next_goal->parent = NULL;
		} else {
			next_goal = malloc(sizeof(sentence));
			copy_sentence(next_goal, global_context->parent->goal->child0);
			next_goal->parent = NULL;
		}
		new_var = create_object_variable(name_buffer, global_context);
		substitute_variable(next_goal, new_var);
	} while(**c == ',');

	++*c;
	skip_whitespace(c);
	if(**c != '{' && **c != ';'){
		error(ERROR_BRACE_OR_SEMICOLON);
	}

	explicit_scope = (**c == '{');
	++*c;
	skip_whitespace(c);

	create_sentence_variable("goal", next_goal, 0, global_context);
	global_context->goal = malloc(sizeof(sentence));
	copy_sentence(global_context->goal, next_goal);
	global_context->goal->parent = NULL;

	return_value = parse_context(c);
	if(explicit_scope){
		if(**c != '}'){
			error(ERROR_END_BRACE);
		}
		++*c;
	}
	if(!return_value){
		error(ERROR_RETURN_EXPECTED);
	}
	if(return_value->type != SENTENCE){
		error(ERROR_RETURN_TYPE);
	}
	if(!return_value->verified){
		error(ERROR_RETURN_VERIFIED);
	}
	if(!sentence_stronger(return_value->sentence_data, global_context->goal)){
		error(ERROR_MISMATCHED_RETURN);
	}
	free_expr_value(return_value);
	drop_scope();

	output_value = create_expr_value(SENTENCE);
	output_value->sentence_data = malloc(sizeof(sentence));
	copy_sentence(output_value->sentence_data, global_context->goal);
	output_value->sentence_data->parent = NULL;
	output_value->verified = 1;

	return output_value;
}

expr_value *choose_command(char **c){
	unsigned char explicit_scope;
	sentence *next_goal = NULL;
	sentence *temp_goal;
	variable *var;
	expr_value *arg_value;
	expr_value *return_value;
	expr_value *output_value;

	skip_whitespace(c);
	if(strncmp(*c, "choose", 6) || is_alphanumeric((*c)[6])){
		return NULL;
	}

	*c += 6;
	skip_whitespace(c);

	if(!global_context->goal){
		error(ERROR_GOAL);
	}

	if(!is_alpha(**c)){
		error(ERROR_IDENTIFIER_EXPECTED);
	}

	new_scope();
	do{
		if(**c == ','){
			++*c;
			skip_whitespace(c);
		}
		if((next_goal && next_goal->type != EXISTS) || global_context->parent->goal->type != EXISTS){
			error(ERROR_GOAL_TYPE);
		}

		arg_value = parse_expr_value(c);
		if(arg_value->type != OBJECT){
			error(ERROR_ARGUMENT_TYPE);
		}
		var = arg_value->var;
		free_expr_value(arg_value);

		skip_whitespace(c);
		if(**c != '{' &&  **c != ';' && **c != ','){
			error(ERROR_BRACE_OR_SEMICOLON_OR_COMMA);
		}
		if(next_goal){
			temp_goal = next_goal->child0;
			free(next_goal);
			next_goal = temp_goal;
		} else {
			next_goal = malloc(sizeof(sentence));
			copy_sentence(next_goal, global_context->parent->goal->child0);
			next_goal->parent = NULL;
		}
		substitute_variable(next_goal, var);
	} while(**c == ',');

	explicit_scope = (**c == '{');

	++*c;
	skip_whitespace(c);

	create_sentence_variable("goal", next_goal, 0, global_context);
	global_context->goal = malloc(sizeof(sentence));
	copy_sentence(global_context->goal, next_goal);
	global_context->goal->parent = NULL;

	return_value = parse_context(c);
	if(explicit_scope){
		if(**c != '}'){
			error(ERROR_END_BRACE);
		}
		++*c;
	}
	if(!return_value){
		error(ERROR_RETURN_EXPECTED);
	}
	if(return_value->type != SENTENCE){
		error(ERROR_RETURN_TYPE);
	}
	if(!return_value->verified){
		error(ERROR_RETURN_VERIFIED);
	}
	if(!sentence_stronger(return_value->sentence_data, global_context->goal)){
		error(ERROR_MISMATCHED_RETURN);
	}
	free_expr_value(return_value);
	drop_scope();

	output_value = create_expr_value(SENTENCE);
	output_value->sentence_data = malloc(sizeof(sentence));
	copy_sentence(output_value->sentence_data, global_context->goal);
	output_value->sentence_data->parent = NULL;
	output_value->verified = 1;

	return output_value;
}

expr_value *implies_command(char **c){
	unsigned char explicit_scope;
	char name_buffer[256];
	sentence *next_goal;
	sentence *goal_child0_copy;
	sentence *var_sentence;
	expr_value *return_value;
	expr_value *output_value;

	skip_whitespace(c);
	if(strncmp(*c, "implies", 7) || is_alphanumeric((*c)[7])){
		return NULL;
	}
	*c += 7;
	skip_whitespace(c);

	if(!global_context->goal){
		error(ERROR_GOAL);
	}

	if(!is_alpha(**c)){
		error(ERROR_IDENTIFIER_EXPECTED);
	}

	if(global_context->goal->type != IMPLIES){
		error(ERROR_GOAL_TYPE);
	}

	goal_child0_copy = malloc(sizeof(sentence));
	copy_sentence(goal_child0_copy, global_context->goal->child0);
	goal_child0_copy->parent = NULL;

	new_scope();
	do{
		if(**c == ','){
			++*c;
			skip_whitespace(c);
		}
		if(get_identifier(c, name_buffer, 256)){
			error(ERROR_IDENTIFIER_LENGTH);
		}
		if(name_buffer[0] == '\0'){
			error(ERROR_IDENTIFIER_EXPECTED);
		}

		skip_whitespace(c);
		if(**c != '{' && **c != ';' && **c != ','){
			error(ERROR_BRACE_OR_SEMICOLON_OR_COMMA);
		}
		if(!goal_child0_copy){
			error(ERROR_TOO_MANY_UNPACK);
		}
		if(**c == '{' || **c == ';'){
			var_sentence = goal_child0_copy;
		} else {
			var_sentence = peel_and_left(&goal_child0_copy);
		}
		var_sentence->parent = NULL;
		create_sentence_variable(name_buffer, var_sentence, 1, global_context);
	} while(**c == ',');

	explicit_scope = (**c == '{');
	++*c;
	skip_whitespace(c);

	next_goal = malloc(sizeof(sentence));
	copy_sentence(next_goal, global_context->parent->goal->child1);
	next_goal->parent = NULL;
	create_sentence_variable("goal", next_goal, 0, global_context);
	global_context->goal = malloc(sizeof(sentence));
	copy_sentence(global_context->goal, next_goal);
	global_context->goal->parent = NULL;

	return_value = parse_context(c);
	if(explicit_scope){
		if(**c != '}'){
			error(ERROR_END_BRACE);
		}
		++*c;
	}
	if(!return_value){
		error(ERROR_RETURN_EXPECTED);
	}
	if(return_value->type != SENTENCE){
		error(ERROR_RETURN_TYPE);
	}
	if(!return_value->verified){
		error(ERROR_RETURN_VERIFIED);
	}
	if(!sentence_stronger(return_value->sentence_data, global_context->goal)){
		error(ERROR_MISMATCHED_RETURN);
	}
	free_expr_value(return_value);
	drop_scope();

	output_value = create_expr_value(SENTENCE);
	output_value->sentence_data = malloc(sizeof(sentence));
	copy_sentence(output_value->sentence_data, global_context->goal);
	output_value->sentence_data->parent = NULL;
	output_value->verified = 1;

	return output_value;
}

expr_value *not_command(char **c){
	unsigned char explicit_scope;
	char name_buffer[256];
	sentence *next_goal;
	sentence *var_sentence;
	expr_value *return_value;
	expr_value *output_value;

	skip_whitespace(c);
	if(strncmp(*c, "not", 3) || is_alphanumeric((*c)[3])){
		return NULL;
	}
	*c += 3;
	skip_whitespace(c);

	if(!global_context->goal){
		error(ERROR_GOAL);
	}

	if(global_context->goal->type != NOT){
		error(ERROR_GOAL_TYPE);
	}

	if(get_identifier(c, name_buffer, 256)){
		error(ERROR_IDENTIFIER_LENGTH);
	}
	if(name_buffer[0] == '\0'){
		error(ERROR_IDENTIFIER_EXPECTED);
	}

	skip_whitespace(c);
	if(**c != '{' && **c != ';'){
		error(ERROR_BRACE_OR_SEMICOLON);
	}

	explicit_scope = (**c == '{');
	++*c;
	skip_whitespace(c);

	new_scope();
	var_sentence = malloc(sizeof(sentence));
	copy_sentence(var_sentence, global_context->parent->goal->child0);
	var_sentence->parent = NULL;
	create_sentence_variable(name_buffer, var_sentence, 1, global_context);
	next_goal = create_sentence(FALSE, 0, 0);
	create_sentence_variable("goal", next_goal, 0, global_context);
	global_context->goal = malloc(sizeof(sentence));
	copy_sentence(global_context->goal, next_goal);
	global_context->goal->parent = NULL;
	return_value = parse_context(c);

	if(explicit_scope){
		if(**c != '}'){
			error(ERROR_END_BRACE);
		}
		++*c;
	}

	if(!return_value){
		error(ERROR_RETURN_EXPECTED);
	}
	if(return_value->type != SENTENCE){
		error(ERROR_RETURN_TYPE);
	}
	if(!return_value->verified){
		error(ERROR_RETURN_VERIFIED);
	}
	if(!sentence_stronger(return_value->sentence_data, global_context->goal)){
		error(ERROR_MISMATCHED_RETURN);
	}
	free_expr_value(return_value);
	drop_scope();

	output_value = create_expr_value(SENTENCE);
	output_value->sentence_data = malloc(sizeof(sentence));
	copy_sentence(output_value->sentence_data, global_context->goal);
	output_value->sentence_data->parent = NULL;
	output_value->verified = 1;

	return output_value;
}

variable *context_command(char **c){
	expr_value *return_value;
	variable *output_var;
	dependency *context_dep;
	dependency **old_dependencies;
	char name_buffer[256];
	char *beginning;
	unsigned char is_dependent;

	skip_whitespace(c);
	beginning = *c;
	if(strncmp(*c, "dependent", 9) || is_alphanumeric((*c)[9])){
		is_dependent = 0;
		if(strncmp(*c, "context", 7) || is_alphanumeric((*c)[7])){
			return NULL;
		}
	} else {
		is_dependent = 1;
		*c += 9;
		skip_whitespace(c);
		if(strncmp(*c, "context", 7) || is_alphanumeric((*c)[7])){
			*c = beginning;
			return NULL;
		}
	}
	*c += 7;
	skip_whitespace(c);

	if(!is_dependent && global_context->dependent){
		error(ERROR_DEPENDENT_SCOPE);
	}
	if(is_dependent && global_context->parent && !global_context->dependent){
		error(ERROR_GLOBAL_SCOPE);
	}

	if(get_identifier(c, name_buffer, 256)){
		error(ERROR_IDENTIFIER_LENGTH);
	}
	if(name_buffer[0] == '\0'){
		error(ERROR_IDENTIFIER_EXPECTED);
	}

	skip_whitespace(c);
	if(**c != '{'){
		error(ERROR_BEGIN_BRACE);
	}
	++*c;
	skip_whitespace(c);

	new_scope();
	global_context->dependent = is_dependent;
	output_var = create_context_variable(name_buffer, global_context, global_context->parent);
	if(is_dependent){
		context_dep = add_context_dependency(output_var);
		old_dependencies = global_dependencies;
		global_dependencies = &(context_dep->children);
	}
	return_value = parse_context(c);
	if(return_value){
		error(ERROR_UNEXPECTED_RETURN);
	}

	if(**c != '}'){
		error(ERROR_END_BRACE);
	}
	++*c;
	skip_whitespace(c);

	global_context = global_context->parent;
	if(is_dependent){
		global_dependencies = old_dependencies;
	}

	return output_var;
}

int import_command(char **c){
	char file_name[PATH_MAX];
	unsigned int file_name_size = 0;
	import_entry *entry;
	context *old_root_context;

	if(strncmp(*c, "import", 6) || is_alphanumeric((*c)[6])){
		return 0;
	}
	*c += 6;
	skip_whitespace(c);

	if(global_context->dependent){
		error(ERROR_DEPENDENT_SCOPE);
	}

	if(**c != '"'){
		error(ERROR_QUOTE);
	}
	++*c;

	while(**c != '"' && file_name_size < PATH_MAX - 1){
		file_name[file_name_size] = **c;
		++*c;
		file_name_size++;
	}
	if(**c != '"'){
		error(ERROR_PATH_SIZE);
	}
	file_name[file_name_size] = '\0';
	++*c;
	skip_whitespace(c);
	if(**c != ';'){
		error(ERROR_SEMICOLON);
	}
	entry = get_import_entry(file_name);
	old_root_context = global_root_context;
	global_root_context = global_context;
	reset_destinations(entry->import_context);
	check_dependencies(entry->dependencies);
	import_context(entry->import_context);
	global_root_context = old_root_context;
	++*c;

	return 1;
}

int include_command(char **c, expr_value **return_value){
	char file_name[PATH_MAX];
	char dirc[PATH_MAX];
	char basec[PATH_MAX];
	char working_directory[PATH_MAX];
	char *dname;
	char *bname;
	char *program_text;
	char *program_start;
	char *old_file_name;
	char **old_program_pointer;
	char *old_program_start;
	unsigned int old_line_number;
	unsigned int file_name_size = 0;

	if(strncmp(*c, "include", 7) || is_alphanumeric((*c)[7])){
		return 0;
	}
	*c += 7;
	skip_whitespace(c);
	if(**c != '"'){
		error(ERROR_QUOTE);
	}
	++*c;

	while(**c != '"' && file_name_size < PATH_MAX - 1){
		file_name[file_name_size] = **c;
		++*c;
		file_name_size++;
	}
	if(**c != '"'){
		error(ERROR_PATH_SIZE);
	}
	file_name[file_name_size] = '\0';
	++*c;
	skip_whitespace(c);
	if(**c != ';'){
		error(ERROR_SEMICOLON);
	}
	++*c;
	strcpy(dirc, file_name);
	strcpy(basec, file_name);
	dname = dirname(dirc);
	bname = basename(basec);
	if(!getcwd(working_directory, PATH_MAX)){
		error(ERROR_GET_WORKING_DIRECTORY);
	}
	if(chdir(dname)){
		error(ERROR_CHANGE_DIRECTORY);
	}

	old_file_name = global_file_name;
	old_program_pointer = global_program_pointer;
	old_program_start = global_program_start;
	old_line_number = global_line_number;

	program_start = load_file(bname);
	if(!program_start){
		error(ERROR_FILE_READ);
	}
	program_text = program_start;

	global_file_name = bname;
	global_line_number = 1;
	global_program_pointer = &program_text;
	global_program_start = program_start;
	*return_value = parse_context(&program_text);
	if(*program_text == '}'){
		error(ERROR_EOF);
	}
	free(program_start);

	if(chdir(working_directory)){
		error(ERROR_CHANGE_DIRECTORY);
	}
	global_file_name = old_file_name;
	global_program_pointer = old_program_pointer;
	global_program_start = old_program_start;
	global_line_number = old_line_number;

	skip_whitespace(c);
	if(**c && *return_value){
		error(ERROR_EOF);
	}

	return 1;
}

expr_value *parse_command(char **c){
	definition *def;
	variable *axiom;
	variable *var;
	expr_value *return_value;

	if((def = define_command(c))){
		//pass
	} else if((axiom = axiom_command(c))){
		//pass
	} else if(print_command(c)){
		//pass
	} else if((return_value = return_command(c))){
		return return_value;
	} else if(object_command(c)){
		//pass
	} else if(relation_command(c)){
		//pass
	} else if(debug_command(c)){
		//pass
	} else if((var = context_command(c))){
		//pass
	} else if(import_command(c)){
		//pass
	} else if(include_command(c, &return_value)){
		if(return_value){
			return return_value;
		}
	} else if((var = prove_command(c))){
		//pass
	} else if((return_value = given_command(c))){
		return return_value;
	} else if((return_value = choose_command(c))){
		return return_value;
	} else if((return_value = implies_command(c))){
		return return_value;
	} else if((return_value = not_command(c))){
		return return_value;
	} else if(assign_command(c)){
		//pass
	} else {
		if(global_context->dependent){
			error(ERROR_DEPENDENT_SCOPE);
		}
		evaluate_command(c);
	}

	return NULL;
}

expr_value *parse_context(char **c){
	expr_value *return_value = NULL;

	while(**c && **c != '}' && !return_value){
		return_value = parse_command(c);
		skip_whitespace(c);
	}

	if(return_value && **c && **c != '}'){
		error(ERROR_BRACE_OR_EOF);
	}

	return return_value;
}

