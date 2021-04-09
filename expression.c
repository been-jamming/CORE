#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "proposition.h"
#include "expression.h"

char *program_text;
statement *goals[MAX_DEPTH];
unsigned int goal_depth;

static statement *parse_statement_value_recursive(char **c, unsigned char *is_verified, unsigned char *did_bind);

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
			if(!s->is_bound){
				output = s->prop->depth;
			}
			for(i = 0; i < s->num_args; i++){
				if(!s->prop_args[i].is_bound){
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

statement *parse_statement_identifier(char **c, unsigned char *is_verified){
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

	if(!var){
		*is_verified = 0;
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

void set_num_bound_props(statement *s, int num_bound_props){
	s->num_bound_props = num_bound_props;

	switch(s->type){
		case AND:
		case OR:
		case IMPLIES:
			set_num_bound_props(s->child0, num_bound_props);
			set_num_bound_props(s->child1, num_bound_props);
			break;
		case NOT:
		case FORALL:
		case EXISTS:
			set_num_bound_props(s->child0, num_bound_props);
			break;
		default:
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
					error(1);
				}
				copy_statement(&new_child, child);
				reset_replaced(&new_child);
				set_num_bound_props(&new_child, s->num_bound_props);
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
		error(1);
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
		error(1);
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

statement *parse_left(char **c, unsigned char *is_verified){
	statement *s;
	statement *output;
	unsigned char arg_verified;

	s = parse_statement_value(c, &arg_verified);
	*is_verified = arg_verified && *is_verified;
	if(!s){
		fprintf(stderr, "Error: could not parse statement value\n");
		error(1);
	}
	skip_whitespace(c);
	if(**c != ')'){
		free_statement(s);
		fprintf(stderr, "Error: expected ')'\n");
		error(1);
	}
	++*c;
	if(s->type == OR || s->type == IMPLIES){
		*is_verified = 0;
	} else if(s->type != AND){
		free_statement(s);
		fprintf(stderr, "Error: invalid operand for 'left'\n");
		error(1);
	} else if(s->num_bound_vars || s->num_bound_props){
		free_statement(s);
		fprintf(stderr, "Error: expected operand to have no bound variables or propositions\n");
		error(1);
	}

	free_statement(s->child1);
	output = s->child0;
	free(s);

	return output;
}

statement *parse_right(char **c, unsigned char *is_verified){
	statement *s;
	statement *output;
	unsigned char arg_verified;

	s = parse_statement_value(c, &arg_verified);
	*is_verified = arg_verified && *is_verified;
	if(!s){
		fprintf(stderr, "Error: could not parse statement value\n");
		error(1);
	}
	skip_whitespace(c);
	if(**c != ')'){
		free_statement(s);
		fprintf(stderr, "Error: expected ')'\n");
		error(1);
	}
	++*c;
	if(s->type == OR || s->type == IMPLIES){
		*is_verified = 0;
	} else if(s->type != AND){
		free_statement(s);
		fprintf(stderr, "Error: invalid operand for 'right'\n");
		error(1);
	} else if(s->num_bound_vars || s->num_bound_props){
		free_statement(s);
		fprintf(stderr, "Error: expected operand to have no bound variables or propositions\n");
		error(1);
	}

	free_statement(s->child0);
	output = s->child1;
	free(s);

	return output;
}

statement *parse_and_or(char **c, unsigned char is_and, unsigned char *is_verified){
	statement *output;
	statement *last_parent = NULL;
	statement *s;
	statement *new;
	unsigned char args_verified;
	unsigned char arg_verified = 1;

	output = parse_statement_identifier_or_value(c, &arg_verified, ',', ')');
	if(!output){
		fprintf(stderr, "Error: could not parse statement value\n");
		error(1);
	}
	if(output->num_bound_props || output->num_bound_vars){
		free_statement(output);
		fprintf(stderr, "Error: expected operand to have no bound variables or propositions\n");
		error(1);
	}
	args_verified = arg_verified;

	while(**c != ')'){
		++*c;
		skip_whitespace(c);
		arg_verified = 1;
		s = parse_statement_identifier_or_value(c, &arg_verified, ',', ')');
		if(!s){
			free_statement(output);
			fprintf(stderr, "Error: could not parse statement value\n");
			error(1);
		}
		if(s->num_bound_props || s->num_bound_vars){
			free_statement(s);
			free_statement(output);
			fprintf(stderr, "Error: expected operand to have no bound variables or propositions\n");
			error(1);
		}

		if(is_and){
			args_verified = arg_verified && args_verified;
			new = create_statement(AND, 0, 0);
		} else {
			args_verified = arg_verified || args_verified;
			new = create_statement(OR, 0, 0);
		}
		if(!last_parent){
			last_parent = new;
			last_parent->child0 = output;
			last_parent->child1 = s;
			output = last_parent;
		} else {
			new->child0 = last_parent->child1;
			new->child1 = s;
			last_parent->child1 = new;
			last_parent = new;
		}

		skip_whitespace(c);
		if(**c != ')' && **c != ','){
			free_statement(output);
			fprintf(stderr, "Error: expected ',' or ')'\n");
			error(1);
		}
	}

	++*c;
	*is_verified = args_verified && *is_verified;
	return output;
}

statement *parse_swap(char **c, unsigned char *is_verified){
	statement *s;
	statement *output;
	statement *new;
	statement *child0;
	statement *child1;
	unsigned char arg_verified;

	s = parse_statement_value(c, &arg_verified);
	*is_verified = arg_verified && *is_verified;
	if(!s){
		fprintf(stderr, "Error: could not parse statement value\n");
		error(1);
	}
	skip_whitespace(c);
	if(**c != ')'){
		free_statement(s);
		fprintf(stderr, "Error: expected ')'\n");
		error(1);
	}
	++*c;

	if(s->num_bound_vars || s->num_bound_props){
		free_statement(s);
		fprintf(stderr, "Error: expected operand to have no bound variables or propositions\n");
		error(1);
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
	} else {
		free_statement(s);
		fprintf(stderr, "Error: invalid operand for 'swap'\n");
		error(1);
		return NULL;
	}
}

static void skip_branch_args(char **c){
	skip_whitespace(c);
	while(is_alphanumeric(**c) || **c == ','){
		++*c;
		skip_whitespace(c);
	}
}

statement *parse_branch(char **c){
	statement *or_statement;
	statement *new;
	statement *return_statement;
	statement *old_return_statement = NULL;
	variable *new_statement_var;
	unsigned char arg_verified = 1;
	char *arg_pointer;
	char var_name[256];

	or_statement = parse_statement_value(c, &arg_verified);
	skip_whitespace(c);
	if(!or_statement){
		fprintf(stderr, "Error: could not parse statement value\n");
		error(1);
	}
	if(!arg_verified){
		fprintf(stderr, "Error: first argument of 'branch' must be verified\n");
		error(1);
	}
	if(**c != ','){
		fprintf(stderr, "Error: expected ','\n");
		error(1);
	}
	if(or_statement->num_bound_props || or_statement->num_bound_vars){
		fprintf(stderr, "Error: expected argument to have no bound variables or propositions\n");
		error(1);
	}
	arg_pointer = *c;
	++*c;

	skip_branch_args(c);
	if(**c != ')'){
		fprintf(stderr, "Error: expected ')'\n");
		error(1);
	}
	++*c;
	skip_whitespace(c);
	if(**c != '{'){
		fprintf(stderr, "Error: expected '{'\n");
		error(1);
	}
	++*c;
	skip_whitespace(c);

	while(*arg_pointer != ')'){
		arg_pointer++;
		skip_whitespace(&arg_pointer);
		get_identifier(&arg_pointer, var_name, 256);
		if(var_name[0] == '\0'){
			fprintf(stderr, "Error: expected identifier\n");
			error(1);
		}
		skip_whitespace(&arg_pointer);
		if(*arg_pointer != ',' && *arg_pointer != ')'){
			*c = arg_pointer;
			fprintf(stderr, "Error: expected ',' or ')'\n");
			error(1);
		}
		up_scope();
		if(!or_statement){
			*c = arg_pointer;
			fprintf(stderr, "Error: too many arguments to unpack\n");
			error(1);
		}
		if(*arg_pointer == ','){
			new = peel_or_left(&or_statement);
		} else {
			new = or_statement;
		}
		new_statement_var = create_statement_var(var_name, new);
		new_statement_var->num_references++;
		return_statement = verify_block(c, 0, NULL);
		if(!return_statement){
			fprintf(stderr, "Error: expected return statement\n");
			error(1);
		}
		if(max_statement_depth(return_statement) >= current_depth){
			free_statement(return_statement);
			fprintf(stderr, "Error: returned statement depends on variables in its scope\n");
			error(1);
		}

		if(old_return_statement && !compare_statement(return_statement, old_return_statement)){
			fprintf(stderr, "Error: mismatched returned statements\n");
			error(1);
		} else if(!old_return_statement){
			old_return_statement = return_statement;
		} else {
			free_statement(return_statement);
		}

		drop_scope();
		skip_whitespace(c);

		if(*arg_pointer == ','){
			if(strncmp(*c, "or", 2) || is_alphanumeric((*c)[2])){
				fprintf(stderr, "Error: expected 'or'\n");
				error(1);
			}
			*c += 2;
			skip_whitespace(c);
			if(**c != '{'){
				fprintf(stderr, "Error: expected '{'\n");
				error(1);
			}
			++*c;
			skip_whitespace(c);
		}
	}

	return old_return_statement;
}

statement *parse_statement_value_builtin(char **c, unsigned char *is_verified){
	unsigned char is_and;

	if(!strncmp(*c, "left", 4) && !is_alphanumeric((*c)[4])){
		*c += 4;
		skip_whitespace(c);
		if(**c != '('){
			return NULL;
		}
		++*c;
		return parse_left(c, is_verified);
	} else if(!strncmp(*c, "right", 5) && !is_alphanumeric((*c)[5])){
		*c += 5;
		skip_whitespace(c);
		if(**c != '('){
			return NULL;
		}
		++*c;
		return parse_right(c, is_verified);
	} else if((!strncmp(*c, "and", 3) && !is_alphanumeric((*c)[3])) || (!strncmp(*c, "or", 2) && !is_alphanumeric((*c)[2]))){
		if(**c == 'a'){
			*c += 3;
			is_and = 1;
		} else {
			is_and = 0;
			*c += 2;
		}
		skip_whitespace(c);
		if(**c != '('){
			return NULL;
		}
		++*c;
		return parse_and_or(c, is_and, is_verified);
	} else if(!strncmp(*c, "swap", 4) && !is_alphanumeric((*c)[4])){
		*c += 4;
		skip_whitespace(c);
		if(**c != '('){
			return NULL;
		}
		++*c;
		return parse_swap(c, is_verified);
	} else if(!strncmp(*c, "branch", 6) && !is_alphanumeric((*c)[6])){
		*c += 6;
		skip_whitespace(c);
		if(**c != '('){
			return NULL;
		}
		++*c;
		return parse_branch(c);
	}

	return NULL;
}

statement *parse_statement_identifier_or_value(char **c, unsigned char *is_verified, char end_char0, char end_char1){
	statement *output;
	char *beginning;

	beginning = *c;
	output = parse_statement_identifier_proposition(c);
	skip_whitespace(c);
	//The end characters 'end_char0' and 'end_char1' can't be operations
	//on statements. They are used to determine when the user has
	//specified a definition or is using a definition in an unverified
	//expression.
	if(output && (**c == end_char0 || **c == end_char1)){
		*is_verified = 0;
		return output;
	} else {
		if(output){
			free_statement(output);
		}
		*c = beginning;
		return parse_statement_value(c, is_verified);
	}
}

statement *parse_statement_value_parentheses(char **c, statement *output, unsigned char *is_verified, unsigned char *did_bind){
	statement_type compare_type;
	statement *next_output;
	statement *arg;
	variable *var;
	char var_name[256];

	if(output->num_bound_props || output->num_bound_vars){
		fprintf(stderr, "Error: expression has bound propositions or variables\n");
		error(1);
	}
	compare_type = output->type;
	skip_whitespace(c);
	if(**c == ')'){
		fprintf(stderr, "Error: expected expression or variable\n");
		error(1);
	}
	while(**c != ')'){
		if(output->type != compare_type){
			fprintf(stderr, "Error: expected ')' before next arguments\n");
			error(1);
		}
		if(compare_type == EXISTS || compare_type == FORALL){
			get_identifier(c, var_name, 256);
			if(var_name[0] == '\0'){
				fprintf(stderr, "Error: expected identifier name\n");
				error(1);
			}
			skip_whitespace(c);
			if(compare_type == EXISTS){
				*did_bind = 1;
				var = create_object_var(var_name);
			} else if(compare_type == FORALL){
				var = get_object_var(var_name);
			}
			if(!var){
				fprintf(stderr, "Error: failed to find or create variable '%s'\n", var_name);
				error(1);
			}
			next_output = output->child0;
			free(output);
			if(!substitute_variable(next_output, 0, var)){
				fprintf(stderr, "Error: failed to substitute variable\n");
				error(1);
			}
			output = next_output;
			add_bound_variables(output, -1);
		} else if(compare_type == IMPLIES || compare_type == NOT){
			arg = parse_statement_value_recursive(c, is_verified, did_bind);
			skip_whitespace(c);
			if(!arg){
				fprintf(stderr, "Error: failed to parse argument\n");
				error(1);
			}
			if(arg->num_bound_vars){
				fprintf(stderr, "Error: argument has bound variables\n");
				error(1);
			}
			if(arg->num_bound_props){
				fprintf(stderr, "Error: argument has bound propositions\n");
				error(1);
			}
			if(!compare_statement(arg, output->child0)){
				fprintf(stderr, "Error: invalid statement type for operation\n");
				error(1);
			}
			free_statement(arg);
			if(compare_type == IMPLIES){
				free_statement(output->child0);
				next_output = output->child1;
				free(output);
				output = next_output;
			} else if(compare_type == NOT){
				free_statement(output);
				output = create_statement(FALSE, 0, 0);
			}
		} else {
			fprintf(stderr, "Error: invalid statement type for parentheses operation\n");
			error(1);
		}
		if(**c != ',' && **c != ')'){
			fprintf(stderr, "Error: expected ',' or ')'\n");
			error(1);
		}
		if(**c == ','){
			++*c;
			skip_whitespace(c);
		}
	}
	++*c;

	return output;
}

static statement *parse_statement_brackets(char **c, statement *output){
	statement *arg;
	unsigned char foo_verified;

	do{
		if(!output->num_bound_props){
			fprintf(stderr, "Error: statement has no bound propositions\n");
			error(1);
		}
		++*c;
		skip_whitespace(c);

		arg = parse_statement_identifier_or_value(c, &foo_verified, ',', ']');

		if(!arg || arg->num_bound_props){
			return NULL;
		}

		skip_whitespace(c);
		if(**c != ',' && **c != ']'){
			fprintf(stderr, "Error: expected ',' or ']'\n");
			error(1);
		}
		substitute_proposition(output, arg);
		free_statement(arg);
	} while(**c == ',');

	++*c;

	return output;
}

statement *parse_anonymous_definition(char **c){
	statement *output;
	int *new_int;
	int num_bound_vars = 0;
	char name_buffer[256];

	clear_bound_variables();
	skip_whitespace(c);
	while(**c != ':'){
		get_identifier(c, name_buffer, 256);
		if(name_buffer[0] == '\0'){
			fprintf(stderr, "Error: expected identifier\n");
			error(1);
		}
		if(read_dictionary(bound_variables, name_buffer, 0)){
			fprintf(stderr, "Error: duplicate identifier\n");
			error(1);
		}
		new_int = malloc(sizeof(int));
		*new_int = num_bound_vars;
		write_dictionary(&bound_variables, name_buffer, new_int, 0);
		num_bound_vars++;
		skip_whitespace(c);
		if(**c == ','){
			++*c;
			skip_whitespace(c);
		} else if(**c != ':'){
			fprintf(stderr, "Error: expected ',' or ':'\n");
			error(1);
		}
	}
	++*c;
	skip_whitespace(c);
	output = parse_statement(c, num_bound_vars, 0);
	if(**c != '>'){
		fprintf(stderr, "Error: expected '>'\n");
		error(1);
	}
	++*c;
	clear_bound_variables();

	return output;
}

static statement *parse_statement_value_recursive(char **c, unsigned char *is_verified, unsigned char *did_bind){
	statement *output;

	skip_whitespace(c);
	if((output = parse_statement_value_builtin(c, is_verified))){
		//pass
	} else if(**c == '('){
		++*c;
		output = parse_statement_value_recursive(c, is_verified, did_bind);
		if(**c != ')'){
			if(output){
				free_statement(output);
			}
			return NULL;
		}
		++*c;
	} else if(**c == '<'){
		++*c;
		output = parse_anonymous_definition(c);
		*is_verified = 0;
	} else {
		output = parse_statement_identifier(c, is_verified);
		if(!output){
			return NULL;
		}
	}
	skip_whitespace(c);
	while(**c && **c != ';' && **c != ')' && **c != ',' && **c != ']' && **c != ':'){
		if(**c == '['){
			output = parse_statement_brackets(c, output);
			if(!output){
				return NULL;
			}
		} else if(**c == '('){
			++*c;
			output = parse_statement_value_parentheses(c, output, is_verified, did_bind);
		} else {
			free_statement(output);
			return NULL;
		}
		skip_whitespace(c);
	}

	return output;
}

statement *parse_statement_value(char **c, unsigned char *is_verified){
	unsigned char did_bind = 0;
	statement *output;

	*is_verified = 1;
	output = parse_statement_value_recursive(c, is_verified, &did_bind);
	if(output && !*is_verified && did_bind){
		free_statement(output);
		fprintf(stderr, "Error: cannot bind variable in unverified statement\n");
		error(1);
	}

	return output;
}