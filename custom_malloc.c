#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define CMALLOC_PRINT_ERRORS

typedef struct cmalloc_node cmalloc_node;

struct cmalloc_node{
	cmalloc_node *child0;
	cmalloc_node *child1;
	cmalloc_node *parent;
	void *value;
};

static cmalloc_node *global_node = NULL;

static cmalloc_node *seek_parent(uintptr_t *ptr_num){
	void *pointer;
	cmalloc_node *current_node;
	cmalloc_node *parent_node;

	if(!global_node){
#ifdef CMALLOC_PRINT_ERRORS
		fprintf(stderr, "cmalloc error: cmalloc must be initialized\n");
#endif
		return NULL;
	}

	pointer = (void *) *ptr_num;
	current_node = global_node;
	while(current_node){
		if(current_node->value == pointer){
			return current_node;
		}
		parent_node = current_node;
		if(*ptr_num&1){
			current_node = current_node->child1;
		} else {
			current_node = current_node->child0;
		}
		//Preserve the last bit of ptr_num for the caller
		if(current_node){
			*ptr_num >>= 1;
		}
	}

	return parent_node;
}

void *custom_malloc(size_t size){
	void *output;
	uintptr_t ptr_num;
	cmalloc_node *current_node;
	cmalloc_node *parent_node;

	output = malloc(size);
	if(!output){
#ifdef CMALLOC_PRINT_ERRORS
		fprintf(stderr, "cmalloc error: malloc returned NULL\n");
#endif
		return NULL;
	}
	ptr_num = (uintptr_t) output;

	parent_node = seek_parent(&ptr_num);
	if(!parent_node){
		return NULL;
	}

	current_node = malloc(sizeof(cmalloc_node));
	if(!current_node){
#ifdef CMALLOC_PRINT_ERRORS
		fprintf(stderr, "cmalloc error: malloc returned NULL\n");
#endif
		return NULL;
	}
	if(ptr_num&1){
		parent_node->child1 = current_node;
	} else {
		parent_node->child0 = current_node;
	}
	current_node->parent = parent_node;
	current_node->value = output;
	current_node->child0 = NULL;
	current_node->child1 = NULL;

	return output;
}

void custom_free(void *pointer){
	uintptr_t ptr_num;
	cmalloc_node *parent_node;
	cmalloc_node *next_parent;

	if(!pointer){
		return;
	}

	ptr_num = (uintptr_t) pointer;

	parent_node = seek_parent(&ptr_num);
	if(!parent_node){
		return;
	}

	if(parent_node->value != pointer){
#ifdef CMALLOC_PRINT_ERRORS
		fprintf(stderr, "cmalloc error: pointer to free not found\n");
#endif
		return;
	}
	free(pointer);
	parent_node->value = NULL;
	while(!parent_node->value && parent_node != global_node && !parent_node->child0 && !parent_node->child1){
		next_parent = parent_node->parent;
		if(next_parent->child0 == parent_node){
			next_parent->child0 = NULL;
		} else if(next_parent->child1 == parent_node){
			next_parent->child1 = NULL;
		}
		free(parent_node);
		parent_node = next_parent;
	}
}

static void custom_malloc_abort_node(cmalloc_node *node){
	if(node->child0){
		custom_malloc_abort_node(node->child0);
	}
	if(node->child1){
		custom_malloc_abort_node(node->child1);
	}
	if(node->value){
		free(node->value);
	}
	free(node);
}

void custom_malloc_abort(){
	if(!global_node){
#ifdef CMALLOC_PRINT_ERRORS
		fprintf(stderr, "cmalloc error: cmalloc must be initialized\n");
#endif
		return;
	}

	custom_malloc_abort_node(global_node);
}

void custom_malloc_init(){
	global_node = malloc(sizeof(cmalloc_node));
	if(!global_node){
#ifdef CMALLOC_PRINT_ERRORS
		fprintf(stderr, "cmalloc error: cmalloc initialization failed because malloc returned NULL\n");
#endif
		return;
	}
	global_node->value = NULL;
	global_node->parent = NULL;
	global_node->child0 = NULL;
	global_node->child1 = NULL;
}

void custom_malloc_deinit(){
	if(!global_node){
#ifdef CMALLOC_PRINT_ERRORS
		fprintf(stderr, "cmalloc error: cmalloc must be initialized\n");
#endif
		return;
	}
	free(global_node);
}

