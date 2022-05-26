extern sentence *goals[MAX_DEPTH];
extern unsigned int goal_depth;

void clear_bound_variables(void);
expr_value *create_expr_value(expr_value_type type);
void free_expr_value(expr_value *val);
void free_definition_independent(definition *def);
void free_definition_void(void *v);
void free_definition(definition *def);
void decrement_definition(definition *def);
void decrement_definition_void(void *v);
void free_relation_independent(relation *rel);
void free_relation_void(void *v);
void free_relation(relation *rel);
void decrement_relation(relation *rel);
void decrement_relation_void(void *v);
void free_variable_independent(variable *var);
void free_variable_void(void *v);
void free_variable(variable *var);
void decrement_variable(variable *var);
void decrement_variable_void(void *v);
void free_context_independent(context *c);
void decrement_context(context *c);
void free_context(context *c);
expr_value *definition_to_value(char *name);
expr_value *relation_to_value(char *name);
expr_value *parse_expr_identifier(char **c);
void add_bound_variables(sentence *s, int increment);
void substitute_variable_recursive(sentence *s, int id, variable *v);
void substitute_variable(sentence *s, variable *v);
void reset_replaced(sentence *s);
void set_num_bound_props(sentence *s, int num_props);
void substitute_variable_bound(sentence *s, int id, int new_id);
void substitute_proposition(sentence *s, sentence *child);
variable *create_object_variable(char *var_name, context *parent_context);
variable *create_sentence_variable(char *var_name, sentence *sentence_data, unsigned char verified, context *parent_context);
variable *create_context_variable(char *var_name, context *context_data, context *parent_context);
variable *get_variable(char *var_name, context *parent_context);

expr_value *parse_left(char **c);
expr_value *parse_right(char **c);
expr_value *parse_and_or(char **c, unsigned char is_and);
expr_value *parse_expand(char **c);
expr_value *parse_iff(char **c);
expr_value *parse_branch(char **c);
expr_value *parse_expr_value_builtin(char **c);
expr_value *parse_expr_value_pipe(char **c, expr_value *input);
expr_value *parse_expr_value_parentheses(char **c, expr_value *input);
expr_value *parse_expr_value_brackets(char **c, expr_value *input);
expr_value *parse_anonymous_definition(char **c);

expr_value *parse_expr_value(char **c);
expr_value *parse_context(char **c);
