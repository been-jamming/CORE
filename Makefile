CC = gcc
DEL = rm
FLAGS = -Wall -pedantic -DUSE_CUSTOM_ALLOC -g
#FLAGS = -Wall -pedantic -g

core: src/commands.c custom_malloc.o dictionary.o predicate.o expression.o
	$(CC) src/commands.c expression.o predicate.o dictionary.o custom_malloc.o $(FLAGS) -o core

custom_malloc.o: src/custom_malloc.c
	$(CC) src/custom_malloc.c -c $(FLAGS)

dictionary.o: src/dictionary.c
	$(CC) src/dictionary.c -c $(FLAGS)

predicate.o: src/predicate.c
	$(CC) src/predicate.c -c $(FLAGS)

expression.o: src/expression.c
	$(CC) src/expression.c -c $(FLAGS)

clean:
	$(DEL) custom_malloc.o
	$(DEL) dictionary.o
	$(DEL) predicate.o
	$(DEL) expression.o
	$(DEL) core

compile_proofs:
	./core proofs/*
	rm -rf docs/*
	python generate_docs.py
