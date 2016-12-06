CC = mpicc
FLAGS = -g
all: main.o
	$(CC) $(FLAGS) -o main main.o convolution.o matrix.o debug.o

main.o: main.c convolution.o debug.o
	$(CC) $(FLAGS) -c main.c

convolution.o: convolution.c convolution.h matrix.o debug.o
	$(CC) $(FLAGS) -c convolution.c

matrix.o: matrix.c matrix.h
	$(CC) $(FLAGS) -c matrix.c

debug.o: debug.c debug.h
	$(CC) $(FLAGS) -c debug.c

clean:
	$(RM) main *.o binary.txt smooth.txt original.txt
