#define MAX_DEPTH 128

#define ERROR_IDENTIFIER_LENGTH 1
#define ERROR_IDENTIFIER_EXPECTED 2
#define ERROR_ARGUMENT_COUNT 3
#define ERROR_END_PARENTHESES 4
#define ERROR_SENTENCE_VALUE 5
#define ERROR_SENTENCE_OPERATION 6
#define ERROR_DEFINITION_EXISTS 7
#define ERROR_DEFINITION_EMPTY 8
#define ERROR_RELATION_EXISTS 9
#define ERROR_RELATION_EMPTY 10
#define ERROR_BRACKET_EXPECTED 11
#define ERROR_DOT_EXPECTED 12
#define ERROR_CONTEXT_MEMBER 13
#define ERROR_CANNOT_DOT 14
#define ERROR_NO_BOUND_VARIABLES 15
#define ERROR_VARIABLE_OVERWRITE 16
#define ERROR_VARIABLE_NOT_FOUND 17
#define ERROR_ARGUMENT_TYPE 18
#define ERROR_ARGUMENT_BOUND_VARIABLES 19
#define ERROR_ARGUMENT_BOUND_PROPOSITIONS 20
#define ERROR_PARENTHESES_OR_COMMA 21
#define ERROR_COMMA 22
#define ERROR_PARENTHESES 23
#define ERROR_MISMATCHED_IMPLICATIONS 24

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
	SENTENCE,
	OBJECT
} expr_value_type;

typedef enum{
	SENTENCE_VAR,
	OBJECT_VAR,
	CONTEXT_VAR
} var_type;

typedef struct sentence sentence;
typedef struct variable variable;
typedef struct definition definition;
typedef struct bound_proposition bound_proposition;
typedef struct proposition_arg proposition_arg;
typedef struct relation relation;
typedef struct context context;
typedef struct expr_value expr_value;

extern context *global_context;

struct definition{
	char *name;
	sentence *sentence_data;
	unsigned int num_references;
	unsigned int num_args;
};

struct expr_value{
	expr_value_type type;
	union{
		struct{
			sentence *sentence_data;
			unsigned char verified;
		};
		variable *var;
	};
};

struct variable{
	char *name;
	var_type type;
	union{
		struct{
			sentence *sentence_data;
			unsigned char verified;
		};
		context *context_data;
	};
	unsigned int num_references;
	context *parent_context;
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

struct relation{
	int relation_id;
	char *name;
	sentence *sentence_data;
};

struct sentence{
	sentence_type type;
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
};

sentence *create_sentence(sentence_type type, int num_bound_vars, int num_bound_props);
unsigned char is_whitespace(char *c);
void skip_whitespace(char **c);
void error(int error_code);
int is_digit(char c);
int is_alpha(char c);
int is_alphanumeric(char c);
int get_identifier(char **c, char *buffer, size_t buffer_length);
int get_relation_identifier(char **c, char *buffer, size_t buffer_length);
sentence *parse_true_false(char **c, int num_bound_vars, int num_bound_props);
sentence *parse_relation(char **c, int num_bound_vars, int num_bound_props);
sentence *parse_proposition(char **c, int num_bound_vars, int num_bound_props);
sentence *parse_not(char **c, int num_bound_vars, int num_bound_props);
sentence *parse_all(char **c, int num_bound_vars, int num_bound_props);
sentence *parse_exists(char **c, int num_bound_vars, int num_bound_props);
sentence *parse_parentheses(char **c, int num_bound_vars, int num_bound_props);
sentence *parse_sentence_value(char **c, int num_bound_vars, int num_bound_props);
int get_operation(char **c);
static sentence *parse_sentence_recursive(int priority, sentence *s0, char **c, int num_bound_vars, int num_bound_props);
sentence *parse_sentence(char **c, int num_bound_vars, int num_bound_props);
void free_sentence(sentence *s);
void free_sentence_independent(sentence *s);
void decrement_references_sentence(sentence *s);
void print_sentence(sentence *s);
int sentence_stronger(sentence *s0, sentence *s1);
int sentence_equivalent(sentence *s0, sentence *s1);
void copy_sentence(sentence *dest, sentence *s);
sentence *peel_or_left(sentence **s);

