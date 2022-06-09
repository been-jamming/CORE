#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include "custom_malloc.h"
#include "dictionary.h"
#include "predicate.h"
#include "expression.h"
#include "commands.h"
#include "imports.h"

dictionary global_imports;
import_entry *global_import_entry = NULL;

import_entry *create_import_entry(char *import_path, context *import_context){
	import_entry *output;

	output = malloc(sizeof(import_entry));
	output->import_path = import_path;
	output->import_context = import_context;
	output->dependencies = NULL;
	
	return output;
}

void free_import_entry(import_entry *entry){
	dependency *current_dependency;
	dependency *next_dependency;

	current_dependency = entry->dependencies;
	while(current_dependency){
		next_dependency = current_dependency->next;
		free(current_dependency);
		current_dependency = next_dependency;
	}
	free(entry->import_path);
	free_context(entry->import_context);
	free(entry);
}

void free_import_entry_void(void *v){
	free_import_entry(v);
}

void init_verifier(void){
	global_imports = create_dictionary(NULL);
}

import_entry *get_import_entry(char *file_name){
	char full_path[PATH_MAX];
	char *path_return;
	char *path;
	import_entry *output;
	char *program_text;
	char *program_start;
	char *old_file_name;
	char **old_program_pointer;
	char *old_program_start;
	context *old_context;
	import_entry *old_import_entry;
	unsigned int old_line_number;
	expr_value *return_value;

	path_return = realpath(file_name, full_path);
	if(!path_return){
		error(ERROR_FILE_READ);
	}
	path = malloc(sizeof(char)*(strlen(full_path) + 1));
	strcpy(path, full_path);
	output = read_dictionary(global_imports, path, 0);
	if(output){
		return output;
	} else {
		old_file_name = global_file_name;
		old_program_pointer = global_program_pointer;
		old_program_start = global_program_start;
		old_context = global_context;
		old_line_number = global_line_number;
		old_import_entry = global_import_entry;

		global_file_name = file_name;
		program_start = load_file(file_name);
		if(!program_start){
			error(ERROR_FILE_READ);
		}
		program_text = program_start;

		global_context = create_context(NULL);
		global_line_number = 1;
		global_program_pointer = &program_text;
		global_program_start = program_start;
		return_value = parse_context(&program_text);
		global_import_entry = create_import_entry(path, global_context);
		if(*program_text == '}'){
			error(ERROR_EOF);
		}
		if(return_value){
			printf("file '%s' returned: ", global_file_name);
			print_expr_value(return_value);
			printf("\n");
			free_expr_value(return_value);
		}
		output = global_import_entry;
		write_dictionary(&global_imports, path, output, 0);

		global_file_name = old_file_name;
		global_program_pointer = old_program_pointer;
		global_program_start = old_program_start;
		global_context = old_context;
		global_line_number = old_line_number;
		global_import_entry = old_import_entry;
		free(program_start);

		return output;
	}
}

int main(int argc, char **argv){
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
		get_import_entry(argv[i]);
	}
	free_dictionary(&global_imports, free_import_entry_void);
	clear_bound_variables();
	clear_bound_propositions();

#ifdef USE_CUSTOM_ALLOC
	custom_malloc_deinit();
#endif

	printf("Proof verification successful.\n");
	return 0;
}

