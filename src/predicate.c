#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "dictionary.h"
#include "predicate.h"
#include "custom_malloc.h"

//CORE First Order Logic Parser
//Ben Jones

//The parser is recursive and maintains state by passing a double pointer (char **) when it calls a function.

int global_relation_id;
dictionary global_bound_variables;
dictionary global_bound_propositions;
unsigned int global_line_number;
char *global_file_name;
char **global_program_pointer;
char *global_program_start;

//Predicate parsing order of operations
static int order_of_operations[4] = {3, 2, 1, 0};
//All CORE error messages
static char *error_messages[] = {
	
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
	fprintf(stderr, "\n%*c^\n", (int) (error_place - line_start + num_digits_uint(line_number) + 1), ' ');

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
		return;
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
	while(!is_alpha(*end_of_identifier) && !is_whitespace(end_of_identifier) && *end_of_identifier && *end_of_identifier != ';' && *end_of_identifier != '[' && *end_of_identifier != ']'){
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

int main(int argc, char **argv){

}

