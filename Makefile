CC = cc
DEL = rm -r
DIR = mkdir -p
FLAGS = -Wall -pedantic -DUSE_CUSTOM_ALLOC -g
#FLAGS = -Wall -pedantic -g
OBJS = $(addprefix build/, commands.o custom_malloc.o dictionary.o predicate.o compare.o expression.o imports.o)

core: $(OBJS)
	$(CC) $(OBJS) $(FLAGS) -o core

build/custom_malloc.o: src/custom_malloc.c | build
	$(CC) src/custom_malloc.c -c -o build/custom_malloc.o $(FLAGS)

build/dictionary.o: src/dictionary.c | build
	$(CC) src/dictionary.c -c -o build/dictionary.o $(FLAGS)

build/compare.o: src/compare.c | build
	$(CC) src/compare.c -c -o build/compare.o $(FLAGS)

build/predicate.o: src/predicate.c | build
	$(CC) src/predicate.c -c -o build/predicate.o $(FLAGS)

build/expression.o: src/expression.c | build
	$(CC) src/expression.c -c -o build/expression.o $(FLAGS)

build/imports.o: src/imports.c | build
	$(CC) src/imports.c -c -o build/imports.o $(FLAGS)

build/commands.o: src/commands.c | build
	$(CC) src/commands.c -c -o build/commands.o $(FLAGS)

build:
	$(DIR) build

clean:
	$(DEL) build
	$(DEL) core

