#define MAX_DEPTH 128

extern int global_relation_id;
extern dictionary global_bound_variables;
extern dictionary global_bound_propositions;
extern unsigned int global_line_number;
extern char *global_file_name;
extern char **global_program_pointer;
extern char *global_program_start;

typedef enum{
	AND,
	OR,
	IMPLIES,
	BICOND,
	EXISTS,
	FORALL,
	NOT,
	RELATION,
	PROPOSITION,
	FALSE,
	TRUE
} sentence_type;

typedef enum{
	OBJECT,
	SENTENCE,
	CONTEXT
} variable_type;

typedef struct sentence sentence;
typedef struct variable variable;
typedef struct definition definition;
typedef struct proposition_arg proposition_arg;
typedef struct relation relation;
typedef struct context context;

struct definition{
	char *name;
	sentence *sentence_data;
	unsigned int num_references;
	unsigned int num_args;
};

struct variable{
	char *name;
	variable_type type;
	sentence *sentence_data;
	unsigned int num_references;
};

struct proposition_arg{
	unsigned char is_bound;
	unsigned char replaced;
	union{
		int var_id;
		variable *var;
	};
};

struct relation{
	int relation_id;
	char *name;
	sentence *sentence_data;
};

struct sentence{
	statement_type type;
	union{
		//For boolean operations
		struct{
			sentence *child0;
			sentence *child1;
		};
		//For relations
		struct{
			relation *relation_data;
			unsigned char is_bound0;
			unsigned char replaced0;
			union{
				int var0_id;
				variable *var0;
			};
			unsigned char is_bound1;
			unsigned char replaced1;
			union{
				int var1_id;
				variable *var1;
			};
		};
		//For propositions
		struct{
			unsigned char is_bound;
			union{
				definition *definition_data;
				int definition_id;
			};
			proposition_arg *proposition_args;
			int num_args;
		};
	};

	sentence *parent;
	int num_bound_vars;
	int num_bound_props;
};

struct context{
	dictionary variables;
	dictionary definitions;
	dictionary relations;
	context *parent;
	unsigned int num_references;
};

