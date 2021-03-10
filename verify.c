#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "expr.h"

char *program_text;

statement *parse_statement_value(char **c, unsigned char verified);

statement *verify_block(char **c);

unsigned int umax(unsigned int a, unsigned int b){
	if(a > b){
		return a;
	}

	return b;
}

void free_proposition(proposition *p){
	free(p->name);
	if(p->statement_data){
		free_statement(p->statement_data);
	}
	free(p);
}

void free_variable(variable *v){
	free(v->name);
	if(v->type == STATEMENT && v->statement_data){
		free_statement(v->statement_data);
	}
	free(v);
}

void free_proposition_independent(proposition *p){
	free(p->name);
	if(p->statement_data){
		free_statement_independent(p->statement_data);
	}
	free(p);
}

void free_variable_independent(variable *v){
	free(v->name);
	if(v->type == STATEMENT && v->statement_data){
		free_statement_independent(v->statement_data);
	}
	free(v);
}

void free_proposition_void(void *p){
	free_proposition_independent(p);
}

void free_variable_void(void *v){
	free_variable_independent(v);
}

void proposition_decrement_references_void(void *p){
	proposition *prop;

	prop = p;
	if(prop->statement_data){
		decrement_references(prop->statement_data);
	}
}

void variable_decrement_references_void(void *p){
	variable *v;

	v = p;
	if(v->type == STATEMENT && v->statement_data){
		decrement_references(v->statement_data);
	}
}

void init_verifier(){
	int i;

	current_depth = 0;
	bound_variables = create_dictionary(NULL);
	bound_propositions = create_dictionary(NULL);

	for(i = 0; i < MAX_DEPTH; i++){
		variables[i] = create_dictionary(NULL);
		definitions[i] = create_dictionary(NULL);
	}
}

void up_scope(){
	if(current_depth < MAX_DEPTH - 1){
		current_depth++;
	} else {
		fprintf(stderr, "Error: maximum scope depth reached\n");
		exit(1);
	}
}

void drop_scope(){
	if(current_depth){
		iterate_dictionary(variables[current_depth], variable_decrement_references_void);
		iterate_dictionary(definitions[current_depth], proposition_decrement_references_void);
		free_dictionary(variables + current_depth, free_variable_void);
		free_dictionary(definitions + current_depth, free_proposition_void);
		current_depth--;
	} else {
		fprintf(stderr, "WARNING: drop scope called when scope was 0\n");
	}
}

char *load_file(char *file_name){
	FILE *fp;
	size_t size;
	char *output;

	fp = fopen(file_name, "r");
	if(!fp){
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	output = malloc(sizeof(char)*(size + 1));
	if(fread(output, sizeof(char), size, fp) < size){
		free(output);
		return NULL;
	}
	fclose(fp);
	output[size] = '\0';

	return output;
}

void clear_bound_variables(){
	free_dictionary(&bound_variables, free);
}

void clear_bound_propositions(){
	free_dictionary(&bound_propositions, free);
}

unsigned int max_statement_depth(statement *s){
	unsigned int a;
	unsigned int b;
	unsigned int output = 0;
	int i;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
			return umax(max_statement_depth(s->child0), max_statement_depth(s->child1));
		case NOT:
		case FORALL:
		case EXISTS:
			return max_statement_depth(s->child0);
		case MEMBERSHIP:
			if(s->is_bound0){
				a = 0;
			} else {
				a = s->var0->depth;
			}
			if(s->is_bound1){
				b = 0;
			} else {
				b = s->var1->depth;
			}
			return umax(a, b);
		case PROPOSITION:
			if(s->is_bound){
				output = s->prop->depth;
			}
			for(i = 0; i < s->num_args; i++){
				if(s->prop_args[i].is_bound){
					output = umax(output, s->prop_args[i].var->depth);
				}
			}
			return output;
		default:
			return 0;
	}
}

statement *parse_statement_identifier_proposition(char **c){
	proposition *prop;
	statement *output;
	char name_buffer[256];
	char *beginning;
	int i;

	skip_whitespace(c);
	beginning = *c;
	get_identifier(c, name_buffer, 256);
	if(name_buffer[0] == '\0'){
		*c = beginning;
		return NULL;
	}
	for(i = current_depth; i >= 0; i--){
		prop = read_dictionary(definitions[i], name_buffer, 0);
		if(prop){
			break;
		}
	}

	if(prop){
		output = malloc(sizeof(statement));
		output->type = PROPOSITION;
		output->is_bound = 0;
		output->prop = prop;
		prop->num_references++;
		output->num_args = prop->num_args;
		output->prop_args = malloc(sizeof(proposition_arg)*output->num_args);
		for(i = 0; i < output->num_args; i++){
			output->prop_args[i].is_bound = 1;
			output->prop_args[i].var_id = i;
		}
		output->num_bound_vars = output->num_args;
		output->num_bound_props = 0;
		return output;
	} else {
		*c = beginning;
		return NULL;
	}
}

statement *statement_to_definition(char *name){
	proposition *prop;
	statement *output;
	statement *new;
	int i;

	for(i = current_depth; i >= 0; i--){
		prop = read_dictionary(definitions[i], name, 0);
		if(prop && !prop->statement_data){
			prop = NULL;
		}
		if(prop){
			break;
		}
	}

	if(prop){
		output = create_statement(IMPLIES, prop->statement_data->num_bound_vars, 0);
		output->child0 = create_statement(PROPOSITION, prop->statement_data->num_bound_vars, 0);
		output->child0->is_bound = 0;
		output->child0->prop = prop;
		prop->num_references++;
		output->child0->num_args = prop->num_args;
		output->child0->prop_args = malloc(sizeof(proposition_arg)*output->child0->num_args);
		for(i = 0; i < output->child0->num_args; i++){
			output->child0->prop_args[i].is_bound = 1;
			output->child0->prop_args[i].var_id = i;
		}
		output->child1 = malloc(sizeof(statement));
		copy_statement(output->child1, prop->statement_data);
		for(i = prop->statement_data->num_bound_vars - 1; i >= 0; i--){
			new = create_statement(FORALL, i, 0);
			new->child0 = output;
			output = new;
		}

		return output;
	}

	return NULL;
}

statement *definition_to_statement(char *name){
	proposition *prop;
	statement *output;
	statement *new;
	int i;

	for(i = current_depth; i >= 0; i--){
		prop = read_dictionary(definitions[i], name, 0);
		if(prop && !prop->statement_data){
			prop = NULL;
		}
		if(prop){
			break;
		}
	}

	if(prop){
		output = create_statement(IMPLIES, prop->statement_data->num_bound_vars, 0);
		output->child1 = create_statement(PROPOSITION, prop->statement_data->num_bound_vars, 0);
		output->child1->is_bound = 0;
		output->child1->prop = prop;
		prop->num_references++;
		output->child1->num_args = prop->num_args;
		output->child1->prop_args = malloc(sizeof(proposition_arg)*output->child1->num_args);
		for(i = 0; i < output->child1->num_args; i++){
			output->child1->prop_args[i].is_bound = 1;
			output->child1->prop_args[i].var_id = i;
		}
		output->child0 = malloc(sizeof(statement));
		copy_statement(output->child0, prop->statement_data);
		for(i = prop->statement_data->num_bound_vars - 1; i >= 0; i--){
			new = create_statement(FORALL, i, 0);
			new->child0 = output;
			output = new;
		}

		return output;
	}

	return NULL;
}

statement *parse_statement_identifier(char **c, unsigned char verified){
	variable *var;
	statement *output;
	proposition *prop;
	char name_buffer[256];
	char *beginning;
	int i;

	skip_whitespace(c);
	beginning = *c;
	if(**c == '#'){
		++*c;
		get_identifier(c, name_buffer, 256);
		if(name_buffer[0]){
			output = definition_to_statement(name_buffer);
		}
		if(!name_buffer[0] || !output){
			*c = beginning;
			return NULL;
		}
		return output;
	}
	get_identifier(c, name_buffer, 256);
	if(name_buffer[0] == '\0'){
		*c = beginning;
		return NULL;
	}
	skip_whitespace(c);
	if(**c == '#'){
		++*c;
		output = statement_to_definition(name_buffer);
		if(!output){
			*c = beginning;
		}
		return output;
	}

	for(i = current_depth; i >= 0; i--){
		var = read_dictionary(variables[i], name_buffer, 0);
		if(var && var->type != STATEMENT){
			var = NULL;
		}
		if(var){
			break;
		}
	}

	if(!var && !verified){
		for(i = current_depth; i >= 0; i--){
			prop = read_dictionary(definitions[i], name_buffer, 0);
			if(prop && !prop->statement_data){
				prop = NULL;
			}
			if(prop){
				break;
			}
		}

		if(!prop){
			*c = beginning;
			return NULL;
		} else {
			output = malloc(sizeof(statement));
			copy_statement(output, prop->statement_data);
			return output;
		}
	}

	if(!var){
		*c = beginning;
		return NULL;
	} else {
		output = malloc(sizeof(statement));
		copy_statement(output, var->statement_data);
		return output;
	}
}

void add_bound_variables(statement *s, int increment){
	int i;

	s->num_bound_vars += increment;
	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
			add_bound_variables(s->child0, increment);
			add_bound_variables(s->child1, increment);
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			add_bound_variables(s->child0, increment);
			break;
		case MEMBERSHIP:
			if(s->is_bound0){
				s->var0_id += increment;
			}
			if(s->is_bound1){
				s->var1_id += increment;
			}
			break;
		case PROPOSITION:
			for(i = 0; i < s->num_args; i++){
				if(s->prop_args[i].is_bound){
					s->prop_args[i].var_id += increment;
				}
			}
			break;
		case FALSE:
		case TRUE:
			//pass
			break;
	}
}

void substitute_variable_recursive(statement *s, int id, variable *v){
	int i;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
			substitute_variable_recursive(s->child0, id, v);
			substitute_variable_recursive(s->child1, id, v);
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			substitute_variable_recursive(s->child0, id, v);
			break;
		case MEMBERSHIP:
			if(s->is_bound0 && s->var0_id == id){
				s->is_bound0 = 0;
				s->var0 = v;
				v->num_references++;
			}
			if(s->is_bound1 && s->var1_id == id){
				s->is_bound1 = 0;
				s->var1 = v;
				v->num_references++;
			}
			break;
		case PROPOSITION:
			for(i = 0; i < s->num_args; i++){
				if(s->prop_args[i].is_bound && s->prop_args[i].var_id == id){
					s->prop_args[i].is_bound = 0;
					s->prop_args[i].var = v;
					v->num_references++;
				}
			}
			break;
		case TRUE:
		case FALSE:
			//pass
			break;
	}
}

int substitute_variable(statement *s, int id, variable *v){
	if(s->num_bound_props || s->num_bound_vars <= id){
		return 0;
	}
	substitute_variable_recursive(s, id, v);

	return 1;
}

void reset_replaced(statement *s){
	int i;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
			reset_replaced(s->child0);
			reset_replaced(s->child1);
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			reset_replaced(s->child0);
			break;
		case MEMBERSHIP:
			s->replaced0 = 0;
			s->replaced1 = 0;
			break;
		case PROPOSITION:
			for(i = 0; i < s->num_args; i++){
				s->prop_args[i].replaced = 0;
			}
			break;
		case TRUE:
		case FALSE:
			//pass
			break;
	}
}

void substitute_variable_bound(statement *s, int id, int new_id){
	int i;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
			substitute_variable_bound(s->child0, id, new_id);
			substitute_variable_bound(s->child1, id, new_id);
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			substitute_variable_bound(s->child0, id, new_id);
			break;
		case PROPOSITION:
			for(i = 0; i < s->num_args; i++){
				if(s->prop_args[i].is_bound && s->prop_args[i].var_id == id && !s->prop_args[i].replaced){
					s->prop_args[i].var_id = new_id;
					s->prop_args[i].replaced = 1;
				}
			}
			break;
		case MEMBERSHIP:
			if(s->is_bound0 && s->var0_id == id && !s->replaced0){
				s->var0_id = new_id;
				s->replaced0 = 1;
			}
			if(s->is_bound1 && s->var1_id == id && !s->replaced1){
				s->var1_id = new_id;
				s->replaced1 = 1;
			}
			break;
		case TRUE:
		case FALSE:
			//pass
			break;
	}
}

void substitute_proposition(statement *s, statement *child){
	int i;
	statement new_child;

	s->num_bound_props--;
	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
			substitute_proposition(s->child0, child);
			substitute_proposition(s->child1, child);
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			substitute_proposition(s->child0, child);
			break;
		case PROPOSITION:
			if(s->is_bound && !s->prop_id){
				if(s->num_args != child->num_bound_vars){
					fprintf(stderr, "Error: argument count mismatch\n");
					exit(1);
				}
				copy_statement(&new_child, child);
				reset_replaced(&new_child);
				for(i = 0; i < s->num_args; i++){
					if(s->prop_args[i].is_bound){
						substitute_variable_bound(&new_child, i, s->prop_args[i].var_id + s->num_args - s->num_bound_vars);
					} else {
						substitute_variable_recursive(&new_child, i, s->prop_args[i].var);
						s->prop_args[i].var->num_references--;
					}
				}
				add_bound_variables(&new_child, s->num_bound_vars - s->num_args);
				new_child.num_bound_props = s->num_bound_props;
				free(s->prop_args);
				*s = new_child;
			} else if(s->is_bound){
				s->prop_id--;
			}
			break;
		case MEMBERSHIP:
		case TRUE:
		case FALSE:
			//pass
			break;
	}
			
}

variable *create_object_var(char *var_name){
	variable *v;

	v = read_dictionary(variables[current_depth], var_name, 0);
	if(v && !v->num_references){
		free_variable(v);
	} else if(v){
		fprintf(stderr, "Error: cannot overwrite bound variable\n");
		exit(1);
	}

	v = malloc(sizeof(variable));
	v->type = OBJECT;
	v->num_references = 0;
	v->name = malloc(sizeof(char)*(strlen(var_name) + 1));
	strcpy(v->name, var_name);
	v->depth = current_depth;

	write_dictionary(variables + current_depth, var_name, v, 0);

	return v;
}

variable *get_object_var(char *var_name){
	variable *v;
	int i;

	for(i = current_depth; i >= 0; i--){
		v = read_dictionary(variables[i], var_name, 0);
		if(v && v->type != OBJECT){
			v = NULL;
		}
		if(v){
			break;
		}
	}

	return v;
}

variable *get_statement_var(char *var_name){
	variable *v;
	int i;

	for(i = current_depth; i >= 0; i--){
		v = read_dictionary(variables[i], var_name, 0);
		if(v && v->type != STATEMENT){
			v = NULL;
		}
		if(v){
			break;
		}
	}

	return v;
}

variable *create_statement_var(char *var_name, statement *s){
	variable *v;

	v = read_dictionary(variables[current_depth], var_name, 0);
	if(v && !v->num_references){
		free_variable(v);
	} else if(v){
		free_statement(s);
		fprintf(stderr, "Error: cannot overwrite bound variable\n");
		exit(1);
	}

	v = malloc(sizeof(variable));
	v->type = STATEMENT;
	v->num_references = 0;
	v->name = malloc(sizeof(char)*(strlen(var_name) + 1));
	strcpy(v->name, var_name);
	v->statement_data = s;
	v->depth = current_depth;

	write_dictionary(variables + current_depth, var_name, v, 0);

	return v;
}

statement *parse_statement_value_builtin(char **c, unsigned char verified){
	char var_name[256];
	statement *s;
	statement *output;
	statement *child0;
	statement *child1;
	variable *new_statement_var;
	statement *new;
	statement *return_statement0;
	statement *return_statement1;

	if(!strncmp(*c, "left", 4) && !is_alphanumeric((*c)[4])){
		*c += 4;
		skip_whitespace(c);
		if(**c != '('){
			return NULL;
		}
		++*c;
		s = parse_statement_value(c, verified);
		skip_whitespace(c);
		if(**c != ')'){
			free_statement(s);
			fprintf(stderr, "Error: expected ')'\n");
			exit(1);
		}
		++*c;
		if(verified && (s->type == OR || s->type == IMPLIES)){
			free_statement(s);
			fprintf(stderr, "Error: resulting statement must be verified\n");
			exit(1);
		} else if(s->type != AND){
			free_statement(s);
			fprintf(stderr, "Error: invalid operand for 'left'\n");
			exit(1);
		}

		free_statement(s->child1);
		output = s->child0;
		free(s);

		return output;
	} else if(!strncmp(*c, "right", 5) && !is_alphanumeric((*c)[5])){
		*c += 5;
		skip_whitespace(c);
		if(**c != '('){
			return NULL;
		}
		++*c;
		s = parse_statement_value(c, verified);
		if(!s){
			fprintf(stderr, "Error: could not parse statement value\n");
			exit(1);
		}
		skip_whitespace(c);
		if(**c != ')'){
			free_statement(s);
			fprintf(stderr, "Error: expected ')'\n");
			exit(1);
		}
		++*c;
		if(verified && (s->type == OR || s->type == IMPLIES)){
			free_statement(s);
			fprintf(stderr, "Error: resulting statement must be verified\n");
			exit(1);
		} else if(s->type != AND && s->type != OR && s->type != IMPLIES){
			fprintf(stderr, "Error: invalid operand for 'right' %d\n", s->type);
			free_statement(s);
			exit(1);
		}

		free_statement(s->child0);
		output = s->child1;
		free(s);

		return output;
	} else if(!strncmp(*c, "swap", 4) && !is_alphanumeric((*c)[4])){
		*c += 4;
		skip_whitespace(c);
		if(**c != '('){
			return NULL;
		}
		++*c;
		s = parse_statement_value(c, verified);
		skip_whitespace(c);
		if(**c != ')'){
			free_statement(s);
			fprintf(stderr, "Error: expected ')'\n");
			exit(1);
		}
		++*c;
		if(s->type != AND && s->type != OR && s->type != IMPLIES){
			free_statement(s);
			fprintf(stderr, "Error: invalid operand for 'swap'\n");
			exit(1);
		}
		if(s->type == AND || s->type == OR){
			output = s->child0;
			s->child0 = s->child1;
			s->child1 = output;
			output = s;

			return s;
		} else if(s->type == IMPLIES){
			child0 = s->child0;
			child1 = s->child1;

			new = create_statement(NOT, s->num_bound_vars, s->num_bound_props);
			new->child0 = child0;
			s->child1 = new;

			new = create_statement(NOT, s->num_bound_vars, s->num_bound_props);
			new->child0 = child1;
			s->child0 = new;

			return s;
		}
	} else if(!strncmp(*c, "branch", 6) && !is_alphanumeric((*c)[6])){
		*c += 6;
		skip_whitespace(c);
		if(**c != '('){
			return NULL;
		}
		++*c;
		skip_whitespace(c);
		s = parse_statement_value(c, 1);
		skip_whitespace(c);
		if(!s){
			fprintf(stderr, "Error: could not parse statement value\n");
			exit(1);
		}
		if(**c != ','){
			fprintf(stderr, "Error: expected ','\n");
			exit(1);
		}
		++*c;
		if(s->type != OR || s->num_bound_props || s->num_bound_vars){
			fprintf(stderr, "Error: invalid operand for 'branch'\n");
			exit(1);
		}
		++*c;
		skip_whitespace(c);
		get_identifier(c, var_name, 256);
		if(var_name[0] == '\0'){
			fprintf(stderr, "Error: expected identifier\n");
			exit(1);
		}
		skip_whitespace(c);
		if(**c != ')'){
			fprintf(stderr, "Error: expected ')'\n");
			exit(1);
		}
		++*c;
		skip_whitespace(c);
		if(**c != '{'){
			fprintf(stderr, "Error: expected '{'\n");
			exit(1);
		}
		++*c;
		skip_whitespace(c);
		up_scope();
		new = malloc(sizeof(statement));
		copy_statement(new, s->child0);
		new_statement_var = create_statement_var(var_name, new);
		new_statement_var->num_references++;
		return_statement0 = verify_block(c);
		if(!return_statement0){
			fprintf(stderr, "Error: expected return statement\n");
			exit(1);
		}
		if(max_statement_depth(return_statement0) >= current_depth){
			free_statement(return_statement0);
			fprintf(stderr, "Error: returned statement depends on variables in its scope\n");
			exit(1);
		}
		drop_scope();
		skip_whitespace(c);
		if(strncmp(*c, "or", 2) || is_alphanumeric((*c)[2])){
			free_statement(return_statement0);
			fprintf(stderr, "Error: expected 'or'\n");
			exit(1);
		}
		*c += 2;
		skip_whitespace(c);
		if(**c != '{'){
			free_statement(return_statement0);
			fprintf(stderr, "Error: expected '{'\n");
			exit(1);
		}
		++*c;
		skip_whitespace(c);
		up_scope();
		new = malloc(sizeof(statement));
		copy_statement(new, s->child1);
		new_statement_var = create_statement_var(var_name, new);
		new_statement_var->num_references++;
		return_statement1 = verify_block(c);
		if(!return_statement1){
			free_statement(return_statement0);
			fprintf(stderr, "Error: expected return statement\n");
			exit(1);
		}
		if(max_statement_depth(return_statement1) >= current_depth){
			free_statement(return_statement0);
			free_statement(return_statement1);
			fprintf(stderr, "Error: returned statement depends on variables in its scope\n");
			exit(1);
		}
		drop_scope();
		if(!compare_statement(return_statement0, return_statement1)){
			free_statement(return_statement0);
			free_statement(return_statement1);
			fprintf(stderr, "Error: mismatched returned statements\n");
			exit(1);
		}
		free_statement(return_statement1);
		free_statement(s);

		return return_statement0;
	}

	return NULL;
}

statement *parse_statement_value(char **c, unsigned char verified){
	statement *output;
	statement *next_output;
	statement *arg;
	variable *var;
	char var_name[256];
	char *beginning;

	skip_whitespace(c);
	if((output = parse_statement_value_builtin(c, verified))){
		//pass
	} else if(**c == '('){
		++*c;
		output = parse_statement_value(c, verified);
		if(**c != ')'){
			if(output){
				free_statement(output);
			}
			return NULL;
		}
		++*c;
	} else {
		output = parse_statement_identifier(c, verified);
		if(!output){
			return NULL;
		}
	}
	skip_whitespace(c);
	while(**c && **c != ';' && **c != ')' && **c != ',' && **c != ']'){
		if(**c == '['){
			++*c;
			skip_whitespace(c);
			beginning = *c;
			arg = parse_statement_identifier_proposition(c);
			skip_whitespace(c);
			if(!arg || **c != ']'){
				if(arg){
					free_statement(arg);
				}
				*c = beginning;
				arg = parse_statement_value(c, 0);
			}
			if(!arg || arg->num_bound_props){
				free_statement(output);
				return NULL;
			}
			skip_whitespace(c);
			if(**c != ']'){
				free_statement(arg);
				free_statement(output);
				return NULL;
			}
			++*c;
			substitute_proposition(output, arg);
			free_statement(arg);
		} else if(**c == '('){
			++*c;
			skip_whitespace(c);
			if(output->type == EXISTS || output->type == FORALL){
				get_identifier(c, var_name, 256);
				skip_whitespace(c);
				if(**c != ')'){
					free_statement(output);
					return NULL;
				}
				++*c;
				if(output->type == EXISTS){
					if(!verified || output->num_bound_props){
						free_statement(output);
						return NULL;
					}
					var = create_object_var(var_name);
				} else if(output->type == FORALL){
					if(output->num_bound_props){
						free_statement(output);
						return NULL;
					}
					var = get_object_var(var_name);
				} else {
					free_statement(output);
					return NULL;
				}
				if(!var){
					free_statement(output);
					return NULL;
				}
				next_output = output->child0;
				free(output);
				if(!substitute_variable(next_output, 0, var)){
					free_statement(next_output);
					return NULL;
				}
				output = next_output;
				add_bound_variables(output, -1);
			} else if(output->type == IMPLIES || output->type == NOT){
				arg = parse_statement_value(c, verified);
				skip_whitespace(c);
				if(**c != ')' || !arg || arg->num_bound_vars || arg->num_bound_props || !compare_statement(arg, output->child0)){
					if(arg){
						free_statement(arg);
					}
					free_statement(output);
					return NULL;
				}
				++*c;
				free_statement(arg);
				if(output->type == IMPLIES){
					free_statement(output->child0);
					next_output = output->child1;
					free(output);
					output = next_output;
				} else if(output->type == NOT){
					free_statement(output);
					output = create_statement(FALSE, 0, 0);
				}
					
			} else {
				free_statement(output);
				return NULL;
			}
		} else {
			free_statement(output);
			return NULL;
		}
		skip_whitespace(c);
	}

	return output;
}

int print_command(char **c){
	statement *s;

	if(strncmp(*c, "print", 5) || is_alphanumeric((*c)[5])){
		return 0;
	}
	*c += 5;
	skip_whitespace(c);
	s = parse_statement_value(c, 1);
	if(!s){
		fprintf(stderr, "Error: could not parse statement value\n");
		exit(1);
	}
	print_statement(s);
	printf("\n");
	free_statement(s);
	skip_whitespace(c);
	if(**c != ';'){
		fprintf(stderr, "Error: expected ';'\n");
		exit(1);
	}
	++*c;

	return 1;
}

proposition *definition_command(char **c){
	int num_bound_vars = 0;
	int *new_int;
	char name_buffer[256];
	char *def_name;
	statement *s;
	proposition *output;

	clear_bound_variables();
	skip_whitespace(c);
	if(strncmp(*c, "define", 6) || is_alphanumeric((*c)[6])){
		return NULL;
	}

	*c += 6;
	skip_whitespace(c);
	
	get_identifier(c, name_buffer, 256);
	if(name_buffer[0] == '\0'){
		fprintf(stderr, "Error: expected identifier\n");
		exit(1);
	}

	def_name = malloc(sizeof(char)*(strlen(name_buffer) + 1));
	strcpy(def_name, name_buffer);

	skip_whitespace(c);

	if(**c == '{'){
		++*c;
		skip_whitespace(c);
		while(**c != '}'){
			get_identifier(c, name_buffer, 256);
			if(name_buffer[0] == '\0'){
				free(def_name);
				fprintf(stderr, "Error: expected identifier\n");
				exit(1);
			}
			if(read_dictionary(bound_variables, name_buffer, 0)){
				fprintf(stderr, "Error: duplicate identifier\n");
				exit(1);
			}
			new_int = malloc(sizeof(int));
			*new_int = num_bound_vars;
			write_dictionary(&bound_variables, name_buffer, new_int, 0);
			num_bound_vars++;
			skip_whitespace(c);
			if(**c == ','){
				++*c;
				skip_whitespace(c);
			} else if(**c != '}'){
				free(def_name);
				fprintf(stderr, "Error: expected ',' or '}'\n");
				exit(1);
			}
		}
		++*c;
	}
	skip_whitespace(c);
	if(**c != ':'){
		free(def_name);
		fprintf(stderr, "Error: expected ':'\n");
		exit(1);
	}
	++*c;
	s = parse_statement(c, num_bound_vars, 0);
	if(**c != ';'){
		free(def_name);
		free_statement(s);
		fprintf(stderr, "Error: expected ';'\n");
		exit(1);
	}
	++*c;
	output = malloc(sizeof(proposition));
	output->name = def_name;
	output->statement_data = s;
	output->num_references = 0;
	output->depth = current_depth;
	output->num_args = num_bound_vars;
	printf("definition %s: ", def_name);
	print_statement(s);
	printf("\n");

	return output;
}

variable *axiom_command(char **c){
	int num_bound_props = 0;
	int num_args;
	bound_proposition *new_bound_prop;
	char name_buffer[256];
	char *axiom_name;
	char *int_end;
	variable *output;
	statement *s;

	clear_bound_propositions();
	skip_whitespace(c);
	if(strncmp(*c, "axiom", 5) || is_alphanumeric((*c)[5])){
		return NULL;
	}

	*c += 5;
	skip_whitespace(c);

	get_identifier(c, name_buffer, 256);
	if(name_buffer[0] == '\0'){
		fprintf(stderr, "Error: expected identifier\n");
		exit(1);
	}

	axiom_name = malloc(sizeof(char)*(strlen(name_buffer) + 1));
	strcpy(axiom_name, name_buffer);

	skip_whitespace(c);

	if(**c == '['){
		++*c;
		skip_whitespace(c);
		while(**c != ']'){
			get_identifier(c, name_buffer, 256);
			if(name_buffer[0] == '\0'){
				free(axiom_name);
				fprintf(stderr, "Error: expected identifier\n");
				exit(1);
			}
			if(read_dictionary(bound_propositions, name_buffer, 0)){
				free(axiom_name);
				fprintf(stderr, "Error: duplicate identifier\n");
				exit(1);
			}
			new_bound_prop = malloc(sizeof(bound_proposition));
			skip_whitespace(c);
			if(**c == '('){
				++*c;
				skip_whitespace(c);
				if(**c != ')' && !is_digit(**c)){
					free(axiom_name);
					free(new_bound_prop);
					fprintf(stderr, "Error: expected number of arguments\n");
					exit(1);
				} else if(is_digit(**c)){
					num_args = strtol(*c, &int_end, 10);
					*c = int_end;
					if(num_args < 0){
						free(axiom_name);
						free(new_bound_prop);
						fprintf(stderr, "Error: expected a positive number of arguments\n");
						exit(1);
					}
					skip_whitespace(c);
					if(**c != ')'){
						free(axiom_name);
						free(new_bound_prop);
						fprintf(stderr, "Error: expected ')'\n");
						exit(1);
					}
					++*c;
				} else {
					++*c;
					num_args = 0;
				}
				new_bound_prop->num_args = num_args;
			} else {
				new_bound_prop->num_args = 0;
			}
			new_bound_prop->prop_id = num_bound_props;
			write_dictionary(&bound_propositions, name_buffer, new_bound_prop, 0);
			num_bound_props++;
			skip_whitespace(c);
			if(**c == ','){
				++*c;
				skip_whitespace(c);
			} else if(**c != ']'){
				free(axiom_name);
				fprintf(stderr, "Error: expected ',' or ']'\n");
				exit(1);
			}
		}
		++*c;
	}
	skip_whitespace(c);
	if(**c != ':'){
		free(axiom_name);
		fprintf(stderr, "Error: expected ':'\n");
		exit(1);
	}
	++*c;
	s = parse_statement(c, 0, num_bound_props);
	if(**c != ';'){
		free(axiom_name);
		free_statement(s);
		fprintf(stderr, "Error: expected ';'\n");
		exit(1);
	}
	++*c;
	printf("axiom %s: ", axiom_name);
	print_statement(s);
	printf("\n");

	output = malloc(sizeof(variable));
	output->name = axiom_name;
	output->type = STATEMENT;
	output->statement_data = s;
	output->num_references = 0;
	output->depth = current_depth;

	return output;
}

variable *assign_command(char **c){
	char name_buffer[256];
	char *beginning;
	statement *s;
	variable *output;

	skip_whitespace(c);
	beginning = *c;
	get_identifier(c, name_buffer, 256);
	if(name_buffer[0] == '\0'){
		*c = beginning;
		return NULL;
	}

	skip_whitespace(c);
	if(**c != '='){
		*c = beginning;
		return NULL;
	}

	++*c;
	skip_whitespace(c);
	
	s = parse_statement_value(c, 1);
	if(!s){
		fprintf(stderr, "Error: could not parse statement value\n");
		exit(1);
	}
	skip_whitespace(c);
	if(**c != ';'){
		free_statement(s);
		fprintf(stderr, "Error: expected ';'\n");
		exit(1);
	}
	++*c;

	output = create_statement_var(name_buffer, s);
	
	return output;
}

statement *return_command(char **c){
	statement *output;

	skip_whitespace(c);
	if(strncmp(*c, "return", 6) || is_alphanumeric((*c)[6])){
		return NULL;
	}

	*c += 6;
	skip_whitespace(c);
	output = parse_statement_value(c, 1);
	if(!output){
		fprintf(stderr, "Error: could not parse statement value\n");
		exit(1);
	}
	skip_whitespace(c);
	if(**c != ';'){
		free_statement(output);
		fprintf(stderr, "Error: expected ';'\n");
		exit(1);
	}
	++*c;

	return output;
}

statement *verify_command(char **c){
	proposition *def;
	proposition *other_def;
	variable *axiom;
	variable *other_axiom;
	variable *var;
	statement *return_value;

	if((def = definition_command(c))){
		if((other_def = read_dictionary(definitions[current_depth], def->name, 0))){
			if(other_def->num_references){
				fprintf(stderr, "Error: cannot overwrite bound definition\n");
				exit(1);
			} else {
				free_proposition(other_def);
			}
		}
		write_dictionary(definitions + current_depth, def->name, def, 0);
	} else if((axiom = axiom_command(c))){
		if((other_axiom = read_dictionary(variables[current_depth], axiom->name, 0))){
			if(other_axiom->num_references){
				fprintf(stderr, "Error: cannot overwrite bound axiom\n");
				exit(1);
			} else {
				free_variable(other_axiom);
			}
		}
		write_dictionary(variables + current_depth, axiom->name, axiom, 0);
	} else if(print_command(c)){
		//Pass
	} else if((var = assign_command(c))){
		//Pass
	} else if((return_value = return_command(c))){
		return return_value;
	} else {
		fprintf(stderr, "Error: unknown command '%s'\n", *c);
		exit(1);
	}

	return NULL;
}

statement *verify_block(char **c){
	statement *return_value = NULL;

	skip_whitespace(c);
	while(**c && **c != '}' && !return_value){
		return_value = verify_command(c);
		skip_whitespace(c);
	}

	if(return_value && **c && **c != '}'){
		free_statement(return_value);
		fprintf(stderr, "Error: expected '}' or EOF after 'return'\n");
		exit(1);
	}

	if(**c){
		++*c;
	}

	return return_value;
}

int main(int argc, char **argv){
	char *program_start;
	statement *return_value;

	init_verifier();
	program_text = load_file(argv[1]);
	program_start = program_text;
	if(!program_text){
		fprintf(stderr, "Error: could not read file\n");
		return 1;
	}

	return_value = verify_block(&program_text);
	if(return_value){
		printf("\nProgram returned: ");
		print_statement(return_value);
		printf("\n");
	}

	free(program_start);
	free_dictionary(variables, free_variable_void);
	free_dictionary(definitions, free_proposition_void);
	return 0;
}
