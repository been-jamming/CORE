#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "expr.h"
#include "verify.h"

void init_verifier(){
	int i;

	current_depth = 0;
	goal_depth = 0;
	bound_variables = create_dictionary(NULL);
	bound_propositions = create_dictionary(NULL);

	for(i = 0; i < MAX_DEPTH; i++){
		variables[i] = create_dictionary(NULL);
		definitions[i] = create_dictionary(NULL);
		goals[i] = NULL;
	}
}

void up_scope(){
	if(current_depth < MAX_DEPTH - 1){
		current_depth++;
	} else {
		fprintf(stderr, "Error: maximum scope depth reached\n");
		error(1);
	}
}

void drop_scope(){
	if(current_depth){
		iterate_dictionary(variables[current_depth], variable_decrement_references_void);
		iterate_dictionary(definitions[current_depth], proposition_decrement_references_void);
		free_dictionary(variables + current_depth, free_variable_void);
		free_dictionary(definitions + current_depth, free_proposition_void);
		current_depth--;
	} else {
		fprintf(stderr, "WARNING: drop scope called when scope was 0\n");
	}
}

void clear_bound_variables(){
	free_dictionary(&bound_variables, free);
}

void clear_bound_propositions(){
	free_dictionary(&bound_propositions, free);
}

char *load_file(char *file_name){
	FILE *fp;
	size_t size;
	char *output;

	fp = fopen(file_name, "r");
	if(!fp){
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	output = malloc(sizeof(char)*(size + 1));
	if(fread(output, sizeof(char), size, fp) < size){
		free(output);
		return NULL;
	}
	fclose(fp);
	output[size] = '\0';

	return output;
}

void write_goal(statement *goal){
	statement *goal_copy;
	proposition *goal_prop;

	goal_copy = malloc(sizeof(statement));
	copy_statement(goal_copy, goal);
	goal_prop = malloc(sizeof(proposition));
	goal_prop->name = malloc(sizeof(char)*(strlen("goal") + 1));
	strcpy(goal_prop->name, "goal");
	goal_prop->num_references = 1;
	goal_prop->depth = current_depth;
	goal_prop->num_args = 0;
	goal_prop->statement_data = goal_copy;

	write_dictionary(definitions + current_depth, "goal", goal_prop, 0);
}

int print_command(char **c){
	statement *s;
	unsigned char is_verified;

	if(strncmp(*c, "print", 5) || is_alphanumeric((*c)[5])){
		return 0;
	}
	*c += 5;
	skip_whitespace(c);
	s = parse_statement_value(c, &is_verified);
	if(!s){
		fprintf(stderr, "Error: could not parse statement value\n");
		error(1);
	}
	if(is_verified){
		printf("'%s' line %d   (verified): ", global_file_name, line_number);
	} else {
		printf("'%s' line %d (unverified): ", global_file_name, line_number);
	}
	print_statement(s);
	printf("\n");
	free_statement(s);
	skip_whitespace(c);
	if(**c != ';'){
		fprintf(stderr, "Error: expected ';'\n");
		error(1);
	}
	++*c;

	return 1;
}

proposition *definition_command(char **c){
	int num_bound_vars = 0;
	int *new_int;
	char name_buffer[256];
	char *def_name;
	statement *s;
	proposition *output;

	clear_bound_variables();
	skip_whitespace(c);
	if(strncmp(*c, "define", 6) || is_alphanumeric((*c)[6])){
		return NULL;
	}
	*c += 6;
	skip_whitespace(c);
	
	get_identifier(c, name_buffer, 256);
	if(name_buffer[0] == '\0'){
		fprintf(stderr, "Error: expected identifier\n");
		error(1);
	}

	def_name = malloc(sizeof(char)*(strlen(name_buffer) + 1));
	strcpy(def_name, name_buffer);

	skip_whitespace(c);

	if(**c == '('){
		++*c;
		skip_whitespace(c);
		while(**c != ')'){
			get_identifier(c, name_buffer, 256);
			if(name_buffer[0] == '\0'){
				free(def_name);
				fprintf(stderr, "Error: expected identifier\n");
				error(1);
			}
			if(read_dictionary(bound_variables, name_buffer, 0)){
				fprintf(stderr, "Error: duplicate identifier\n");
				error(1);
			}
			new_int = malloc(sizeof(int));
			*new_int = num_bound_vars;
			write_dictionary(&bound_variables, name_buffer, new_int, 0);
			num_bound_vars++;
			skip_whitespace(c);
			if(**c == ','){
				++*c;
				skip_whitespace(c);
			} else if(**c != ')'){
				free(def_name);
				fprintf(stderr, "Error: expected ',' or ')'\n");
				error(1);
			}
		}
		++*c;
	}
	skip_whitespace(c);
	if(**c != ':'){
		free(def_name);
		fprintf(stderr, "Error: expected ':'\n");
		error(1);
	}
	++*c;
	s = parse_statement(c, num_bound_vars, 0);
	if(**c != ';'){
		free(def_name);
		free_statement(s);
		fprintf(stderr, "Error: expected ';'\n");
		error(1);
	}
	++*c;
	output = malloc(sizeof(proposition));
	output->name = def_name;
	output->statement_data = s;
	output->num_references = 0;
	output->depth = current_depth;
	output->num_args = num_bound_vars;

	clear_bound_variables();

	return output;
}

variable *axiom_command(char **c){
	int num_bound_props = 0;
	int num_args;
	bound_proposition *new_bound_prop;
	char name_buffer[256];
	char *axiom_name;
	char *int_end;
	variable *output;
	statement *s;

	clear_bound_propositions();
	skip_whitespace(c);
	if(strncmp(*c, "axiom", 5) || is_alphanumeric((*c)[5])){
		return NULL;
	}

	*c += 5;
	skip_whitespace(c);

	get_identifier(c, name_buffer, 256);
	if(name_buffer[0] == '\0'){
		fprintf(stderr, "Error: expected identifier\n");
		error(1);
	}

	axiom_name = malloc(sizeof(char)*(strlen(name_buffer) + 1));
	strcpy(axiom_name, name_buffer);

	skip_whitespace(c);

	if(**c == '['){
		++*c;
		skip_whitespace(c);
		while(**c != ']'){
			get_identifier(c, name_buffer, 256);
			if(name_buffer[0] == '\0'){
				free(axiom_name);
				fprintf(stderr, "Error: expected identifier\n");
				error(1);
			}
			if(read_dictionary(bound_propositions, name_buffer, 0)){
				free(axiom_name);
				fprintf(stderr, "Error: duplicate identifier\n");
				error(1);
			}
			new_bound_prop = malloc(sizeof(bound_proposition));
			skip_whitespace(c);
			if(**c == '('){
				++*c;
				skip_whitespace(c);
				if(**c != ')' && !is_digit(**c)){
					free(axiom_name);
					free(new_bound_prop);
					fprintf(stderr, "Error: expected number of arguments\n");
					error(1);
				} else if(is_digit(**c)){
					num_args = strtol(*c, &int_end, 10);
					*c = int_end;
					if(num_args < 0){
						free(axiom_name);
						free(new_bound_prop);
						fprintf(stderr, "Error: expected a positive number of arguments\n");
						error(1);
					}
					skip_whitespace(c);
					if(**c != ')'){
						free(axiom_name);
						free(new_bound_prop);
						fprintf(stderr, "Error: expected ')'\n");
						error(1);
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
			write_dictionary(&bound_propositions, name_buffer, new_bound_prop, 0);
			num_bound_props++;
			skip_whitespace(c);
			if(**c == ','){
				++*c;
				skip_whitespace(c);
			} else if(**c != ']'){
				free(axiom_name);
				fprintf(stderr, "Error: expected ',' or ']'\n");
				error(1);
			}
		}
		++*c;
	}
	skip_whitespace(c);
	if(**c != ':'){
		free(axiom_name);
		fprintf(stderr, "Error: expected ':'\n");
		error(1);
	}
	++*c;
	s = parse_statement(c, 0, num_bound_props);
	skip_whitespace(c);
	if(**c != ';'){
		free(axiom_name);
		free_statement(s);
		fprintf(stderr, "Error: expected ';'\n");
		error(1);
	}
	++*c;

	clear_bound_propositions();

	output = malloc(sizeof(variable));
	output->name = axiom_name;
	output->type = STATEMENT;
	output->statement_data = s;
	output->num_references = 0;
	output->depth = current_depth;

	return output;
}

variable *assign_command(char **c){
	char name_buffer[256];
	unsigned char is_verified;
	char *beginning;
	statement *s;
	variable *output;

	skip_whitespace(c);
	beginning = *c;
	get_identifier(c, name_buffer, 256);
	if(name_buffer[0] == '\0'){
		*c = beginning;
		return NULL;
	}

	skip_whitespace(c);
	if(**c != '='){
		*c = beginning;
		return NULL;
	}
	++*c;
	skip_whitespace(c);
	
	s = parse_statement_value(c, &is_verified);
	if(!s){
		fprintf(stderr, "Error: could not parse statement value\n");
		error(1);
	}
	skip_whitespace(c);
	if(**c != ';'){
		free_statement(s);
		fprintf(stderr, "Error: expected ';'\n");
		error(1);
	}
	if(!is_verified){
		free_statement(s);
		fprintf(stderr, "Error: statement must be verified\n");
		error(1);
	}
	++*c;

	output = create_statement_var(name_buffer, s);
	
	return output;
}

statement *return_command(char **c){
	unsigned char is_verified;
	statement *output;

	skip_whitespace(c);
	if(strncmp(*c, "return", 6) || is_alphanumeric((*c)[6])){
		return NULL;
	}

	*c += 6;
	skip_whitespace(c);
	output = parse_statement_value(c, &is_verified);
	if(!output){
		fprintf(stderr, "Error: could not parse statement value\n");
		error(1);
	}
	skip_whitespace(c);
	if(**c != ';'){
		free_statement(output);
		fprintf(stderr, "Error: expected ';'\n");
		error(1);
	}
	if(!is_verified){
		free_statement(output);
		fprintf(stderr, "Error: statement must be verified\n");
		error(1);
	}
	++*c;

	return output;
}

int evaluate_command(char **c){
	unsigned char is_verified;
	statement *s;
	char *beginning;

	skip_whitespace(c);
	beginning = *c;
	s = parse_statement_value(c, &is_verified);
	if(!s){
		*c = beginning;
		return 0;
	}

	skip_whitespace(c);
	if(**c != ';'){
		free_statement(s);
		fprintf(stderr, "Error: expected ';'\n");
		error(1);
	}
	if(!is_verified){
		free_statement(s);
		fprintf(stderr, "Error: statement must be verified\n");
		error(1);
	}
	++*c;
	free_statement(s);

	return 1;
}

int debug_command(char **c, statement *goal){
	unsigned char is_verified;
	statement *s;
	statement *t;

	skip_whitespace(c);
	if(strncmp(*c, "debug", 5) || is_alphanumeric((*c)[5])){
		return 0;
	}

	*c += 5;
	skip_whitespace(c);
	s = parse_statement_value(c, &is_verified);
	if(!s){
		fprintf(stderr, "Error: could not parse statement value\n");
		error(1);
	}

	skip_whitespace(c);
	if(**c != ';'){
		free_statement(s);
		fprintf(stderr, "Error: expected ';'\n");
		error(1);
	}
	++*c;
	//Do stuff here
	t = peel_and_left(&s);
	print_statement(t);
	printf(" -- ");
	print_statement(s);
	printf("\n");
	
	free_statement(s);
	free_statement(t);

	return 1;
}

int extract_command(char **c){
	unsigned char is_verified;
	char var_name[256];
	statement *all;
	statement *extracted;

	skip_whitespace(c);
	if(strncmp(*c, "extract", 7) || is_alphanumeric((*c)[7])){
		return 0;
	}

	*c += 7;
	skip_whitespace(c);
	all = parse_statement_value(c, &is_verified);
	if(!all){
		fprintf(stderr, "Error: could not parse statement value\n");
		error(1);
	}
	if(!is_verified){
		fprintf(stderr, "Error: statement must be verified\n");
		error(1);
	}

	skip_whitespace(c);
	if(**c != ','){
		fprintf(stderr, "Error: expected ','\n");
		error(1);
	}

	while(**c == ','){
		++*c;
		skip_whitespace(c);
		get_identifier(c, var_name, 256);
		skip_whitespace(c);

		if(var_name[0] == '\0'){
			fprintf(stderr, "Error: expected identifier\n");
			error(1);
		}
		if(!all){
			fprintf(stderr, "Error: not enough values to unpack\n");
			error(1);
		}

		if(**c != ',' && **c != ';'){
			fprintf(stderr, "Error: expected ',' or ';'\n");
			error(1);
		} else if(**c == ';'){
			create_statement_var(var_name, all);
		} else {
			extracted = peel_and_left(&all);
			create_statement_var(var_name, extracted);
		}
	}

	++*c;
	return 1;
}

variable *prove_command(char **c){
	int num_bound_props = 0;
	int num_args;
	int i;
	int j;
	bound_proposition *new_bound_prop;
	proposition **prop_list = NULL;
	unsigned int prop_list_size;
	char name_buffer[256];
	char *proof_name;
	char *int_end;
	variable *output;
	statement *result;
	statement *goal;
	statement *returned;
	statement prop_statement;

	clear_bound_propositions();
	skip_whitespace(c);
	if(strncmp(*c, "prove", 5) || is_alphanumeric((*c)[5])){
		return NULL;
	}

	*c += 5;
	skip_whitespace(c);

	get_identifier(c, name_buffer, 256);
	if(name_buffer[0] == '\0'){
		fprintf(stderr, "Error: expected identifier\n");
		error(1);
	}

	proof_name = malloc(sizeof(char)*(strlen(name_buffer) + 1));
	strcpy(proof_name, name_buffer);
	up_scope();

	skip_whitespace(c);

	if(**c == '['){
		prop_list = malloc(sizeof(proposition *)*8);
		prop_list_size = 8;
		++*c;
		skip_whitespace(c);
		while(**c != ']'){
			get_identifier(c, name_buffer, 256);
			if(name_buffer[0] == '\0'){
				free(proof_name);
				fprintf(stderr, "Error: expected identifier\n");
				error(1);
			}
			if(read_dictionary(bound_propositions, name_buffer, 0)){
				free(proof_name);
				fprintf(stderr, "Error: duplicate identifier\n");
				error(1);
			}
			if(num_bound_props >= prop_list_size){
				prop_list_size += 8;
				prop_list = realloc(prop_list, sizeof(proposition)*prop_list_size);
				if(!prop_list){
					free(proof_name);
					fprintf(stderr, "Error: could not expand proposition list\n");
					error(1);
				}
			}
			prop_list[num_bound_props] = malloc(sizeof(proposition));
			new_bound_prop = malloc(sizeof(bound_proposition));
			skip_whitespace(c);
			if(**c == '('){
				++*c;
				skip_whitespace(c);
				if(**c != ')' && !is_digit(**c)){
					free(proof_name);
					free(new_bound_prop);
					fprintf(stderr, "Error: expected number of arguments\n");
					error(1);
				} else if(is_digit(**c)){
					num_args = strtol(*c, &int_end, 10);
					*c = int_end;
					if(num_args < 0){
						free(proof_name);
						free(new_bound_prop);
						fprintf(stderr, "Error: expected a positive number of arguments\n");
						error(1);
					}
					skip_whitespace(c);
					if(**c != ')'){
						free(proof_name);
						free(new_bound_prop);
						fprintf(stderr, "Error: expected ')'\n");
						error(1);
					}
					++*c;
				} else {
					++*c;
					num_args = 0;
				}
			} else {
				num_args = 0;
			}
			prop_list[num_bound_props]->name = malloc(sizeof(char)*(strlen(name_buffer) + 1));
			strcpy(prop_list[num_bound_props]->name, name_buffer);
			prop_list[num_bound_props]->statement_data = NULL;
			prop_list[num_bound_props]->num_references = 1;
			prop_list[num_bound_props]->depth = current_depth;
			prop_list[num_bound_props]->num_args = num_args;

			new_bound_prop->num_args = num_args;
			new_bound_prop->prop_id = num_bound_props;
			write_dictionary(&bound_propositions, name_buffer, new_bound_prop, 0);

			num_bound_props++;
			skip_whitespace(c);
			if(**c == ','){
				++*c;
				skip_whitespace(c);
			} else if(**c != ']'){
				free(proof_name);
				fprintf(stderr, "Error: expected ',' or ']'\n");
				error(1);
			}
		}
		++*c;
	} else {
		prop_list_size = 0;
	}

	for(i = 0; i < num_bound_props; i++){
		write_dictionary(definitions + current_depth, prop_list[i]->name, prop_list[i], 0);
	}
	skip_whitespace(c);
	if(**c != ':'){
		free(proof_name);
		fprintf(stderr, "Error: expected ':'\n");
		error(1);
	}
	++*c;
	result = parse_statement(c, 0, num_bound_props);
	skip_whitespace(c);
	if(**c != '{'){
		free(proof_name);
		free_statement(result);
		fprintf(stderr, "Error: expected '{'\n");
		error(1);
	}
	++*c;
	skip_whitespace(c);

	goal = malloc(sizeof(statement));
	copy_statement(goal, result);
	for(i = 0; i < num_bound_props; i++){
		prop_statement.type = PROPOSITION;
		prop_statement.num_bound_vars = prop_list[i]->num_args;
		prop_statement.num_bound_props = 0;
		prop_statement.prop_args = malloc(sizeof(proposition_arg)*(prop_list[i]->num_args));
		for(j = 0; j < prop_list[i]->num_args; j++){
			prop_statement.prop_args[j].is_bound = 1;
			prop_statement.prop_args[j].var_id = j;
		}
		prop_statement.is_bound = 0;
		prop_statement.prop = prop_list[i];
		prop_statement.num_args = prop_list[i]->num_args;
		substitute_proposition(goal, &prop_statement);
		free(prop_statement.prop_args);
	}

	clear_bound_propositions();
	write_goal(goal);
	returned = verify_block(c, 1, goal);
	
	if(!returned){
		free_statement(goal);
		free_statement(result);
		free(proof_name);
		free(prop_list);
		fprintf(stderr, "Error: expected returned statement\n");
		error(1);
	}

	if(!compare_statement(goal, returned)){
		free_statement(returned);
		free_statement(goal);
		free_statement(result);
		free(proof_name);
		free(prop_list);
		fprintf(stderr, "Error: returned statement does not match goal\n");
		error(1);
	}
	free_statement(returned);
	if(goal != returned){
		free_statement(goal);
	}
	drop_scope();
	free(prop_list);

	clear_bound_propositions();

	output = malloc(sizeof(variable));
	output->name = proof_name;
	output->type = STATEMENT;
	output->statement_data = result;
	output->num_references = 0;
	output->depth = current_depth;

	return output;
}

statement *given_command(char **c, statement *goal){
	char name_buffer[256];
	statement *next_goal;
	statement *return_statement;
	variable *new_var;

	skip_whitespace(c);
	if(strncmp(*c, "given", 5) || is_alphanumeric((*c)[5])){
		return NULL;
	}

	*c += 5;
	skip_whitespace(c);

	if(goal->type != FORALL){
		fprintf(stderr, "Error: incorrect goal type\n");
		error(1);
	}

	get_identifier(c, name_buffer, 256);
	if(name_buffer[0] == '\0'){
		fprintf(stderr, "Error: expected identifier\n");
		error(1);
	}

	skip_whitespace(c);
	if(**c != '{'){
		fprintf(stderr, "Error: expected '{'\n");
		error(1);
	}

	++*c;
	skip_whitespace(c);

	up_scope();
	next_goal = malloc(sizeof(statement));
	copy_statement(next_goal, goal->child0);

	new_var = create_object_var(name_buffer);
	if(!substitute_variable(next_goal, 0, new_var)){
		fprintf(stderr, "Error: cannot substitute variable\n");
		error(1);
	}
	add_bound_variables(next_goal, -1);
	write_goal(next_goal);
	return_statement = verify_block(c, 1, next_goal);

	if(!return_statement){
		fprintf(stderr, "Error: expected returned statement\n");
		error(1);
	}

	if(!compare_statement(next_goal, return_statement)){
		fprintf(stderr, "Error: returned statement does not match goal\n");
		error(1);
	}
	free_statement(return_statement);
	if(return_statement != next_goal){
		free_statement(next_goal);
	}
	drop_scope();

	return goal;
}

statement *choose_command(char **c, statement *goal){
	char name_buffer[256];
	statement *next_goal;
	statement *return_statement;
	variable *var;

	skip_whitespace(c);
	if(strncmp(*c, "choose", 6) || is_alphanumeric((*c)[6])){
		return NULL;
	}

	*c += 6;
	skip_whitespace(c);

	if(goal->type != EXISTS){
		fprintf(stderr, "Error: incorrect goal type\n");
		error(1);
	}

	get_identifier(c, name_buffer, 256);
	if(name_buffer[0] == '\0'){
		fprintf(stderr, "Error: expected identifier\n");
		error(1);
	}

	skip_whitespace(c);
	if(**c != '{'){
		fprintf(stderr, "Error: expected '{'\n");
		error(1);
	}
	
	++*c;
	skip_whitespace(c);

	var = get_object_var(name_buffer);
	if(!var){
		fprintf(stderr, "Error: unknown variable '%s'\n", name_buffer);
		error(1);
	}

	up_scope();
	next_goal = malloc(sizeof(statement));
	copy_statement(next_goal, goal->child0);

	if(!substitute_variable(next_goal, 0, var)){
		fprintf(stderr, "Error: cannot substitute variable\n");
		error(1);
	}
	add_bound_variables(next_goal, -1);
	write_goal(next_goal);
	return_statement = verify_block(c, 1, next_goal);

	if(!return_statement){
		fprintf(stderr, "Error: expected returned statement\n");
		error(1);
	}

	if(!compare_statement(next_goal, return_statement)){
		fprintf(stderr, "Error: returned statement does not match goal\n");
		error(1);
	}
	free_statement(return_statement);
	if(return_statement != next_goal){
		free_statement(next_goal);
	}
	drop_scope();

	return goal;
}

statement *implies_command(char **c, statement *goal){
	char name_buffer[256];
	statement *next_goal;
	statement *var_statement;
	statement *return_statement;

	skip_whitespace(c);
	if(strncmp(*c, "implies", 7) || is_alphanumeric((*c)[7])){
		return NULL;
	}

	*c += 7;
	skip_whitespace(c);

	if(goal->type != IMPLIES){
		fprintf(stderr, "Error: incorrect goal type\n");
		error(1);
	}

	get_identifier(c, name_buffer, 256);
	if(name_buffer[0] == '\0'){
		fprintf(stderr, "Error: expected identifier\n");
		error(1);
	}

	skip_whitespace(c);
	if(**c != '{'){
		fprintf(stderr, "Error: expected '{'\n");
		error(1);
	}

	++*c;
	skip_whitespace(c);

	up_scope();
	var_statement = malloc(sizeof(statement));
	copy_statement(var_statement, goal->child0);
	create_statement_var(name_buffer, var_statement);
	next_goal = malloc(sizeof(statement));
	copy_statement(next_goal, goal->child1);
	write_goal(next_goal);
	return_statement = verify_block(c, 1, next_goal);

	if(!return_statement){
		fprintf(stderr, "Error: expected returned statement\n");
		error(1);
	}

	if(!compare_statement(next_goal, return_statement)){
		fprintf(stderr, "Error: returned statement does not match goal\n");
		error(1);
	}
	free_statement(return_statement);
	if(return_statement != next_goal){
		free_statement(next_goal);
	}
	drop_scope();

	return goal;
}

statement *not_command(char **c, statement *goal){
	char name_buffer[256];
	statement *next_goal;
	statement *var_statement;
	statement *return_statement;

	skip_whitespace(c);
	if(strncmp(*c, "not", 3) || is_alphanumeric((*c)[3])){
		return NULL;
	}

	*c += 3;
	skip_whitespace(c);

	if(goal->type != NOT){
		fprintf(stderr, "Error: incorrect goal type\n");
		error(1);
	}

	get_identifier(c, name_buffer, 256);
	if(name_buffer[0] == '\0'){
		fprintf(stderr, "Error: expected identifier\n");
		error(1);
	}

	skip_whitespace(c);
	if(**c != '{'){
		fprintf(stderr, "Error: expected '{'\n");
		error(1);
	}

	++*c;
	skip_whitespace(c);

	up_scope();
	var_statement = malloc(sizeof(statement));
	copy_statement(var_statement, goal->child0);
	create_statement_var(name_buffer, var_statement);
	next_goal = create_statement(FALSE, 0, 0);
	write_goal(next_goal);
	return_statement = verify_block(c, 1, next_goal);

	if(!return_statement){
		fprintf(stderr, "Error: expected returned statement\n");
		error(1);
	}

	if(!compare_statement(next_goal, return_statement)){
		fprintf(stderr, "Error: returned statement does not match goal\n");
		error(1);
	}
	free_statement(return_statement);
	if(return_statement != next_goal){
		free_statement(next_goal);
	}
	drop_scope();

	return goal;
}

statement *verify_command(char **c, unsigned char allow_proof_value, statement *goal){
	proposition *def;
	proposition *other_def;
	variable *axiom;
	variable *other_proof;
	variable *var;
	variable *proof;
	statement *return_value;

	if((def = definition_command(c))){
		if((other_def = read_dictionary(definitions[current_depth], def->name, 0))){
			if(other_def->num_references){
				fprintf(stderr, "Error: cannot overwrite bound definition\n");
				error(1);
			} else {
				free_proposition(other_def);
			}
		}
		write_dictionary(definitions + current_depth, def->name, def, 0);
	} else if((axiom = axiom_command(c))){
		if((other_proof = read_dictionary(variables[current_depth], axiom->name, 0))){
			if(other_proof->num_references){
				fprintf(stderr, "Error: cannot overwrite bound proof\n");
				error(1);
			} else {
				free_variable(other_proof);
			}
		}
		write_dictionary(variables + current_depth, axiom->name, axiom, 0);
	} else if(print_command(c)){
		//Pass
	} else if((var = assign_command(c))){
		//Pass
	} else if((return_value = return_command(c))){
		return return_value;
	} else if((proof = prove_command(c))){
		if((other_proof = read_dictionary(variables[current_depth], proof->name, 0))){
			if(other_proof->num_references){
				fprintf(stderr, "Error: cannot overwrite bound proof\n");
				error(1);
			} else {
				free_variable(other_proof);
			}
		}
		write_dictionary(variables + current_depth, proof->name, proof, 0);
	} else if(allow_proof_value && (return_value = given_command(c, goal))){
		return return_value;
	} else if(allow_proof_value && (return_value = choose_command(c, goal))){
		return return_value;
	} else if(allow_proof_value && (return_value = implies_command(c, goal))){
		return return_value;
	} else if(allow_proof_value && (return_value = not_command(c, goal))){
		return return_value;
	} else if(debug_command(c, goal)){
		//pass
	} else if(extract_command(c)){
		//pass
	} else if(evaluate_command(c)){
		//Pass
	} else {
		fprintf(stderr, "Error: unknown command\n");
		error(1);
	}

	return NULL;
}

statement *verify_block(char **c, unsigned char allow_proof_value, statement *goal){
	statement *return_value = NULL;

	skip_whitespace(c);
	while(**c && **c != '}' && !return_value){
		return_value = verify_command(c, allow_proof_value, goal);
		skip_whitespace(c);
	}

	if(return_value && **c && **c != '}'){
		free_statement(return_value);
		fprintf(stderr, "Error: expected '}' or EOF\n");
		error(1);
	}

	if(**c){
		++*c;
	}

	return return_value;
}

int main(int argc, char **argv){
	char *program_start;
	statement *return_value;
	int i;

	init_verifier();
	
	for(i = 1; i < argc; i++){
		program_text = load_file(argv[i]);
		program_start = program_text;
		if(!program_text){
			fprintf(stderr, "Error: could not read file\n");
			return 1;
		}

		line_number = 1;
		global_file_name = argv[i];
		global_program_pointer = &program_text;
		global_program_start = program_text;
		return_value = verify_block(&program_text, 0, NULL);
		if(return_value){
			printf("\nProgram returned: ");
			print_statement(return_value);
			printf("\n");
			free_statement(return_value);
		}

		free(program_start);
	}
	free_dictionary(variables, free_variable_void);
	free_dictionary(definitions, free_proposition_void);
	return 0;
}

