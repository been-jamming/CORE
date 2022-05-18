typedef struct context context;

struct context{
	dictionary variables;
	dictionary definitions;
	dictionary relations;
	unsigned int num_references;
};
