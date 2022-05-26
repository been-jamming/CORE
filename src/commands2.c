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

definition *create_definition(char *name, sentence *sentence_data, context *parent_context){
	definition *def;

	def = malloc(sizeof(def));
	if(sentence_data){
		def->sentence_data = malloc(sizeof(sentence));
		copy_sentence(def->sentence_data, sentence_data);
	} else {
		def->sentnece_data = NULL;
	}
	def->name = malloc(sizeof(char)*(strlen(name) + 1));
	strcpy(def->name, name);
	def->num_args = def->sentence_data->num_bound_vars;
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

