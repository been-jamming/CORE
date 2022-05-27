#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "predicate.h"
#include "expression2.h"
#include "custom_malloc.h"

//CORE Command Parser
//Ben Jones

void init_verifier(void){
	int i;

	global_context = create_context(NULL);
}

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

	def = malloc(sizeof(def));
	if(sentence_data){
		def->sentence_data = malloc(sizeof(sentence));
		copy_sentence(def->sentence_data, sentence_data);
	} else {
		def->sentence_data = NULL;
	}
	def->name = malloc(sizeof(char)*(strlen(name) + 1));
	strcpy(def->name, name);
	def->num_args = num_args;
	def->num_references = 0;
	write_dictionary(&(parent_context->definitions), name, def, 0);

	return def;
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

	skip_whitespace(c);
	if(strncmp(*c, "object", 6) || is_alphanumeric((*c)[6])){
		return 0;
	}
	*c += 6;
	skip_whitespace(c);

	if(get_identifier(c, var_name, 256)){
		error(ERROR_IDENTIFIER_LENGTH);
	}
	if(var_name[0] == '\0'){
		error(ERROR_IDENTIFIER_EXPECTED);
	}

	create_object_variable(var_name, global_context);

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

		create_object_variable(var_name, global_context);
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

	skip_whitespace(c);
	if(strncmp(*c, "define", 6) || is_alphanumeric((*c)[6])){
		return NULL;
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
	if(**c == ';'){
		++*c;
		output = create_definition(def_name, NULL, num_bound_vars, global_context);
	} else if(**c == ':'){
		++*c;
		s = parse_sentence(c, num_bound_vars, 0);
		if(**c != ';'){
			error(ERROR_SEMICOLON);
		}
		++*c;
		output = create_definition(def_name, s, num_bound_vars, global_context);
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
		while(**c == ']'){
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
	return output;
}

