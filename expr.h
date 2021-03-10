#define MAX_DEPTH 32

extern unsigned int current_depth;
extern dictionary variables[MAX_DEPTH];
extern dictionary definitions[MAX_DEPTH];
extern dictionary bound_variables;
extern dictionary bound_propositions;

typedef enum{
	AND = 0,
	OR,
	IMPLIES,
	EXISTS,
	FORALL,
	NOT,
	MEMBERSHIP,
	PROPOSITION,
	FALSE,
	TRUE
} statement_type;

typedef enum{
	OBJECT,
	STATEMENT
} var_type;

typedef struct statement statement;
typedef struct variable variable;
typedef struct proposition proposition;
typedef struct bound_proposition bound_proposition;
typedef struct proposition_arg proposition_arg;

struct proposition{
	char *name;
	statement *statement_data;
	unsigned int num_references;
	unsigned int depth;
};

struct variable{
	char *name;
	var_type type;
	statement *statement_data;
	unsigned int num_references;
	unsigned int depth;
};

struct bound_proposition{
	int prop_id;
	int num_args;
};

struct proposition_arg{
	unsigned char is_bound;
	unsigned char replaced;
	union{
		int var_id;
		variable *var;
	};
};

struct statement{
	statement_type type;
	union{
		//For boolean operations
		struct{
			statement *child0;
			statement *child1;
		};
		//For membership
		struct{
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
		//For proposition
		struct{
			unsigned char is_bound;
			union{
				proposition *prop;
				int prop_id;
			};
			proposition_arg *prop_args;
			int num_args;
		};
	};
	
	int num_bound_vars;
	int num_bound_props;
};

int is_digit(char c);
int is_alpha(char c);
int is_alphanumeric(char c);
void skip_whitespace(char **c);
void get_identifier(char **c, char *buffer, size_t buffer_length);
statement *create_statement(statement_type type, int num_bound_vars, int num_bound_props);
statement *parse_statement(char **c, int num_bound_vars, int num_bound_props);
void free_statement(statement *s);
void copy_statement(statement *dest, statement *s);
unsigned char compare_statement(statement *a, statement *b);
void print_statement(statement *s);
