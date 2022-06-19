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

//CORE import system
//Ben Jones

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

void deinit_verifier(void){
	free_dictionary(&global_imports, free_import_entry_void);
	clear_bound_variables();
	clear_bound_propositions();
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

	old_file_name = global_file_name;
	global_file_name = file_name;
	path_return = realpath(file_name, full_path);
	if(!path_return){
		error(ERROR_FILE_READ);
	}
	path = malloc(sizeof(char)*(strlen(full_path) + 1));
	strcpy(path, full_path);
	output = read_dictionary(global_imports, path, 0);
	if(output){
		global_file_name = old_file_name;
		return output;
	} else {
		old_program_pointer = global_program_pointer;
		old_program_start = global_program_start;
		old_context = global_context;
		old_line_number = global_line_number;
		old_import_entry = global_import_entry;

		program_start = load_file(file_name);
		if(!program_start){
			error(ERROR_FILE_READ);
		}
		program_text = program_start;

		global_context = create_context(NULL);
		global_line_number = 1;
		global_program_pointer = &program_text;
		global_program_start = program_start;
		global_import_entry = create_import_entry(path, global_context);
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
		free(program_start);

		return output;
	}
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
	next->next = global_import_entry->dependencies;
	next->previous = NULL;
	if(next->next){
		next->next->previous = next;
	}
	global_import_entry->dependencies = next;
}

void add_object_dependency(variable *var){
	dependency *next;

	next = malloc(sizeof(dependency));
	next->type = OBJECT_DEPEND;
	next->var = var;
	next->var->num_references++;
	next->next = global_import_entry->dependencies;
	next->previous = NULL;
	if(next->next){
		next->next->previous = next;
	}
	global_import_entry->dependencies = next;
}

void add_definition_dependency(definition *def){
	dependency *next;

	next = malloc(sizeof(dependency));
	next->type = DEFINITION_DEPEND;
	next->def = def;
	next->def->num_references++;
	next->next = global_import_entry->dependencies;
	next->previous = NULL;
	if(next->next){
		next->next->previous = next;
	}
	global_import_entry->dependencies = next;
}

void add_relation_dependency(relation *rel){
	dependency *next;

	next = malloc(sizeof(dependency));
	next->type = RELATION_DEPEND;
	next->rel = rel;
	next->rel->num_references++;
	next->next = global_import_entry->dependencies;
	next->previous = NULL;
	if(next->next){
		next->next->previous = next;
	}
	global_import_entry->dependencies = next;
}

void reset_destination_variable(variable *var){
	var->destination = NULL;
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
	iterate_dictionary(c->variables, reset_destination_variable_void);
	iterate_dictionary(c->definitions, reset_destination_definition_void);
	iterate_dictionary(c->relations, reset_destination_relation_void);
}

variable *transfer_variable(variable *var){
	variable *output;

	output = var->destination;
	if(!output){
		//Error if the variable already exists but is not compatible
		output = read_dictionary(global_context->variables, var->name, 0);
		if(output && (var->type != SENTENCE_VAR || output->type != SENTENCE_VAR)){
			error(ERROR_IMPORT_VARIABLE);
		}
		if(var->type == OBJECT_VAR){
			output = create_object_variable(var->name, global_context);
		} else if(var->type == SENTENCE_VAR){
			output = create_sentence_variable(var->name, transfer_sentence(var->sentence_data, NULL), var->verified, global_context);
		} else if(var->type == CONTEXT_VAR){
			output = create_context_variable(var->name, transfer_context(var->context_data), global_context);
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
		output = read_dictionary(global_context->relations, rel->name, 0);
		if(output){
			error(ERROR_IMPORT_RELATION);
		}
		output = create_relation(rel->name, transfer_sentence(rel->sentence_data, NULL), global_context);
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
		output = read_dictionary(global_context->definitions, def->name, 0);
		if(output){
			error(ERROR_IMPORT_DEFINITION);
		}
		output = create_definition(def->name, transfer_sentence(def->sentence_data, NULL), def->num_args, global_context);
		def->destination = output;
	}

	return output;
}

void transfer_definition_void(void *v){
	transfer_definition(v);
}

void transfer_variable_no_context_void(void *v){
	if(((variable *) v)->type != CONTEXT_VAR){
		transfer_variable(v);
	}
}

void transfer_variable_context_void(void *v){
	if(((variable *) v)->type == CONTEXT_VAR){
		transfer_variable(v);
	}
}

context *transfer_context(context *c){
	context *output;
	context *parent_context;

	reset_destinations(c);
	parent_context = global_context;
	output = create_context(parent_context);
	global_context = output;
	iterate_dictionary(c->variables, transfer_variable_no_context_void);
	iterate_dictionary(c->relations, transfer_relation_void);
	iterate_dictionary(c->definitions, transfer_definition_void);
	iterate_dictionary(c->variables, transfer_variable_context_void);
	global_context = parent_context;

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

	check_var = get_variable(var->name, global_context);
	if(!check_var){
		error(ERROR_VARIABLE_NOT_FOUND);
	}
	if(check_var->type != SENTENCE_VAR){
		error(ERROR_AXIOM_INCOMPATIBLE);
	}
	new_sentence = transfer_sentence(var->sentence_data, NULL);
	if(!sentence_stronger(check_var->sentence_data, new_sentence)){
		error(ERROR_AXIOM_INCOMPATIBLE);
	}
	free_sentence(new_sentence);
	var->destination = check_var;
}

void check_object_dependency(variable *var){
	variable *check_var;

	check_var = get_variable(var->name, global_context);
	if(!check_var){
		error(ERROR_VARIABLE_NOT_FOUND);
	}
	if(check_var->type != OBJECT_VAR){
		error(ERROR_OBJECT_INCOMPATIBLE);
	}
	var->destination = check_var;
}

void check_definition_dependency(definition *def){
	definition *check_definition;
	context *search_context;

	search_context = global_context;
	while(search_context){
		check_definition = read_dictionary(search_context->definitions, def->name, 0);
		if(check_definition){
			break;
		}
		search_context = search_context->parent;
	}

	if(!check_definition){
		error(ERROR_DEFINITION_EXISTS);
	}
	if(def->num_args != check_definition->num_args){
		error(ERROR_DEFINITION_INCOMPATIBLE);
	}
	def->destination = check_definition;
}

void check_relation_dependency(relation *rel){
	relation *check_relation;
	context *search_context;

	search_context = global_context;
	while(search_context){
		check_relation = read_dictionary(search_context->relations, rel->name, 0);

		if(check_relation){
			break;
		}
		search_context = search_context->parent;
	}

	if(!check_relation){
		error(ERROR_RELATION_EXISTS);
	}
	rel->destination = check_relation;
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
	}
}

void check_dependencies(import_entry *entry){
	dependency *search_dependency;

	search_dependency = entry->dependencies;
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
	iterate_dictionary(c->variables, transfer_variable_no_context_void);
	iterate_dictionary(c->relations, transfer_relation_void);
	iterate_dictionary(c->definitions, transfer_definition_void);
	iterate_dictionary(c->variables, transfer_variable_context_void);
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

