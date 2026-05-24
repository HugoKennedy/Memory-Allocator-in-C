CFLAGS=-Wall -g
LDFLAGS=-lm

BIN=memory_alloc

all: $(BIN)

memory_alloc: memory_alloc.c memory_alloc_main.c memory_alloc.h
	$(CC) -o memory_alloc memory_alloc.c memory_alloc_main.c  -Icmocka -Lcmocka -lcmocka $(CFLAGS) $(LDFLAGS)

test: test_memory_alloc
	./test_memory_alloc

test_memory_alloc: test_memory_alloc.c memory_alloc.c memory_alloc.h
	$(CC) -o test_memory_alloc test_memory_alloc.c memory_alloc.c -Icmocka -Lcmocka -lcmocka $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(BIN) test_memory_alloc
