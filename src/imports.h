#include "predicate.h"
#include "dictionary.h"

typedef enum{
	AXIOM_DEPEND,
	OBJECT_DEPEND,
	DEFINITION_DEPEND,
	RELATION_DEPEND
} dependency_type;

typedef struct dependency dependency;

struct dependency{
	dependency_type type;
	union{
		//Axiom and object
		variable *var;
		//Definition
		definition *def;
		//Relation
		relation *rel;
	};
	dependency *next;
	dependency *previous;
};

typedef struct import_entry import_entry;

struct import_entry{
	char *import_path;
	context *import_context;
	dependency *dependencies;
};

extern dictionary global_imports;
extern import_entry *global_import_entry;

void add_axiom_dependency(variable *var);
void add_object_dependency(variable *var);
void add_definition_dependency(definition *def);
void add_relation_dependency(relation *rel);

sentence *transfer_sentence(sentence *s, sentence *parent);
variable *transfer_object(variable *obj);
relation *transfer_relation(relation *rel);
definition *transfer_definition(definition *def);

