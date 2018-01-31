CC = gcc
SRC = dberror.c storage_mgr.c buffer_mgr.c buffer_mgr_stat.c buffer_ll.c test_assign2_1.c 
OBJ = dberror.o storage_mgr.o buffer_mgr.o  buffer_mgr_stat.o buffer_ll.o test_assign2_1.o

assign1: $(OBJ)
	$(CC) -o test_assign2_1 $?

%.o: %.c
	$(CC) -g -c $<

run: assign1
	./test_assign2_1

clean:
	rm test_assign2_1.c *.o
