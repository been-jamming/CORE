extern char *program_text;
extern statement *goals[MAX_DEPTH];
extern unsigned int goal_depth;

statement *parse_statement_identifier_or_value(char **c, unsigned char *is_verified, char end_char0, char end_char1);
statement *parse_statement_value(char **c, unsigned char *is_verified);

statement *verify_block(char **c, unsigned char allow_proof_value, statement *goal);

unsigned int umax(unsigned int a, unsigned int b);
void free_proposition(proposition *p);
void free_variable(variable *v);
void free_proposition_independent(proposition *p);
void free_variable_independent(variable *v);
void free_proposition_void(void *p);
void free_variable_void(void *v);
void proposition_decrement_references_void(void *p);
void variable_decrement_references_void(void *p);
void init_verifier();
void up_scope();
void drop_scope();
unsigned int max_statement_depth(statement *s);
statement *parse_statement_identifier_proposition(char **c);
statement *statement_to_definition(char *name);
statement *definition_to_statement(char *name);
statement *parse_statement_identifier(char **c, unsigned char *is_verified);
void add_bound_variables(statement *s, int increment);
void substitute_variable_recursive(statement *s, int id, variable *v);
int substitute_variable(statement *s, int id, variable *v);
void reset_replaced(statement *s);
void set_num_bound_props(statement *s, int num_bound_props);
void substitute_variable_bound(statement *s, int id, int new_id);
void substitute_proposition(statement *s, statement *child);
variable *create_object_var(char *var_name, unsigned int depth);
variable *get_object_var(char *var_name, unsigned int *depth);
variable *get_statement_var(char *var_name);
variable *create_statement_var(char *var_name, statement *s);
statement *parse_statement_value_builtin(char **c, unsigned char *is_verified);
