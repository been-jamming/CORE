CC = gcc
DEL = rm
FLAGS = -Wall -DUSE_CUSTOM_ALLOC -g

core: src/commands.c custom_malloc.o dictionary.o proposition.o expression.o
	$(CC) src/commands.c expression.o proposition.o dictionary.o custom_malloc.o $(FLAGS) -o core

custom_malloc.o: src/custom_malloc.c
	$(CC) src/custom_malloc.c -c $(FLAGS)

dictionary.o: src/dictionary.c
	$(CC) src/dictionary.c -c $(FLAGS)

proposition.o: src/proposition.c
	$(CC) src/proposition.c -c $(FLAGS)

expression.o: src/expression.c
	$(CC) src/expression.c -c $(FLAGS)

clean:
	$(DEL) custom_malloc.o
	$(DEL) dictionary.o
	$(DEL) proposition.o
	$(DEL) expression.o
	$(DEL) core

compile_proofs:
	./core proofs/*
	rm -rf docs/*
	python generate_docs.py
