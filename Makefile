CC = gcc
DEL = rm
FLAGS = -Wall -pedantic -DUSE_CUSTOM_ALLOC -g
#FLAGS = -Wall -pedantic -g

core: build/commands.o build/custom_malloc.o build/dictionary.o build/predicate.o build/expression.o build/imports.o
	$(CC) build/commands.o build/expression.o build/predicate.o build/dictionary.o build/custom_malloc.o build/imports.o $(FLAGS) -o core

build/custom_malloc.o: src/custom_malloc.c
	$(CC) src/custom_malloc.c -c -o build/custom_malloc.o $(FLAGS)

build/dictionary.o: src/dictionary.c
	$(CC) src/dictionary.c -c -o build/dictionary.o $(FLAGS)

build/predicate.o: src/predicate.c
	$(CC) src/predicate.c -c -o build/predicate.o $(FLAGS)

build/expression.o: src/expression.c
	$(CC) src/expression.c -c -o build/expression.o $(FLAGS)

build/imports.o: src/imports.c
	$(CC) src/imports.c -c -o build/imports.o $(FLAGS)

build/commands.o: src/commands.c
	$(CC) src/commands.c -c -o build/commands.o $(FLAGS)

clean:
	$(DEL) build/custom_malloc.o
	$(DEL) build/dictionary.o
	$(DEL) build/predicate.o
	$(DEL) build/expression.o
	$(DEL) build/imports.o
	$(DEL) build/commands.o
	$(DEL) core

compile_proofs:
	./core proofs/*
	rm -rf docs/*
	python generate_docs.py
