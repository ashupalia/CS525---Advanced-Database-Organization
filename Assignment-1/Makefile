CC = gcc
SRC = dberror.c storage_mgr.c test_assign1_1.c
OBJ = dberror.o storage_mgr.o test_assign1_1.o

assign1: $(OBJ)
	$(CC) -o test_assign1 $?

%.o: %.c
	$(CC) -g -c $<

run: assign1
	./test_assign1

clean:
	rm test_assign1 *.o
