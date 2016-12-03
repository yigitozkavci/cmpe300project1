CC = gcc
FLAGS = -g -Wall
all: main.o
	$(CC) $(FLAGS) -o main main.o convolution.o matrix.o

main.o: main.c convolution.o
	$(CC) $(FLAGS) -c main.c

convolution.o: convolution.c convolution.h matrix.o
	$(CC) $(FLAGS) -c convolution.c

matrix.o: matrix.c matrix.h
	$(CC) $(FLAGS) -c matrix.c

clean:
	$(RM) main *.o binary.txt smooth.txt original.txt
