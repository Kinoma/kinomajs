CFLAGS += -Wall -W -O3 

default: mtest

mtest: mtest.o
	$(CC) $(CFLAGS) mtest.o -ltommath -o mtest

clean:
	rm -f *.o mtest *~
