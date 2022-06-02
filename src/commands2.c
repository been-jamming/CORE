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

	if((def = read_dictionary(parent_context->definitions, name, 0))){
		if(def->num_references == 0){
			free_definition(def);
		} else {
			error(ERROR_DUPLICATE_DEFINITION);
		}
	}
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
	if(sentence_data){
		rel->sentence_data = malloc(sizeof(sentence));
		copy_sentence(rel->sentence_data, sentence_data);
	} else {
		rel->sentence_data = NULL;
	}
	rel->name = malloc(sizeof(char)*(strlen(name) + 1));
	strcpy(rel->name, name);
	rel->num_references = 0;
	write_dictionary(&(parent_context->relations), name, rel, 0);

	return rel;
}

static char *load_file(char *file_name){
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

	if(global_context->parent){
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
	char *name;
	int *new_int;
	sentence *s;

	skip_whitespace(c);
	if(strncmp(*c, "relation", 8) || is_alphanumeric((*c)[8])){
		return 0;
	}
	*c += 8;
	skip_whitespace(c);
	if(get_relation_identifier(c, identifier0, 256)){
		error(ERROR_IDENTIFIER_LENGTH);
	}
	skip_whitespace(c);
	if(**c == ';'){
		if(identifier0[0] == '\0'){
			error(ERROR_RELATION_IDENTIFIER);
		}
		++*c;

		if(read_dictionary(global_context->relations, identifier0, 0)){
			error(ERROR_DUPLICATE_RELATION);
		}
		if(global_context->parent){
			error(ERROR_RELATION_CONTEXT);
		}
		create_relation(identifier0, NULL, global_context);
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
		create_relation(identifier1, s, global_context);
		return 1;
	}
}

expr_value *return_command(char **c){
	expr_value *output;
	expr_value *arg;
	sentence *arg_sentence;
	sentence *output_sentence;
	sentence *last_parent = NULL;
	sentence *s;
	sentence *new;

	skip_whitespace(c);
	if(strncmp(*c, "return", 6) || is_alphanumeric((*c)[6])){
		return NULL;
	}
	*c += 6;
	skip_whitespace(c);

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

	skip_whitespace(c);
	if(strncmp(*c, "debug", 5) || is_alphanumeric((*c)[5])){
		return 0;
	}
	*c += 5;
	skip_whitespace(c);
	val = parse_expr_value(c);
	printf("DEBUG: ");
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
	global_context->goal = goal;
	returned = parse_context(c);
	global_context->goal = NULL;

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

expr_value *parse_command(char **c){
	definition *def;
	variable *axiom;
	variable *var;
	expr_value *val;
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
	} else if((var = prove_command(c))){
		//pass
	} else if(assign_command(c)){
		//pass
	} else {
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

int main(int argc, char **argv){
	char *program_start;
	char *program_text;
	expr_value *return_value;
	int i;

	if(argc < 2){
		fprintf(stderr, "Error: no input files\n");
		return 1;
	}

#ifdef USE_CUSTOM_ALLOC
	custom_malloc_init();
#endif

	init_verifier();

	for(i = 1; i < argc; i++){
		program_start = load_file(argv[i]);
		program_text = program_start;
		if(!program_text){
			fprintf(stderr, "Error: could not read file '%s'.", argv[i]);
#ifdef USE_CUSTOM_ALLOC
			custom_malloc_abort();
#endif
			return 1;
		}
		global_line_number = 1;
		global_file_name = argv[i];
		global_program_pointer = &program_text;
		global_program_start = program_text;
		return_value = parse_context(&program_text);
		if(*program_text == '}'){
			error(ERROR_EOF);
		}
		if(return_value){
			printf("file '%s' returned: ", global_file_name);
			print_expr_value(return_value);
			printf("\n");
			free_expr_value(return_value);
		}
		free(program_start);
	}
	free_context(global_context);
	clear_bound_variables();
	clear_bound_propositions();

#ifdef USE_CUSTOM_ALLOC
	custom_malloc_deinit();
#endif

	printf("Proof verification successful.\n");
	return 0;
}
