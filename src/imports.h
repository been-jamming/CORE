#include "predicate.h"
#include "dictionary.h"

typedef enum{
	AXIOM_DEPEND,
	OBJECT_DEPEND,
	DEFINITION_DEPEND,
	RELATION_DEPEND,
	CONTEXT_DEPEND
} dependency_type;

typedef struct dependency dependency;

struct dependency{
	dependency_type type;
	union{
		//Axiom, object, and context
		variable *var;
		//Definition
		definition *def;
		//Relation
		relation *rel;
	};
	dependency *next;
	dependency *previous;
	//Context
	dependency *children;
};

typedef struct import_entry import_entry;

struct import_entry{
	char *import_path;
	context *import_context;
	dependency *dependencies;
};

extern dictionary global_imports;
extern import_entry *global_import_entry;
extern dependency **global_dependencies;
extern context *global_root_context;

import_entry *get_import_entry(char *file_name);
void reset_destinations(context *c);

void add_axiom_dependency(variable *var);
void add_object_dependency(variable *var);
void add_definition_dependency(definition *def);
void add_relation_dependency(relation *rel);
dependency *add_context_dependency(variable *var);

void transfer_variable_void(void *v);
void transfer_relation_void(void *v);
void transfer_definition_void(void *v);
sentence *transfer_sentence(sentence *s, sentence *parent);
variable *transfer_variable(variable *var);
relation *transfer_relation(relation *rel);
definition *transfer_definition(definition *def);
context *transfer_context(context *c);

void check_dependencies(dependency *search_dependency);
void import_context(context *c);
