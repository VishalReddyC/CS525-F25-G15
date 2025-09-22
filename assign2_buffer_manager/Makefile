CC = gcc
CFLAGS = -Wall -g

OBJS = buffer_mgr.o buffer_mgr_stat.o dberror.o storage_mgr.o

all: test_assign2_1 test_assign2_2

test_assign2_1: $(OBJS) test_assign2_1.o
	$(CC) $(CFLAGS) -o test_assign2_1 $(OBJS) test_assign2_1.o

test_assign2_2: $(OBJS) test_assign2_2.o
	$(CC) $(CFLAGS) -o test_assign2_2 $(OBJS) test_assign2_2.o

clean:
	rm -f *.o test_assign2_1 test_assign2_2

