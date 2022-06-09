#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "dictionary.h"
#include "predicate.h"
#include "expression.h"
#include "custom_malloc.h"

//CORE First Order Logic Parser
//Ben Jones

//The parser is recursive and maintains state by passing a double pointer (char **) when it calls a function.

int global_relation_id;
dictionary global_bound_variables;
dictionary global_bound_propositions;
unsigned int global_line_number = 0;
char *global_file_name = NULL;
char **global_program_pointer = NULL;
char *global_program_start = NULL;
context *global_context = NULL;

//Predicate parsing order of operations
static int order_of_operations[4] = {3, 2, 1, 0};
//All CORE error messages
static char *error_messages[] = {
	NULL,
	"Identifier exceeds maximum length",
	"Expected identifier",
	"Argument count mismatch",
	"Expected ')'",
	"Unrecognized sentence value",
	"Unrecognized sentence operation",
	"Definition does not exist",
	"Definition is empty",
	"Relation does not exist",
	"Relation is empty",
	"Expected ']'",
	"Expected '.'",
	"Context member not found",
	"Cannot apply '.'",
	"Sentence has no bound variables",
	"Cannot overwrite referenced variable",
	"Variable not found",
	"Invalid argument type",
	"Argument has bound variables",
	"Argument has bound propositions",
	"Expected ')' or ','",
	"Expected ','",
	"Expected ')'",
	"Mismatched implications",
	"Expected verified argument",
	"Too many arguments to unpack",
	"Expected '{'",
	"Expected return statement",
	"Expected '}'",
	"Expected 'or'",
	"Mismatched returned sentences",
	"Expected '('",
	"Invalid operand type",
	"Operand has bound variables",
	"Operand has bound propositions",
	"Expected '|'",
	"Expected '|' or ','",
	"Expected verified operand",
	"Expected expression or variable",
	"Mismatched argument",
	"Expected expression",
	"Operand must have more bound propositions",
	"Expected ']' or ','",
	"Expected ':' or ','",
	"Expected '>'",
	"Unrecognized operation",
	"No parent context",
	"Expected ';'",
	"Duplicate identifier",
	"Expected ':' or ';'",
	"Expected number of arguments",
	"Expected ':'",
	"Expected relation identifier",
	"Duplicate relation",
	"Empty relation in non-global scope",
	"Duplicate definition",
	"Expected ';' or ','",
	"Expected '}' or EOF",
	"Expected EOF",
	"Invalid return type",
	"Return value unverified",
	"Scope is not global",
	"Incorrect goal type",
	"Expected '{' or ';'",
	"Expected '{', ';', or ','",
	"No goal",
	"Unexpected return value",
	"Argument is not trivially true",
	"Could not read file"
};

//Allocate a sentence structure
sentence *create_sentence(sentence_type type, int num_bound_vars, int num_bound_props){
	sentence *output;

	output = malloc(sizeof(sentence));
	output->type = type;
	output->num_bound_vars = num_bound_vars;
	output->num_bound_props = num_bound_props;
	output->parent = NULL;

	return output;
}

//Check to see if the next character is a whitespace character, or if the next two characters are "//"
unsigned char is_whitespace(char *c){
	return *c == ' ' || *c == '\t' || *c == '\n' || *c == '\r' || (*c == '/' && c[1] == '/');
}

//Skip to the next character which isn't whitespace
void skip_whitespace(char **c){
	while(is_whitespace(*c)){
		if(**c == '\n'){
			global_line_number++;
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

//Get the number of digits in an unsigned integer
//Used in composition of error message
static int num_digits_uint(unsigned int i){
	int output = 0;

	do{
		i /= 10;
		output++;
	} while(i);

	return output;
}

//Print fancy error message
void error(int error_code){
	char *error_place;
	char *line_start;
	char *place;

	fprintf(stderr, "\033[31;1mError\033[0;1m: %s\033[0m\n", error_messages[error_code]);
	if(error_code == ERROR_FILE_READ){
		fprintf(stderr, "in '%s'\n", global_file_name);
#ifdef USE_CUSTOM_ALLOC
		custom_malloc_abort();
#endif
		exit(ERROR_FILE_READ);
		return;
	}
	fprintf(stderr, "in '%s'\n%d ", global_file_name, global_line_number);
	error_place = *global_program_pointer;
	line_start = error_place;
	while(line_start != global_program_start && line_start[-1] != '\n'){
		line_start--;
	}
	skip_whitespace(&line_start);
	for(place = line_start; *place && *place != '\n'; place++){
		fputc(*place, stderr);
	}
	fprintf(stderr, "\n%*c^\n", (int) (error_place - line_start + num_digits_uint(global_line_number) + 1), ' ');

#ifdef USE_CUSTOM_ALLOC
	custom_malloc_abort();
#endif
	exit(error_code);
}

//Determines whether a character is a digit
int is_digit(char c){
	return c >= '0' && c <= '9';
}

//Determines whether a character is a letter
int is_alpha(char c){
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

//Determines whether a character is alphanumeric
//Used in parsing identifiers
//Note: '_' is considered alphanumeric
int is_alphanumeric(char c){
	return is_alpha(c) || is_digit(c) || c == '_';
}

//Copies the next identifier in the source into a buffer
//Increments the pointer past the identifier
//Returns 1 if the identifier was longer than the buffer, otherwise 0
int get_identifier(char **c, char *buffer, size_t buffer_length){
	char *end_of_identifier;

	//Each identifier must begin with a letter
	if(!is_alpha(**c)){
		buffer[0] = '\0';
		return 0;
	}

	end_of_identifier = *c;
	while(is_alphanumeric(*end_of_identifier)){
		end_of_identifier++;
	}

	if(end_of_identifier - *c >= buffer_length){
		buffer[0] = '\0';
		return 1;
	}

	strncpy(buffer, *c, end_of_identifier - *c);
	buffer[end_of_identifier - *c] = '\0';
	*c += end_of_identifier - *c;
	return 0;
}

//Copies the next valid relation identifier into a buffer
//Increments the pointer past the identifier
//Returns 1 if the identifier was longer than the buffer, otherwise 0
int get_relation_identifier(char **c, char *buffer, size_t buffer_length){
	char *end_of_identifier;

	//If the first character is a letter, parse as a normal identifier
	if(is_alpha(**c)){
		return get_identifier(c, buffer, buffer_length);
	}

	end_of_identifier = *c;
	while(!is_alpha(*end_of_identifier) && !is_whitespace(end_of_identifier) && *end_of_identifier && *end_of_identifier != ';' && *end_of_identifier != '(' && *end_of_identifier != ')'){
		end_of_identifier++;
	}

	if(end_of_identifier - *c >= buffer_length){
		buffer[0] = '\0';
		return 1;
	}

	strncpy(buffer, *c, end_of_identifier - *c);
	buffer[end_of_identifier - *c] = '\0';
	*c += end_of_identifier - *c;
	return 0;
}

//Parse a predicate which is either the true or false literal
sentence *parse_true_false(char **c, int num_bound_vars, int num_bound_props){
	sentence *output;

	skip_whitespace(c);
	if(!strncmp(*c, "true", 4) && !is_alphanumeric((*c)[4])){
		output = create_sentence(TRUE, num_bound_vars, num_bound_props);
		*c += 4;
		return output;
	} else if(!strncmp(*c, "false", 5) && !is_alphanumeric((*c)[5])){
		output = create_sentence(FALSE, num_bound_vars, num_bound_props);
		*c += 5;
		return output;
	}

	return NULL;
}

//Parse a relation symbol
//The symbol may sit inside of another context stored in a variable
relation *parse_relation_symbol(char **c){
	char name_buffer[256];
	context *search_context;
	variable *var = NULL;
	relation *rel = NULL;

	skip_whitespace(c);
	if(get_relation_identifier(c, name_buffer, 256)){
		return NULL;
	}
	search_context = global_context;
	while(search_context){
		rel = read_dictionary(search_context->relations, name_buffer, 0);
		if(!rel){
			var = read_dictionary(search_context->variables, name_buffer, 0);
			if(var){
				break;
			}
		} else {
			break;
		}
		search_context = search_context->parent;
	}
	if(rel){
		return rel;
	} else if(var){
		if(var->type != CONTEXT_VAR){
			return NULL;
		}
		search_context = var->context_data;
		while(search_context){
			skip_whitespace(c);
			if(**c != '.'){
				return NULL;
			}
			++*c;
			skip_whitespace(c);
			if(get_relation_identifier(c, name_buffer, 256)){
				return NULL;
			}
			rel = read_dictionary(search_context->relations, name_buffer, 0);
			if(!rel){
				var = read_dictionary(search_context->variables, name_buffer, 0);
				if(var && var->type == CONTEXT_VAR){
					search_context = var->context_data;
				} else {
					return NULL;
				}
			} else {
				return rel;
			}
		}
	}

	return NULL;
}

//Parse an object symbol
//The object may sit inside of another context stored in a variable
variable *parse_object_symbol(char **c, int **var_id){
	char name_buffer[256];
	context *search_context;
	variable *var = NULL;

	skip_whitespace(c);
	if(get_identifier(c, name_buffer, 256)){
		return NULL;
	}
	*var_id = read_dictionary(global_bound_variables, name_buffer, 0);
	if(*var_id){
		return NULL;
	}
	search_context = global_context;
	while(search_context){
		var = read_dictionary(search_context->variables, name_buffer, 0);
		if(var){
			break;
		}
		search_context = search_context->parent;
	}
	if(!var){
		return NULL;
	}
	if(var->type == OBJECT_VAR){
		return var;
	} else {
		if(var->type != CONTEXT_VAR){
			return NULL;
		}
		search_context = var->context_data;
		while(search_context){
			skip_whitespace(c);
			if(**c != '.'){
				return NULL;
			}
			++*c;
			skip_whitespace(c);
			if(get_identifier(c, name_buffer, 256)){
				return NULL;
			}
			var = read_dictionary(search_context->variables, name_buffer, 0);
			if(!var){
				return NULL;
			}
			if(var->type == CONTEXT_VAR){
				search_context = var->context_data;
			} else if(var->type == OBJECT_VAR){
				return var;
			} else {
				return NULL;
			}
		}
		return NULL;
	}
}

//Parse a definition symbol
//The symbol may sit inside of another context stored in a variable
definition *parse_definition_symbol(char **c, bound_proposition **bound_prop){
	char name_buffer[256];
	context *search_context;
	variable *var = NULL;
	definition *def = NULL;

	skip_whitespace(c);
	if(get_identifier(c, name_buffer, 256)){
		return NULL;
	}
	*bound_prop = read_dictionary(global_bound_propositions, name_buffer, 0);
	if(*bound_prop){
		return NULL;
	}
	search_context = global_context;
	while(search_context){
		def = read_dictionary(search_context->definitions, name_buffer, 0);
		if(!def){
			var = read_dictionary(search_context->variables, name_buffer, 0);
			if(var){
				break;
			}
		} else {
			break;
		}
		search_context = search_context->parent;
	}
	if(def){
		return def;
	} else if(var){
		if(var->type != CONTEXT_VAR){
			return NULL;
		}
		search_context = var->context_data;
		while(search_context){
			skip_whitespace(c);
			if(**c != '.'){
				return NULL;
			}
			++*c;
			skip_whitespace(c);
			if(get_identifier(c, name_buffer, 256)){
				return NULL;
			}
			def = read_dictionary(search_context->definitions, name_buffer, 0);
			if(!def){
				var = read_dictionary(search_context->variables, name_buffer, 0);
				if(var && var->type == CONTEXT_VAR){
					search_context = var->context_data;
				} else {
					return NULL;
				}
			} else {
				printf("'%s'\n", name_buffer);
				return def;
			}
		}
	}

	return NULL;
}

//Parse a relation
//Allocates and returns a sentence structure when successful
//Returns NULL when unsuccessful
sentence *parse_relation(char **c, int num_bound_vars, int num_bound_props){
	int *var0_id = NULL;
	int *var1_id = NULL;
	variable *var0 = NULL;
	variable *var1 = NULL;
	relation *relation_data = NULL;
	sentence *output;
	char *beginning;

	skip_whitespace(c);
	beginning = *c;
	var0 = parse_object_symbol(c, &var0_id);
	if(!var0 && !var0_id){
		*c = beginning;
		return NULL;
	}

	skip_whitespace(c);
	relation_data = parse_relation_symbol(c);
	if(!relation_data){
		*c = beginning;
		return NULL;
	}

	skip_whitespace(c);
	var1 = parse_object_symbol(c, &var1_id);
	if(!var1 && !var1_id){
		*c = beginning;
		return NULL;
	}

	if((var0_id || var0) && (var1_id || var1) && relation_data){
		output = create_sentence(RELATION, num_bound_vars, num_bound_props);
		relation_data->num_references++;
		output->relation_data = relation_data;
		if(var0_id){
			output->var0_id = *var0_id;
		} else {
			output->var0 = var0;
			output->var0->num_references++;
		}
		if(var1_id){
			output->var1_id = *var1_id;
		} else {
			output->var1 = var1;
			output->var1->num_references++;
		}
		output->is_bound0 = (var0_id != NULL);
		output->is_bound1 = (var1_id != NULL);
		return output;
	} else {
		*c = beginning;
		return NULL;
	}
}

//Parse a proposition
//Allocates and returns a sentence structure when successful
//Returns NULL when unsuccessful
sentence *parse_proposition(char **c, int num_bound_vars, int num_bound_props){
	definition *prop = NULL;
	bound_proposition *bound_prop = NULL;
	int *var_id = NULL;
	variable *var = NULL;
	unsigned int num_args;
	unsigned int counted_args = 0;
	proposition_arg *args = NULL;
	sentence *output = NULL;
	char *beginning = NULL;

	skip_whitespace(c);
	beginning = *c;
	prop = parse_definition_symbol(c, &bound_prop);
	if(!prop && !bound_prop){
		*c = beginning;
		return NULL;
	}

	skip_whitespace(c);

	if(bound_prop){
		num_args = bound_prop->num_args;
	} else {
		num_args = prop->num_args;
	}

	if(**c == '('){
		++*c;
		if(num_args == 0){
			skip_whitespace(c);
			if(**c != ')'){
				*c = beginning;
				return NULL;
			}
			++*c;
		} else {
			args = malloc(sizeof(proposition_arg)*num_args);
			skip_whitespace(c);
			while(**c != ')'){
				skip_whitespace(c);
				var = parse_object_symbol(c, &var_id);
				if(!var && !var_id){
					*c = beginning;
					free(args);
					return NULL;
				}
				if(var_id){
					args[counted_args].is_bound = 1;
					args[counted_args].var_id = *var_id;
				} else {
					args[counted_args].is_bound = 0;
					args[counted_args].var = var;
					args[counted_args].var->num_references++;
				}
				counted_args++;
				skip_whitespace(c);
				if(**c == ',' && counted_args < num_args){
					++*c;
				} else if(**c == ','){
					*c = beginning;
					free(args);
					return NULL;
				} else if(**c != ')'){
					*c = beginning;
					free(args);
					return NULL;
				}
			}
			++*c;
			if(counted_args < num_args){
				*c = beginning;
				free(args);
				return NULL;
			}
		}
	} else if(num_args == 0){
		args = NULL;
	} else {
		*c = beginning;
		return NULL;
	}

	if(bound_prop || prop){
		output = create_sentence(PROPOSITION, num_bound_vars, num_bound_props);
		if(bound_prop){
			output->definition_id = bound_prop->prop_id;
			output->proposition_args = args;
		} else {
			output->definition_data = prop;
			output->proposition_args = args;
			output->definition_data->num_references++;
		}
		output->is_bound = (bound_prop != NULL);
		output->num_args = num_args;
		return output;
	} else {
		*c = beginning;
		return NULL;
	}
}

sentence *parse_sentence_value(char **c, int num_bound_vars, int num_bound_props);
sentence *parse_sentence(char **c, int num_bound_vars, int num_bound_props);

//Parse a sentence value beginning with "~" which represents "not"
sentence *parse_not(char **c, int num_bound_vars, int num_bound_props){
	sentence *output;

	skip_whitespace(c);
	if(**c != '~'){
		return NULL;
	}

	++*c;
	output = create_sentence(NOT, num_bound_vars, num_bound_props);
	output->child0 = parse_sentence_value(c, num_bound_vars, num_bound_props);
	output->child0->parent = output;

	return output;
}

//Parse a sentence value beginning with "*" which represents "for all"
sentence *parse_all(char **c, int num_bound_vars, int num_bound_props){
	char var_name[256];
	sentence *output;
	char *beginning;
	int new_int;
	int *old_int;

	skip_whitespace(c);
	beginning = *c;
	if(**c != '*'){
		return NULL;
	}

	++*c;
	skip_whitespace(c);
	if(get_identifier(c, var_name, 256)){
		error(ERROR_IDENTIFIER_LENGTH);
	}
	skip_whitespace(c);
	if(var_name[0] == '\0'){
		*c = beginning;
		return NULL;
	}

	old_int = read_dictionary(global_bound_variables, var_name, 0);

	new_int = num_bound_vars;
	write_dictionary(&global_bound_variables, var_name, &new_int, 0);

	output = create_sentence(FORALL, num_bound_vars, num_bound_props);
	output->child0 = parse_sentence_value(c, num_bound_vars + 1, num_bound_props);
	output->child0->parent = output;

	write_dictionary(&global_bound_variables, var_name, old_int, 0);

	return output;
}

//Parse a sentence value beginning with "^" which represents "there exists"
sentence *parse_exists(char **c, int num_bound_vars, int num_bound_props){
	char var_name[256];
	sentence *output;
	char *beginning;
	int *old_int;
	int new_int;

	skip_whitespace(c);
	beginning = *c;
	if(**c != '^'){
		return NULL;
	}

	++*c;
	skip_whitespace(c);
	if(get_identifier(c, var_name, 256)){
		error(ERROR_IDENTIFIER_LENGTH);
	}
	if(var_name[0] == '\0'){
		*c = beginning;
		return NULL;
	}

	old_int = read_dictionary(global_bound_variables, var_name, 0);

	new_int = num_bound_vars;
	write_dictionary(&global_bound_variables, var_name, &new_int, 0);

	output = create_sentence(EXISTS, num_bound_vars, num_bound_props);
	output->child0 = parse_sentence_value(c, num_bound_vars + 1, num_bound_props);
	output->child0->parent = output;

	write_dictionary(&global_bound_variables, var_name, old_int, 0);

	return output;
}

//Parse a sentence value beginning with parentheses
sentence *parse_parentheses(char **c, int num_bound_vars, int num_bound_props){
	sentence *output;

	skip_whitespace(c);
	if(**c != '('){
		return NULL;
	}
	++*c;
	output = parse_sentence(c, num_bound_vars, num_bound_props);

	if(**c != ')'){
		error(ERROR_END_PARENTHESES);
	}
	++*c;

	return output;
}

//Parse any sentence value
sentence *parse_sentence_value(char **c, int num_bound_vars, int num_bound_props){
	sentence *output;

	if((output = parse_true_false(c, num_bound_vars, num_bound_props))){
		return output;
	} else if((output = parse_relation(c, num_bound_vars, num_bound_props))){
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
		error(ERROR_SENTENCE_VALUE);
	}

	//NULL is never actually returned
	return NULL;
}

//Determine which operation is given by the next character in the source
//Returns -1 when the operation is unrecognized
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
	} else if((*c)[0] == '<' && (*c)[1] == '-' && (*c)[2] == '>'){
		*c += 3;
		return BICOND;
	} else {
		return -1;
	}
}

//Recursive sentence parser
static sentence *parse_sentence_recursive(int priority, sentence *s0, char **c, int num_bound_vars, int num_bound_props){
	sentence *s1 = NULL;
	sentence *output = NULL;
	int operation;
	char *temp_c = NULL;

	skip_whitespace(c);
	temp_c = *c;
	operation = get_operation(&temp_c);
	if(operation == -1 && **c && **c != ')' && **c != ';' && **c != ']' && **c != '{' && **c != '>'){
		error(ERROR_SENTENCE_OPERATION);
	} else if(!**c || **c == ')' || **c == ';' || **c == ']' || **c == '{' || **c == '>'){
		return s0;
	}

	if(order_of_operations[operation] < priority){
		return s0;
	}

	*c = temp_c;
	s1 = parse_sentence_value(c, num_bound_vars, num_bound_props);
	s1 = parse_sentence_recursive(order_of_operations[operation], s1, c, num_bound_vars, num_bound_props);
	output = create_sentence(operation, num_bound_vars, num_bound_props);
	output->child0 = s0;
	output->child1 = s1;
	output->child0->parent = output;
	output->child1->parent = output;

	return output;
}

//Parse a sentence
sentence *parse_sentence(char **c, int num_bound_vars, int num_bound_props){
	sentence *output;

	output = parse_sentence_value(c, num_bound_vars, num_bound_props);
	skip_whitespace(c);
	while(**c && **c != ')' && **c != ';' && **c != ']' && **c != '{' && **c != '>'){
		output = parse_sentence_recursive(0, output, c, num_bound_vars, num_bound_props);
		skip_whitespace(c);
	}

	return output;
}

//Free a sentence and its children recursively
//Decrement outside references
void free_sentence(sentence *s){
	int i;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
		case BICOND:
			free_sentence(s->child0);
			free_sentence(s->child1);
			free(s);
			break;
		case PROPOSITION:
			for(i = 0; i < s->num_args; i++){
				if(!s->proposition_args[i].is_bound){
					s->proposition_args[i].var->num_references--;
				}
			}
			free(s->proposition_args);
			if(!s->is_bound){
				s->definition_data->num_references--;
			}
			free(s);
			break;
		case FORALL:
		case EXISTS:
		case NOT:
			free_sentence(s->child0);
			free(s);
			break;
		case RELATION:
			if(!s->is_bound0){
				s->var0->num_references--;
			}
			if(!s->is_bound1){
				s->var1->num_references--;
			}
			s->relation_data->num_references--;
			free(s);
			break;
		default:
			free(s);
			break;
	}
}

//Free a sentence and its children recursively
//Don't decrement outside references
void free_sentence_independent(sentence *s){
	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
		case BICOND:
			free_sentence_independent(s->child0);
			free_sentence_independent(s->child1);
			free(s);
			break;
		case PROPOSITION:
			free(s->proposition_args);
			free(s);
			break;
		case FORALL:
		case EXISTS:
		case NOT:
			free_sentence_independent(s->child0);
			free(s);
			break;
		default:
			free(s);
			break;
	}
}

//Decrement all outside references of a sentence
void decrement_references_sentence(sentence *s){
	int i;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
		case BICOND:
			decrement_references_sentence(s->child0);
			decrement_references_sentence(s->child1);
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			decrement_references_sentence(s->child0);
			break;
		case PROPOSITION:
			for(i = 0; i < s->num_args; i++){
				if(!s->proposition_args[i].is_bound){
					s->proposition_args[i].var->num_references--;
				}
			}
			if(!s->is_bound){
				s->definition_data->num_references--;
			}
			break;
		case RELATION:
			if(!s->is_bound0){
				s->var0->num_references--;
			}
			if(!s->is_bound1){
				s->var1->num_references--;
			}
			s->relation_data->num_references--;
			break;
		case TRUE:
		case FALSE:
			//pass
			break;
	}
}

//Print a sentence recursively
void print_sentence(sentence *s){
	int i;

	if(s->type != FORALL && s->type != EXISTS){
		printf("(");
	}
	switch(s->type){
		case AND:
			print_sentence(s->child0);
			printf("&");
			print_sentence(s->child1);
			break;
		case OR:
			print_sentence(s->child0);
			printf("|");
			print_sentence(s->child1);
			break;
		case PROPOSITION:
			if(s->is_bound){
				printf("P%d(", s->definition_id);
			} else {
				printf("%s(", s->definition_data->name);
			}
			for(i = 0; i < s->num_args; i++){
				if(i){
					printf(", ");
				}
				if(s->proposition_args[i].is_bound){
					printf("%d", s->proposition_args[i].var_id);
				} else {
					printf("%s", s->proposition_args[i].var->name);
				}
			}
			printf(")");
			break;
		case FORALL:
			printf("*%d", s->num_bound_vars);
			print_sentence(s->child0);
			break;
		case EXISTS:
			printf("^%d", s->num_bound_vars);
			print_sentence(s->child0);
			break;
		case IMPLIES:
			print_sentence(s->child0);
			printf("->");
			print_sentence(s->child1);
			break;
		case BICOND:
			print_sentence(s->child0);
			printf("<->");
			print_sentence(s->child1);
			break;
		case NOT:
			printf("~");
			print_sentence(s->child0);
			break;
		case FALSE:
			printf("false");
			break;
		case TRUE:
			printf("true");
			break;
		case RELATION:
			if(s->is_bound0){
				printf("%d", s->var0_id);
			} else {
				printf("%s", s->var0->name);
			}
			printf(" %s ", s->relation_data->name);
			if(s->is_bound1){
				printf("%d", s->var1_id);
			} else {
				printf("%s", s->var1->name);
			}
			break;
	}
	if(s->type != FORALL && s->type != EXISTS){
		printf(")");
	}
}

//Determines whether one sentence trivially implies another sentence
//Returns 1 if it can determine that s0 implies s1, and 0 otherwise
int sentence_stronger(sentence *s0, sentence *s1){
	int i;

	if(s0->num_bound_vars != s1->num_bound_vars || s0->num_bound_props != s1->num_bound_props){
		return 0;
	} else if(s0->type == OR){
		return sentence_stronger(s0->child0, s1) && sentence_stronger(s0->child1, s1);
	} else if(s1->type == AND){
		return sentence_stronger(s0, s1->child0) && sentence_stronger(s0, s1->child1);
	} else if(s0->type == NOT && s1->type == NOT){
		return sentence_stronger(s1->child0, s0->child0);
	} else if(s0->type == IMPLIES && s1->type == IMPLIES){
		return sentence_stronger(s1->child0, s0->child0) && sentence_stronger(s0->child1, s1->child1);
	} else if(s0->type == FORALL && s1->type == FORALL){
		return sentence_stronger(s0->child0, s1->child0);
	} else if(s0->type == EXISTS && s1->type == EXISTS){
		return sentence_stronger(s0->child0, s1->child0);
	} else if(s0->type == BICOND && s1->type == BICOND){
		if(sentence_equivalent(s0->child0, s1->child0) && sentence_equivalent(s0->child1, s1->child1)){
			return 1;
		}
		if(sentence_equivalent(s0->child0, s1->child1) && sentence_equivalent(s0->child1, s1->child0)){
			return 1;
		}
		if(sentence_equivalent(s0->child0, s0->child1) && sentence_equivalent(s1->child0, s1->child1)){
			return 1;
		}
		return 0;
	} else if(s1->type == BICOND){
		return sentence_equivalent(s1->child0, s1->child1);
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
	} else if(sentence_trivially_false(s0)){
		return 1;
	} else if(sentence_trivially_true(s1)){
		return 1;
	} else {
		if(s0->type == AND && (sentence_stronger(s0->child0, s1) || sentence_stronger(s0->child1, s1))){
			return 1;
		}
		if(s1->type == OR && (sentence_stronger(s0, s1->child0) || sentence_stronger(s0, s1->child1))){
			return 1;
		}
		return 0;
	}
}

//Determine if two sentences imply each other
int sentence_equivalent(sentence *s0, sentence *s1){
	return sentence_stronger(s0, s1) && sentence_stronger(s1, s0);
}

//Determine if a sentence is trivially true
int sentence_trivially_true(sentence *s){
	if(s->num_bound_vars > 0 || s->num_bound_props > 0){
		return 0;
	} else if(s->type == IMPLIES){
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

//Determine if a sentence is trivially false
int sentence_trivially_false(sentence *s){
	int true0;
	int true1;
	int false0;
	int false1;

	if(s->num_bound_vars > 0 || s->num_bound_props > 0){
		return 0;
	} else if(s->type == IMPLIES){
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
	} else if(s->type == EXISTS || s->type == FORALL){
		return sentence_trivially_false(s->child0);
	} else if(s->type == NOT){
		return sentence_trivially_true(s->child0);
	} else if(s->type == FALSE){
		return 1;
	} else {
		return 0;
	}
}

//Copies a sentence into the destination and allocates as needed
void copy_sentence(sentence *dest, sentence *s){
	int i;

	dest->type = s->type;
	dest->num_bound_vars = s->num_bound_vars;
	dest->num_bound_props = s->num_bound_props;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
		case BICOND:
			dest->child0 = malloc(sizeof(sentence));
			dest->child1 = malloc(sizeof(sentence));
			copy_sentence(dest->child0, s->child0);
			copy_sentence(dest->child1, s->child1);
			dest->child0->parent = dest;
			dest->child1->parent = dest;
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			dest->child0 = malloc(sizeof(sentence));
			copy_sentence(dest->child0, s->child0);
			dest->child0->parent = dest;
			break;
		case RELATION:
			dest->relation_data = s->relation_data;
			dest->relation_data->num_references++;
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
				dest->definition_id = s->definition_id;
			} else {
				dest->is_bound = 0;
				dest->definition_data = s->definition_data;
				dest->definition_data->num_references++;
			}
			dest->proposition_args = malloc(sizeof(proposition_arg)*s->num_args);
			memcpy(dest->proposition_args, s->proposition_args, sizeof(proposition_arg)*s->num_args);
			dest->num_args = s->num_args;
			for(i = 0; i < dest->num_args; i++){
				if(!dest->proposition_args[i].is_bound){
					dest->proposition_args[i].var->num_references++;
				}
			}
			break;
		case TRUE:
		case FALSE:
			//pass
			break;
	}
}

//Peel the left-most part of an "|" off of a sentence and return it
sentence *peel_or_left(sentence **s){
	sentence *output = NULL;
	sentence *child = NULL;

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

//Peel the left-most part of an "&" off of a sentence and return it
sentence *peel_and_left(sentence **s){
	sentence *output = NULL;
	sentence *child = NULL;

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

//Count the number of sub-sentences which are "|"-ed together
//Used to determine the number of times peel_or_left can be called
int count_or(sentence *s){
	if(s->type != OR){
		return 1;
	}

	return count_or(s->child0) + count_or(s->child1);
}

