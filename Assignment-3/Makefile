all: hw2 clock_algo

hw2 : dberror.o storage_mgr.o buffer_mgr.o buffer_mgr_stat.o test_assign2_1.o
	gcc -o hw2 dberror.o storage_mgr.o buffer_mgr.o buffer_mgr_stat.o test_assign2_1.o -I.

clock_algo : dberror.o storage_mgr.o buffer_mgr.o buffer_mgr_stat.o  clock_test.o
	gcc -o clock_algo dberror.o storage_mgr.o buffer_mgr.o buffer_mgr_stat.o clock_test.o -I.
	
test_assign2_1.o : test_assign2_1.c test_helper.h buffer_mgr_stat.h buffer_mgr.h storage_mgr.h dberror.h 

clock_test.o : clock_test.c test_helper.h buffer_mgr_stat.h buffer_mgr.h storage_mgr.h dberror.h 

dberror.o : dberror.c dberror.h

storage_mgr.o : storage_mgr.c storage_mgr.h dberror.h 

buffer_mgr_stat.o : buffer_mgr_stat.c buffer_mgr_stat.h

buffer_mgr.o : buffer_mgr.c buffer_mgr.h storage_mgr.h