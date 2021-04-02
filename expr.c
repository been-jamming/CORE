#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "dictionary.h"
#include "expr.h"

unsigned int current_depth;
dictionary variables[MAX_DEPTH];
dictionary definitions[MAX_DEPTH];
dictionary proofs[MAX_DEPTH];
dictionary bound_variables;
dictionary bound_propositions;
unsigned int line_number;
char *global_file_name;
char **global_program_pointer;
char *global_program_start;

int order_of_operations[3] = {3, 2, 1};

statement *create_statement(statement_type type, int num_bound_vars, int num_bound_props){
	statement *output;

	output = malloc(sizeof(statement));
	output->type = type;
	output->num_bound_vars = num_bound_vars;
	output->num_bound_props = num_bound_props;

	return output;
}

void skip_whitespace(char **c){
	while(**c == ' ' || **c == '\t' || **c == '\n' || (**c == '/' && (*c)[1] == '/')){
		if(**c == '\n'){
			line_number++;
		}
		if(**c == '/' && (*c)[1] == '/'){
			while(**c && **c != '\n'){
				++*c;
			}
		} else {
			++*c;
		}
	}
}

void error(int error_code){
	char *error_place;
	char *line_start;
	char *place;

	fprintf(stderr, "Line %d of '%s'\n", line_number, global_file_name);
	error_place = *global_program_pointer;
	line_start = error_place;
	while(line_start != global_program_start && line_start[-1] != '\n'){
		line_start--;
	}
	skip_whitespace(&line_start);
	for(place = line_start; *place && *place != '\n'; place++){
		fputc(*place, stderr);
	}
	fprintf(stderr, "\n%*c^\n", (int) (error_place - line_start), ' ');

	exit(error_code);
}

int is_digit(char c){
	return c >= '0' && c <= '9';
}

int is_alpha(char c){
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int is_alphanumeric(char c){
	return is_alpha(c) || is_digit(c) || c == '_';
}

void get_identifier(char **c, char *buffer, size_t buffer_length){
	char *end_of_identifier;

	if(!is_alpha(**c)){
		buffer[0] = '\0';
		return;
	}

	end_of_identifier = *c;
	while(is_alphanumeric(*end_of_identifier)){
		end_of_identifier++;
	}

	if(end_of_identifier - *c >= buffer_length){
		buffer[0] = '\0';
		return;
	}

	strncpy(buffer, *c, end_of_identifier - *c);
	buffer[end_of_identifier - *c] = '\0';
	*c += end_of_identifier - *c;
}

statement *parse_true_false(char **c, int num_bound_vars, int num_bound_props){
	statement *output;

	skip_whitespace(c);
	if(!strncmp(*c, "true", 4) && !is_alphanumeric((*c)[4])){
		output = create_statement(TRUE, num_bound_vars, num_bound_props);
		*c += 4;
		return output;
	} else if(!strncmp(*c, "false", 5) && !is_alphanumeric((*c)[5])){
		output = create_statement(FALSE, num_bound_vars, num_bound_props);
		*c += 5;
		return output;
	}

	return NULL;
}

statement *parse_membership(char **c, int num_bound_vars, int num_bound_props){
	char var_name0[256];
	char var_name1[256];
	int *var0_id = NULL;
	int *var1_id = NULL;
	variable *var0 = NULL;
	variable *var1 = NULL;
	unsigned char is_bound0;
	unsigned char is_bound1;
	int i;
	statement *output;
	char *beginning;

	skip_whitespace(c);
	beginning = *c;
	get_identifier(c, var_name0, 256);
	if(var_name0[0] == '\0'){
		*c = beginning;
		return NULL;
	}

	skip_whitespace(c);
	get_identifier(c, var_name1, 256);
	if(strcmp(var_name1, "in")){
		*c = beginning;
		return NULL;
	}

	skip_whitespace(c);
	get_identifier(c, var_name1, 256);
	if(var_name1[0] == '\0'){
		*c = beginning;
		return NULL;
	}

	var0_id = read_dictionary(bound_variables, var_name0, 0);
	if(!var0_id){
		is_bound0 = 0;
		for(i = current_depth; i >= 0; i--){
			var0 = read_dictionary(variables[i], var_name0, 0);
			if(var0){
				break;
			}
		}
	} else {
		is_bound0 = 1;
	}

	var1_id = read_dictionary(bound_variables, var_name1, 0);
	if(!var1_id){
		is_bound1 = 0;
		for(i = current_depth; i >= 0; i--){
			var1 = read_dictionary(variables[i], var_name1, 0);
			if(var1){
				break;
			}
		}
	} else {
		is_bound1 = 1;
	}

	if((var0_id || var0) && (var1_id || var1)){
		output = create_statement(MEMBERSHIP, num_bound_vars, num_bound_props);
		if(is_bound0){
			output->var0_id = *var0_id;
		} else {
			output->var0 = var0;
			output->var0->num_references++;
		}
		if(is_bound1){
			output->var1_id = *var1_id;
		} else {
			output->var1 = var1;
			output->var1->num_references++;
		}
		output->is_bound0 = is_bound0;
		output->is_bound1 = is_bound1;
		return output;
	} else {
		*c = beginning;
		return NULL;
	}
}

statement *parse_proposition(char **c, int num_bound_vars, int num_bound_props){
	char prop_name[256];
	char var_name[256];
	unsigned char is_bound;
	bound_proposition *bound_prop = NULL;
	proposition *prop = NULL;
	int *var_id;
	variable *var;
	int i;
	unsigned int num_args;
	unsigned int counted_args = 0;
	proposition_arg *args;
	statement *output;
	char *beginning;

	skip_whitespace(c);
	beginning = *c;
	get_identifier(c, prop_name, 256);
	if(prop_name[0] == '\0'){
		*c = beginning;
		return NULL;
	}

	skip_whitespace(c);
	bound_prop = read_dictionary(bound_propositions, prop_name, 0);
	if(!bound_prop){
		is_bound = 0;
		for(i = current_depth; i >= 0; i--){
			prop = read_dictionary(definitions[i], prop_name, 0);
			if(prop){
				break;
			}
		}
	} else {
		is_bound = 1;
	}

	if(is_bound){
		num_args = bound_prop->num_args;
	} else {
		if(prop){
			num_args = prop->num_args;
		} else {
			*c = beginning;
			return NULL;
		}
	}

	if(**c == '('){
		++*c;
		if(num_args == 0){
			skip_whitespace(c);
			args = NULL;
			if(**c != ')'){
				fprintf(stderr, "Error: argument count mismatch\n");
				*c = beginning;
				return NULL;
			}
			++*c;
		} else {
			args = malloc(sizeof(proposition_arg)*num_args);
			while(**c != ')'){
				skip_whitespace(c);
				get_identifier(c, var_name, 256);
				if(var_name[0] == '\0'){
					fprintf(stderr, "Error: expected argument identifier\n");
					*c = beginning;
					free(args);
					return NULL;
				}
				var_id = read_dictionary(bound_variables, var_name, 0);
				if(!var_id){
					for(i = current_depth; i >= 0; i--){
						var = read_dictionary(variables[i], var_name, 0);
						if(var){
							break;
						}
					}
				}
				if(var_id){
					args[counted_args].is_bound = 1;
					args[counted_args].var_id = *var_id;
				} else if(var){
					args[counted_args].is_bound = 0;
					args[counted_args].var = var;
					args[counted_args].var->num_references++;
				} else {
					fprintf(stderr, "Error: unknown variable '%s'\n", var_name);
					*c = beginning;
					free(args);
					return NULL;
				}
				counted_args++;
				skip_whitespace(c);
				if(**c == ',' && counted_args < num_args){
					++*c;
				} else if(**c == ','){
					fprintf(stderr, "Error: excess arguments in proposition\n");
					*c = beginning;
					free(args);
					return NULL;
				} else if(**c != ')'){
					fprintf(stderr, "Error: expected ',' or ')'\n");
					*c = beginning;
					free(args);
					return NULL;
				}
			}
			++*c;
			if(counted_args < num_args){
				fprintf(stderr, "Error: not enough arguments in proposition\n");
				*c = beginning;
				free(args);
				return NULL;
			}
		}
	} else if(num_args == 0){
		args = NULL;
	} else {
		fprintf(stderr, "Error: expected arguments for proposition\n");
		*c = beginning;
		return NULL;
	}

	if(bound_prop || prop){
		output = create_statement(PROPOSITION, num_bound_vars, num_bound_props);
		if(is_bound){
			output->prop_id = bound_prop->prop_id;
			output->prop_args = args;
		} else {
			output->prop = prop;
			output->prop_args = args;
			output->prop->num_references++;
		}
		output->is_bound = is_bound;
		output->num_args = num_args;
		return output;
	} else {
		*c = beginning;
		return NULL;
	}
}

statement *parse_value(char **c, int num_bound_vars, int num_bound_props);
statement *parse_statement(char **c, int num_bound_vars, int num_bound_props);

statement *parse_not(char **c, int num_bound_vars, int num_bound_props){
	statement *output;
	char *beginning;

	skip_whitespace(c);
	beginning = *c;
	if(**c != '~'){
		return NULL;
	}

	++*c;
	output = create_statement(NOT, num_bound_vars, num_bound_props);
	output->child0 = parse_value(c, num_bound_vars, num_bound_props);
	if(!output->child0){
		*c = beginning;
		free(output);
		return NULL;
	}

	return output;
}

statement *parse_all(char **c, int num_bound_vars, int num_bound_props){
	char var_name[256];
	statement *output;
	char *beginning;
	int *new_int;
	int *old_int;

	skip_whitespace(c);
	beginning = *c;
	if(**c != '*'){
		return NULL;
	}

	++*c;
	skip_whitespace(c);
	get_identifier(c, var_name, 256);
	skip_whitespace(c);
	if(var_name[0] == '\0'){
		*c = beginning;
		return NULL;
	}

	old_int = read_dictionary(bound_variables, var_name, 0);

	new_int = malloc(sizeof(int));
	*new_int = num_bound_vars;
	write_dictionary(&bound_variables, var_name, new_int, 0);

	output = create_statement(FORALL, num_bound_vars, num_bound_props);
	output->child0 = parse_value(c, num_bound_vars + 1, num_bound_props);

	write_dictionary(&bound_variables, var_name, old_int, 0);
	free(new_int);

	return output;
}

statement *parse_exists(char **c, int num_bound_vars, int num_bound_props){
	char var_name[256];
	statement *output;
	char *beginning;
	int *new_int;
	int *old_int;

	skip_whitespace(c);
	beginning = *c;
	if(**c != '^'){
		return NULL;
	}

	++*c;
	skip_whitespace(c);
	get_identifier(c, var_name, 256);
	skip_whitespace(c);
	if(var_name[0] == '\0'){
		*c = beginning;
		return NULL;
	}

	old_int = read_dictionary(bound_variables, var_name, 0);

	new_int = malloc(sizeof(int));
	*new_int = num_bound_vars;
	write_dictionary(&bound_variables, var_name, new_int, 0);

	output = create_statement(EXISTS, num_bound_vars, num_bound_props);
	output->child0 = parse_value(c, num_bound_vars + 1, num_bound_props);

	write_dictionary(&bound_variables, var_name, old_int, 0);
	free(new_int);

	return output;
}

statement *parse_parentheses(char **c, int num_bound_vars, int num_bound_props){
	statement *output;

	skip_whitespace(c);
	if(**c != '('){
		return NULL;
	}
	++*c;
	output = parse_statement(c, num_bound_vars, num_bound_props);

	if(**c != ')'){
		fprintf(stderr, "Error: expected ')'\n");
		error(1);
	}
	++*c;

	return output;
}

statement *parse_value(char **c, int num_bound_vars, int num_bound_props){
	statement *output;

	if((output = parse_true_false(c, num_bound_vars, num_bound_props))){
		return output;
	} else if((output = parse_membership(c, num_bound_vars, num_bound_props))){
		return output;
	} else if((output = parse_proposition(c, num_bound_vars, num_bound_props))){
		return output;
	} else if((output = parse_not(c, num_bound_vars, num_bound_props))){
		return output;
	} else if((output = parse_all(c, num_bound_vars, num_bound_props))){
		return output;
	} else if((output = parse_exists(c, num_bound_vars, num_bound_props))){
		return output;
	} else if((output = parse_parentheses(c, num_bound_vars, num_bound_props))){
		return output;
	} else {
		fprintf(stderr, "Error: could not parse statement value\n");
		error(1);
	}

	return NULL;
}

int get_operation(char **c){
	skip_whitespace(c);
	if(**c == '&'){
		++*c;
		return AND;
	} else if(**c == '|'){
		++*c;
		return OR;
	} else if((*c)[0] == '-' && (*c)[1] == '>'){
		*c += 2;
		return IMPLIES;
	} else {
		return -1;
	}
}

static statement *parse_statement_recursive(int priority, statement *s0, char **c, int num_bound_vars, int num_bound_props){
	statement *s1;
	statement *output;
	statement_type operation;
	char *temp_c;

	skip_whitespace(c);
	temp_c = *c;
	operation = get_operation(&temp_c);
	if(operation == -1 && **c && **c != ')' && **c != ';' && **c != ']' && **c != '{'){
		fprintf(stderr, "Unrecognized operation '%c'\n", **c);
		error(1);
	} else if(!**c || **c == ')' || **c == ';' || **c == ']' || **c == '{'){
		return s0;
	}

	if(order_of_operations[operation] < priority){
		return s0;
	}

	*c = temp_c;
	s1 = parse_value(c, num_bound_vars, num_bound_props);
	s1 = parse_statement_recursive(order_of_operations[operation], s1, c, num_bound_vars, num_bound_props);
	output = create_statement(operation, num_bound_vars, num_bound_props);
	output->child0 = s0;
	output->child1 = s1;

	return output;
}

statement *parse_statement(char **c, int num_bound_vars, int num_bound_props){
	statement *output;

	output = parse_value(c, num_bound_vars, num_bound_props);
	skip_whitespace(c);
	while(**c && **c != ')' && **c != ';' && **c != ']' && **c != '{'){
		output = parse_statement_recursive(0, output, c, num_bound_vars, num_bound_props);
		skip_whitespace(c);
	}

	return output;
}

//Decrement all references inside of a statement in preparation for freeing the statement
void decrement_references(statement *s){
	int i;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
			decrement_references(s->child0);
			decrement_references(s->child1);
			break;
		case PROPOSITION:
			for(i = 0; i < s->num_args; i++){
				if(!s->prop_args[i].is_bound){
					s->prop_args[i].var->num_references--;
				}
			}
			if(!s->is_bound){
				s->prop->num_references--;
			}
			break;
		case FORALL:
		case EXISTS:
		case NOT:
			decrement_references(s->child0);
			break;
		case MEMBERSHIP:
			if(!s->is_bound0){
				s->var0->num_references--;
			}
			if(!s->is_bound1){
				s->var1->num_references--;
			}
			break;
		default:
			//pass
			break;
	}
}

//Free a statement without decrementing any references
void free_statement_independent(statement *s){
	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
			free_statement_independent(s->child0);
			free_statement_independent(s->child1);
			free(s);
			break;
		case PROPOSITION:
			free(s->prop_args);
			free(s);
			break;
		case FORALL:
		case EXISTS:
		case NOT:
			free_statement_independent(s->child0);
			free(s);
			break;
		default:
			free(s);
			break;
	}
}

void free_statement(statement *s){
	int i;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
			free_statement(s->child0);
			free_statement(s->child1);
			free(s);
			break;
		case PROPOSITION:
			for(i = 0; i < s->num_args; i++){
				if(!s->prop_args[i].is_bound){
					s->prop_args[i].var->num_references--;
				}
			}
			free(s->prop_args);
			if(!s->is_bound){
				s->prop->num_references--;
			}
			free(s);
			break;
		case FORALL:
		case EXISTS:
		case NOT:
			free_statement(s->child0);
			free(s);
			break;
		case MEMBERSHIP:
			if(!s->is_bound0){
				s->var0->num_references--;
			}
			if(!s->is_bound1){
				s->var1->num_references--;
			}
		default:
			free(s);
			break;
	}
}

void copy_statement(statement *dest, statement *s){
	int i;

	dest->type = s->type;
	dest->num_bound_vars = s->num_bound_vars;
	dest->num_bound_props = s->num_bound_props;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
			dest->child0 = malloc(sizeof(statement));
			dest->child1 = malloc(sizeof(statement));
			copy_statement(dest->child0, s->child0);
			copy_statement(dest->child1, s->child1);
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			dest->child0 = malloc(sizeof(statement));
			copy_statement(dest->child0, s->child0);
			break;
		case MEMBERSHIP:
			if(s->is_bound0){
				dest->is_bound0 = 1;
				dest->var0_id = s->var0_id;
			} else {
				dest->is_bound0 = 0;
				dest->var0 = s->var0;
				dest->var0->num_references++;
			}
			if(s->is_bound1){
				dest->is_bound1 = 1;
				dest->var1_id = s->var1_id;
			} else {
				dest->is_bound1 = 0;
				dest->var1 = s->var1;
				dest->var1->num_references++;
			}
			break;
		case PROPOSITION:
			if(s->is_bound){
				dest->is_bound = 1;
				dest->prop_id = s->prop_id;
			} else {
				dest->is_bound = 0;
				dest->prop = s->prop;
				dest->prop->num_references++;
			}
			dest->prop_args = malloc(sizeof(proposition_arg)*s->num_args);
			memcpy(dest->prop_args, s->prop_args, sizeof(proposition_arg)*s->num_args);
			dest->num_args = s->num_args;
			for(i = 0; i < dest->num_args; i++){
				if(!dest->prop_args[i].is_bound){
					dest->prop_args[i].var->num_references++;
				}
			}
			break;
		case TRUE:
		case FALSE:
			//pass
			break;
	}
}

void print_statement(statement *s){
	int i;

	printf("(");
	switch(s->type){
		case AND:
			print_statement(s->child0);
			printf("&");
			print_statement(s->child1);
			break;
		case OR:
			print_statement(s->child0);
			printf("|");
			print_statement(s->child1);
			break;
		case PROPOSITION:
			if(s->is_bound){
				printf("P%d(", s->prop_id);
			} else {
				printf("%s(", s->prop->name);
			}
			for(i = 0; i < s->num_args; i++){
				if(i){
					printf(", ");
				}
				if(s->prop_args[i].is_bound){
					printf("%d", s->prop_args[i].var_id);
				} else {
					printf("%s", s->prop_args[i].var->name);
				}
			}
			printf(")");
			break;
		case FORALL:
			printf("*%d", s->num_bound_vars);
			print_statement(s->child0);
			break;
		case EXISTS:
			printf("^%d", s->num_bound_vars);
			print_statement(s->child0);
			break;
		case IMPLIES:
			print_statement(s->child0);
			printf("->");
			print_statement(s->child1);
			break;
		case NOT:
			printf("~");
			print_statement(s->child0);
			break;
		case FALSE:
			printf("false");
			break;
		case TRUE:
			printf("true");
			break;
		case MEMBERSHIP:
			if(s->is_bound0){
				printf("%d", s->var0_id);
			} else {
				printf("%s", s->var0->name);
			}
			printf(" in ");
			if(s->is_bound1){
				printf("%d", s->var1_id);
			} else {
				printf("%s", s->var1->name);
			}
			break;
	}
	printf(")");
}

unsigned char compare_statement(statement *a, statement *b){
	int i;

	if(a->type != b->type || a->num_bound_vars != b->num_bound_vars || a->num_bound_props != b->num_bound_props){
		return 0;
	}

	switch(a->type){
		case AND:
		case OR:
		case IMPLIES:
			return compare_statement(a->child0, b->child0) && compare_statement(a->child1, b->child1);
		case NOT:
		case FORALL:
		case EXISTS:
			return compare_statement(a->child0, b->child0);
		case MEMBERSHIP:
			if(a->is_bound0 != b->is_bound0){
				return 0;
			}
			if(a->is_bound0 && a->var0_id != b->var0_id){
				return 0;
			}
			if(!a->is_bound && a->var0 != b->var0){
				return 0;
			}
			if(a->is_bound1 != b->is_bound1){
				return 0;
			}
			if(a->is_bound1 && a->var1_id != b->var1_id){
				return 0;
			}
			if(!a->is_bound1 && a->var1 != b->var1){
				return 0;
			}
			return 1;
		case PROPOSITION:
			if(a->is_bound != b->is_bound || a->num_args != b->num_args){
				return 0;
			}
			if(a->is_bound && a->prop_id != b->prop_id){
				return 0;
			}
			if(!a->is_bound && a->prop != b->prop){
				return 0;
			}
			for(i = 0; i < a->num_args; i++){
				if(a->prop_args[i].is_bound != b->prop_args[i].is_bound){
					return 0;
				}
				if(a->prop_args[i].is_bound && a->prop_args[i].var_id != b->prop_args[i].var_id){
					return 0;
				}
				if(!a->prop_args[i].is_bound && a->prop_args[i].var != b->prop_args[i].var){
					return 0;
				}
			}
			return 1;
		default:
			return 1;
	}
}

statement *peel_and_left(statement **s){
	statement *output = NULL;
	statement *child;

	if((*s)->type != AND){
		output = *s;
		*s = NULL;
		return output;
	}

	while((*s)->child0->type == AND){
		s = &((*s)->child0);
	}

	output = (*s)->child0;
	child = (*s)->child1;
	free(*s);
	*s = child;

	return output;
}

statement *peel_or_left(statement **s){
	statement *output = NULL;
	statement *child;

	if((*s)->type != OR){
		output = *s;
		*s = NULL;
		return output;
	}

	while((*s)->child0->type == OR){
		s = &((*s)->child0);
	}

	output = (*s)->child0;
	child = (*s)->child1;
	free(*s);
	*s = child;

	return output;
}

