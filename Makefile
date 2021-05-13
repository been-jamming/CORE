CC = gcc
DEL = rm
FLAGS = -Wall -DUSE_CUSTOM_ALLOC

core: commands.c custom_malloc.o dictionary.o proposition.o expression.o
	$(CC) commands.c expression.o proposition.o dictionary.o custom_malloc.o $(FLAGS) -o core

custom_malloc.o: custom_malloc.c
	$(CC) custom_malloc.c -c $(FLAGS)

dictionary.o: dictionary.c
	$(CC) dictionary.c -c $(FLAGS)

proposition.o: proposition.c
	$(CC) proposition.c -c $(FLAGS)

expression.o: expression.c
	$(CC) expression.c -c $(FLAGS)

clean:
	$(DEL) dictionary.o
	$(DEL) proposition.o
	$(DEL) expression.o
	$(DEL) core
