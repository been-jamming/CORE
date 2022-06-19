#ifndef PREDICATE_INCLUDED
#define PREDICATE_INCLUDED

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
#define ERROR_ARGUMENT_VERIFY 25
#define ERROR_TOO_MANY_UNPACK 26
#define ERROR_BEGIN_BRACE 27
#define ERROR_RETURN_EXPECTED 28
#define ERROR_END_BRACE 29
#define ERROR_OR_EXPECTED 30
#define ERROR_MISMATCHED_RETURN 31
#define ERROR_BEGIN_PARENTHESES 32
#define ERROR_OPERAND_TYPE 33
#define ERROR_OPERAND_BOUND_VARIABLES 34
#define ERROR_OPERAND_BOUND_PROPOSITIONS 35
#define ERROR_PIPE 36
#define ERROR_PIPE_OR_COMMA 37
#define ERROR_OPERAND_VERIFY 38
#define ERROR_EXPRESSION_OR_VARIABLE 39
#define ERROR_MISMATCHED_ARGUMENT 40
#define ERROR_EXPRESSION 41
#define ERROR_MORE_BOUND_PROPOSITIONS 42
#define ERROR_BRACKET_OR_COMMA 43
#define ERROR_COLON_OR_COMMA 44
#define ERROR_GREATER_THAN 45
#define ERROR_OPERATION 46
#define ERROR_PARENT_CONTEXT 47
#define ERROR_SEMICOLON 48
#define ERROR_DUPLICATE_IDENTIFIER 49
#define ERROR_COLON_OR_SEMICOLON 50
#define ERROR_NUM_ARGS 51
#define ERROR_COLON 52
#define ERROR_RELATION_IDENTIFIER 53
#define ERROR_DUPLICATE_RELATION 54
#define ERROR_RELATION_CONTEXT 55
#define ERROR_DUPLICATE_DEFINITION 56
#define ERROR_SEMICOLON_OR_COMMA 57
#define ERROR_BRACE_OR_EOF 58
#define ERROR_EOF 59
#define ERROR_RETURN_TYPE 60
#define ERROR_RETURN_VERIFIED 61
#define ERROR_GLOBAL_SCOPE 62
#define ERROR_GOAL_TYPE 63
#define ERROR_BRACE_OR_SEMICOLON 64
#define ERROR_BRACE_OR_SEMICOLON_OR_COMMA 65
#define ERROR_GOAL 66
#define ERROR_UNEXPECTED_RETURN 67
#define ERROR_ARGUMENT_TRUE 68
#define ERROR_FILE_READ 69
#define ERROR_IMPORT_VARIABLE 70
#define ERROR_IMPORT_RELATION 71
#define ERROR_IMPORT_DEFINITION 72
#define ERROR_AXIOM_INCOMPATIBLE 73
#define ERROR_OBJECT_INCOMPATIBLE 74
#define ERROR_DEFINITION_INCOMPATIBLE 75
#define ERROR_QUOTE 76
#define ERROR_PATH_SIZE 77
#define ERROR_RELATION_INCOMPATIBLE 78

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
	//Used to import definitions
	definition *destination;
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
	//Used for imports
	variable *destination;
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
	char *name;
	sentence *sentence_data;
	unsigned int num_references;
	//Used to import relations
	relation *destination;
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
	sentence *goal;
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
sentence *parse_sentence(char **c, int num_bound_vars, int num_bound_props);
void free_sentence(sentence *s);
void free_sentence_independent(sentence *s);
void decrement_references_sentence(sentence *s);
void print_sentence(sentence *s);
int sentence_stronger(sentence *s0, sentence *s1);
int sentence_equivalent(sentence *s0, sentence *s1);
int sentence_trivially_true(sentence *s);
int sentence_trivially_false(sentence *s);
void copy_sentence(sentence *dest, sentence *s);
sentence *peel_or_left(sentence **s);
sentence *peel_and_left(sentence **s);
int count_or(sentence *s);

expr_value *parse_context(char **c);
definition *create_definition(char *name, sentence *sentence_data, int num_args, context *parent_context);
relation *create_relation(char *name, sentence *sentence_data, context *parent_context);

#endif

