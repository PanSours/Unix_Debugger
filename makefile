all: picodb

picodb: picodb.o
	gcc -Wall -o picodb picodb.o

picodb.o: picodb.c
	gcc -c picodb.c

clean:
	rm -rf *.o picodb

