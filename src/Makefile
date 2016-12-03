CC = gcc
FLAGS = -g -Wall
all: main.o convolution.o matrix.o
	$(CC) $(FLAGS) -o main main.o convolution.o matrix.o

main.o: main.c convolution.o matrix.o
	$(CC) $(FLAGS) -c main.c

convolution.o: convolution.c convolution.h
	$(CC) $(FLAGS) -c convolution.c

matrix.o: matrix.c matrix.h
	$(CC) $(FLAGS) -c matrix.c

clean:
	$(RM) main *.o
