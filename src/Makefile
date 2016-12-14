CC = mpicc
FLAGS = -g
all: main.o
	$(CC) $(FLAGS) -o main main.o debug.o slice.o util.o

main.o: main.c debug.o slice.o util.o
	$(CC) $(FLAGS) -c main.c

debug.o: debug.c debug.h
	$(CC) $(FLAGS) -c debug.c

slice.o: slice.c slice.h
	$(CC) $(FLAGS) -c slice.c

util.o: util.c util.h
	$(CC) $(FLAGS) -c util.c

clean:
	$(RM) main *.o binary.txt smooth.txt original.txt
