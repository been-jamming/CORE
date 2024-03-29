#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <libgen.h>
#include "custom_malloc.h"
#include "dictionary.h"
#include "predicate.h"
#include "compare.h"
#include "expression.h"
#include "commands.h"
#include "imports.h"

//CORE import system
//Ben Jones

dictionary global_imports;
import_entry *global_import_entry = NULL;
dependency **global_dependencies = NULL;
context *global_root_context = NULL;

//&RECURSIVE_IMPORT will be stored in global_imports to represent when an import entry
//is being created. This will prevent recursive imports.
static import_entry RECURSIVE_IMPORT;

import_entry *create_import_entry(char *import_path, context *import_context){
	import_entry *output;

	output = malloc(sizeof(import_entry));
	output->import_path = import_path;
	output->import_context = import_context;
	output->dependencies = NULL;
	
	return output;
}

void free_dependencies(dependency *depends){
	dependency *next_dependency;

	while(depends){
		if(depends->type == CONTEXT_DEPEND){
			free_dependencies(depends->children);
		}
		next_dependency = depends->next;
		free(depends);
		depends = next_dependency;
	}
}

void free_import_entry(import_entry *entry){
	free_dependencies(entry->dependencies);
	free(entry->import_path);
	free_context(entry->import_context);
	free(entry);
}

void free_import_entry_void(void *v){
	free_import_entry(v);
}

void free_none(void *v){

}

void init_verifier(void){
	static char *pointer = "";

	global_imports = create_dictionary(NULL);
	global_recursive_include = create_dictionary(NULL);
	global_program_pointer = &pointer;
	global_program_start = pointer;
}

void deinit_verifier(void){
	free_dictionary(&global_imports, free_import_entry_void);
	free_dictionary(&global_recursive_include, free_none);
	clear_bound_variables();
	clear_bound_propositions();
}

import_entry *get_import_entry(char *file_name){
	char *path;
	char *dirc;
	char *basec;
	char *dname;
	char *bname;
	char working_directory[PATH_MAX];
	import_entry *output;
	char *program_text;
	char *program_start;
	char *old_file_name;
	char **old_program_pointer;
	char *old_program_start;
	context *old_context;
	import_entry *old_import_entry;
	dependency **old_dependencies;
	unsigned int old_line_number;
	expr_value *return_value;

	path = realpath(file_name, NULL);
	if(!path){
		error(ERROR_PARSE_PATH);
	}
	custom_register(path, 0);
	output = read_dictionary(global_imports, path, 0);
	if(output && output != &RECURSIVE_IMPORT){
		free(path);
		return output;
	} else if(output == &RECURSIVE_IMPORT){
		error(ERROR_RECURSIVE_IMPORT);
	} else {
		dirc = strdup(path);
		if(!dirc){
			error(ERROR_PARSE_PATH);
		}
		custom_register(dirc, 0);
		basec = strdup(path);
		if(!basec){
			error(ERROR_PARSE_PATH);
		}
		custom_register(basec, 0);
		dname = dirname(dirc);
		bname = basename(basec);
		if(!getcwd(working_directory, PATH_MAX)){
			error(ERROR_GET_WORKING_DIRECTORY);
		}
		if(chdir(dname)){
			error(ERROR_CHANGE_DIRECTORY);
		}

		old_program_pointer = global_program_pointer;
		old_program_start = global_program_start;
		old_context = global_context;
		old_line_number = global_line_number;
		old_import_entry = global_import_entry;
		old_dependencies = global_dependencies;
		old_file_name = global_file_name;

		program_start = load_file(bname);
		if(!program_start){
			error(ERROR_FILE_READ);
		}
		program_text = program_start;

		global_context = create_context(NULL);
		global_line_number = 1;
		global_program_pointer = &program_text;
		global_program_start = program_start;
		global_import_entry = create_import_entry(path, global_context);
		global_dependencies = &(global_import_entry->dependencies);
		global_file_name = file_name;

		//This write prevents recursive imports
		write_dictionary(&global_imports, path, &RECURSIVE_IMPORT, 0);

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
		output = global_import_entry;
		write_dictionary(&global_imports, path, output, 0);

		global_file_name = old_file_name;
		global_program_pointer = old_program_pointer;
		global_program_start = old_program_start;
		global_context = old_context;
		global_line_number = old_line_number;
		global_import_entry = old_import_entry;
		global_dependencies = old_dependencies;

		free(program_start);
		free(dirc);
		free(basec);
		if(chdir(working_directory)){
			error(ERROR_CHANGE_DIRECTORY);
		}

		return output;
	}

	//Execution never reaches this return
	return NULL;
}

void print_dependencies(import_entry *entry){
	dependency *dep;

	printf("Dependencies:\n");
	dep = entry->dependencies;
	while(dep){
		switch(dep->type){
			case AXIOM_DEPEND:
				printf("axiom %s: ", dep->var->name);
				print_sentence(dep->var->sentence_data);
				printf("\n");
				break;
			case OBJECT_DEPEND:
				printf("object %s\n", dep->var->name);
				break;
			case DEFINITION_DEPEND:
				printf("definition %s(%d)\n", dep->def->name, dep->def->num_args);
				break;
			case RELATION_DEPEND:
				printf("relation %s\n", dep->rel->name);
				break;
			case CONTEXT_DEPEND:
				printf("context %s\n", dep->var->name);
				break;
		}
		dep = dep->next;
	}
}

void add_axiom_dependency(variable *var){
	dependency *next;

	next = malloc(sizeof(dependency));
	next->type = AXIOM_DEPEND;
	next->var = var;
	next->var->num_references++;
	next->next = *global_dependencies;
	next->previous = NULL;
	next->children = NULL;
	if(next->next){
		next->next->previous = next;
	}
	*global_dependencies = next;

	var->num_references++;
}

void add_object_dependency(variable *var){
	dependency *next;

	next = malloc(sizeof(dependency));
	next->type = OBJECT_DEPEND;
	next->var = var;
	next->var->num_references++;
	next->next = *global_dependencies;
	next->previous = NULL;
	next->children = NULL;
	if(next->next){
		next->next->previous = next;
	}
	*global_dependencies = next;

	var->num_references++;
}

void add_definition_dependency(definition *def){
	dependency *next;

	next = malloc(sizeof(dependency));
	next->type = DEFINITION_DEPEND;
	next->def = def;
	next->def->num_references++;
	next->next = *global_dependencies;
	next->previous = NULL;
	next->children = NULL;
	if(next->next){
		next->next->previous = next;
	}
	*global_dependencies = next;

	def->num_references++;
}

void add_relation_dependency(relation *rel){
	dependency *next;

	next = malloc(sizeof(dependency));
	next->type = RELATION_DEPEND;
	next->rel = rel;
	next->rel->num_references++;
	next->next = *global_dependencies;
	next->previous = NULL;
	next->children = NULL;
	if(next->next){
		next->next->previous = next;
	}
	*global_dependencies = next;

	rel->num_references++;
}

dependency *add_context_dependency(variable *var){
	dependency *next;

	next = malloc(sizeof(dependency));
	next->type = CONTEXT_DEPEND;
	next->children = NULL;
	next->var = var;
	next->next = *global_dependencies;
	next->previous = NULL;
	if(next->next){
		next->next->previous = next;
	}
	*global_dependencies = next;

	var->num_references++;

	return next;
}

void reset_destination_variable(variable *var){
	var->destination = NULL;
	if(var->type == CONTEXT_VAR){
		reset_destinations(var->context_data);
	}
}

void reset_destination_variable_void(void *v){
	reset_destination_variable(v);
}

void reset_destination_definition(definition *def){
	def->destination = NULL;
}

void reset_destination_definition_void(void *v){
	reset_destination_definition(v);
}

void reset_destination_relation(relation *rel){
	rel->destination = NULL;
}

void reset_destination_relation_void(void *v){
	reset_destination_relation(v);
}

void reset_destinations(context *c){
	c->destination = NULL;
	iterate_dictionary(c->variables, reset_destination_variable_void);
	iterate_dictionary(c->definitions, reset_destination_definition_void);
	iterate_dictionary(c->relations, reset_destination_relation_void);
}

variable *transfer_variable(variable *var){
	variable *output;

	output = var->destination;
	if(!output){
		//Error if the variable already exists but is not compatible
		output = read_dictionary(transfer_context(var->parent_context)->variables, var->name, 0);
		if(output && (var->type != SENTENCE_VAR || output->type != SENTENCE_VAR)){
			global_error_arg0 = var->name;
			error(ERROR_IMPORT_VARIABLE);
		}
		if(var->type == OBJECT_VAR){
			output = create_object_variable(var->name, transfer_context(var->parent_context));
		} else if(var->type == SENTENCE_VAR){
			output = create_sentence_variable(var->name, transfer_sentence(var->sentence_data, NULL), var->verified, transfer_context(var->parent_context));
		} else if(var->type == CONTEXT_VAR){
			output = create_context_variable(var->name, transfer_context(var->context_data), transfer_context(var->parent_context));
			iterate_dictionary(var->context_data->variables, transfer_variable_void);
			iterate_dictionary(var->context_data->relations, transfer_relation_void);
			iterate_dictionary(var->context_data->definitions, transfer_definition_void);
		}
		var->destination = output;
	}
	
	return output;
}

void transfer_variable_void(void *v){
	transfer_variable(v);
}

relation *transfer_relation(relation *rel){
	relation *output;

	output = rel->destination;
	if(!output){
		//Error if the relation already exists but is not the destination
		output = read_dictionary(transfer_context(rel->parent_context)->relations, rel->name, 0);
		if(output){
			global_error_arg0 = rel->name;
			error(ERROR_IMPORT_RELATION);
		}
		output = create_relation(rel->name, transfer_sentence(rel->sentence_data, NULL), transfer_context(rel->parent_context));
		rel->destination = output;
	}

	return output;
}

void transfer_relation_void(void *v){
	transfer_relation(v);
}

definition *transfer_definition(definition *def){
	definition *output;

	output = def->destination;
	if(!output){
		//Error if the definition already exists but is not the destination
		output = read_dictionary(transfer_context(def->parent_context)->definitions, def->name, 0);
		if(output){
			global_error_arg0 = def->name;
			error(ERROR_IMPORT_DEFINITION);
		}
		output = create_definition(def->name, transfer_sentence(def->sentence_data, NULL), def->num_args, transfer_context(def->parent_context));
		def->destination = output;
	}

	return output;
}

void transfer_definition_void(void *v){
	transfer_definition(v);
}

context *transfer_context(context *c){
	context *output;

	output = c->destination;
	if(!output){
		if(c->parent == NULL){
			output = global_root_context;
		} else {
			output = create_context(transfer_context(c->parent));
		}
		c->destination = output;
	}
	return output;
}

sentence *transfer_sentence(sentence *s, sentence *parent){
	sentence *output;
	int i;

	output = create_sentence(s->type, s->num_bound_vars, s->num_bound_props);
	output->parent = parent;
	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
		case BICOND:
			output->child0 = transfer_sentence(s->child0, output);
			output->child1 = transfer_sentence(s->child1, output);
			break;
		case NOT:
		case EXISTS:
		case FORALL:
			output->child0 = transfer_sentence(s->child0, output);
			break;
		case RELATION:
			output->relation_data = transfer_relation(s->relation_data);
			output->relation_data->num_references++;
			output->is_bound0 = s->is_bound0;
			if(s->is_bound0){
				output->var0_id = s->var0_id;
			} else {
				output->var0 = transfer_variable(s->var0);
				output->var0->num_references++;
			}
			output->is_bound1 = s->is_bound1;
			if(s->is_bound1){
				output->var1_id = s->var1_id;
			} else {
				output->var1 = transfer_variable(s->var1);
				output->var1->num_references++;
			}
			break;
		case PROPOSITION:
			output->is_bound = s->is_bound;
			if(s->is_bound){
				output->definition_id = s->definition_id;
			} else {
				output->definition_data = transfer_definition(s->definition_data);
				output->definition_data->num_references++;
			}
			output->num_args = s->num_args;
			output->proposition_args = malloc(sizeof(proposition_arg)*output->num_args);
			for(i = 0; i < output->num_args; i++){
				output->proposition_args[i].is_bound = s->proposition_args[i].is_bound;
				if(s->proposition_args[i].is_bound){
					output->proposition_args[i].var_id = s->proposition_args[i].var_id;
				} else {
					output->proposition_args[i].var = transfer_variable(s->proposition_args[i].var);
					output->proposition_args[i].var->num_references++;
				}
			}
			break;
		default:
			break;
	}

	return output;
}

void check_axiom_dependency(variable *var){
	sentence *new_sentence;
	variable *check_var;

	check_var = get_variable(var->name, global_context, var->parent_context->dependent);
	if(!check_var){
		global_error_arg0 = var->name;
		error(ERROR_VARIABLE_NOT_FOUND);
	}
	if(check_var->type != SENTENCE_VAR){
		global_error_arg0 = var->name;
		error(ERROR_AXIOM_INCOMPATIBLE);
	}
	new_sentence = transfer_sentence(var->sentence_data, NULL);
	if(!sentence_stronger(check_var->sentence_data, new_sentence)){
		global_error_arg0 = var->name;
		printf("sentence 0: ");
		print_sentence(check_var->sentence_data);
		printf("\nsentence 1: ");
		print_sentence(new_sentence);
		printf("\n");
		error(ERROR_AXIOM_INCOMPATIBLE);
	}
	free_sentence(new_sentence);
	var->destination = check_var;
}

void check_object_dependency(variable *var){
	variable *check_var;

	check_var = get_variable(var->name, global_context, var->parent_context->dependent);
	if(!check_var){
		global_error_arg0 = var->name;
		error(ERROR_VARIABLE_NOT_FOUND);
	}
	if(check_var->type != OBJECT_VAR){
		global_error_arg0 = var->name;
		error(ERROR_OBJECT_INCOMPATIBLE);
	}
	var->destination = check_var;
}

void check_definition_dependency(definition *def){
	definition *check_definition;
	context *search_context;
	sentence *new_sentence;

	search_context = global_context;
	do{
		check_definition = read_dictionary(search_context->definitions, def->name, 0);
		if(check_definition){
			break;
		}
		search_context = search_context->parent;
	} while(search_context && !def->parent_context->dependent);

	if(!check_definition){
		error(ERROR_DEFINITION_EXISTS);
	}
	if(def->num_args != check_definition->num_args){
		global_error_arg0 = def->name;
		error(ERROR_DEFINITION_INCOMPATIBLE);
	}
	if(def->sentence_data && !check_definition->sentence_data){
		global_error_arg0 = def->name;
		error(ERROR_DEFINITION_INCOMPATIBLE);
	}
	if(def->sentence_data){
		new_sentence = transfer_sentence(def->sentence_data, NULL);
		if(!check_definition->sentence_data || !sentence_equivalent(new_sentence, check_definition->sentence_data)){
			global_error_arg0 = def->name;
			error(ERROR_DEFINITION_INCOMPATIBLE);
		}
		free_sentence(new_sentence);
	}
	def->destination = check_definition;
}

void check_relation_dependency(relation *rel){
	relation *check_relation;
	context *search_context;
	sentence *new_sentence;

	search_context = global_context;
	do{
		check_relation = read_dictionary(search_context->relations, rel->name, 0);

		if(check_relation){
			break;
		}
		search_context = search_context->parent;
	} while(search_context && !rel->parent_context->dependent);

	if(!check_relation){
		error(ERROR_RELATION_EXISTS);
	}
	if(rel->sentence_data && !check_relation->sentence_data){
		error(ERROR_RELATION_INCOMPATIBLE);
	}
	if(rel->sentence_data){
		new_sentence = transfer_sentence(rel->sentence_data, NULL);
		if(!check_relation->sentence_data || !sentence_equivalent(new_sentence, check_relation->sentence_data)){
			error(ERROR_RELATION_INCOMPATIBLE);
		}
		free_sentence(new_sentence);
	}
	rel->destination = check_relation;
}

void check_context_dependency(variable *var, dependency *children){
	context *old_context;
	variable *check_variable;

	check_variable = get_variable(var->name, global_context, var->parent_context->dependent);
	if(!check_variable){
		global_error_arg0 = var->name;
		error(ERROR_VARIABLE_NOT_FOUND);
	}
	if(check_variable->type != CONTEXT_VAR){
		error(ERROR_CONTEXT_INCOMPATIBLE);
	}
	var->destination = check_variable;
	old_context = global_context;
	global_context = check_variable->context_data;
	check_dependencies(children);
	global_context = old_context;
}

void check_dependency(dependency *dep){
	switch(dep->type){
		case AXIOM_DEPEND:
			check_axiom_dependency(dep->var);
			break;
		case OBJECT_DEPEND:
			check_object_dependency(dep->var);
			break;
		case DEFINITION_DEPEND:
			check_definition_dependency(dep->def);
			break;
		case RELATION_DEPEND:
			check_relation_dependency(dep->rel);
			break;
		case CONTEXT_DEPEND:
			check_context_dependency(dep->var, dep->children);
			break;
	}
}

void check_dependencies(dependency *search_dependency){
	if(!search_dependency){
		return;
	}
	while(search_dependency->next){
		search_dependency = search_dependency->next;
	}
	while(search_dependency){
		check_dependency(search_dependency);
		search_dependency = search_dependency->previous;
	}
}

void import_context(context *c){
	iterate_dictionary(c->variables, transfer_variable_void);
	iterate_dictionary(c->relations, transfer_relation_void);
	iterate_dictionary(c->definitions, transfer_definition_void);
}

int main(int argc, char **argv){
	int i;
	import_entry *entry;

	if(argc < 2){
		fprintf(stderr, "Error: no input files\n");
		return 1;
	}

#ifdef USE_CUSTOM_ALLOC
	custom_malloc_init();
#endif

	init_verifier();

	for(i = 1; i < argc; i++){
		entry = get_import_entry(argv[i]);
		print_dependencies(entry);
	}

	deinit_verifier();

#ifdef USE_CUSTOM_ALLOC
	custom_malloc_deinit();
#endif

	printf("Proof verification successful.\n");
	return 0;
}

