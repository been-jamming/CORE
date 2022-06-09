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
};

typedef struct import_entry import_entry;

struct import_entry{
	char *import_path;
	context *import_context;
	dependency *dependencies;
};

extern dictionary global_imports;
extern import_entry *global_import_entry;

