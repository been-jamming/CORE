CC = gcc
DEL = rm
FLAGS = -Wall

core: commands.c dictionary.o proposition.o expression.o
	$(CC) commands.c expression.o proposition.o dictionary.o $(FLAGS) -o core

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
