#include <stddef.h>
#include <stdatomic.h>

#define REGION_SIZE 1024*4
#define NUM_CHUNK_POOLS 4
#define CHUNK_POOL_SIZE REGION_SIZE/NUM_CHUNK_POOLS

typedef struct chunk chunk;
struct chunk {
	size_t		len;
	unsigned long    id;
	ptrdiff_t	next; // _Atomic needed?
	char        data[];
};

typedef struct chunk_pool {
	unsigned int ref_count; // _Atomic needed?
	char pool[CHUNK_POOL_SIZE];
} chunk_pool;


typedef struct region {
	ptrdiff_t head; // _Atomic needed?
	ptrdiff_t tail; // _Atomic needed?
    chunk_pool chunk_pools[NUM_CHUNK_POOLS];
} region;

region * logger_init();
int logger_write(char *msg);

