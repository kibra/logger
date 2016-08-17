#include <sys/mman.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>


#include "../logger.h"

FILE *log_file_fp;
region *rptr;

unsigned chunk_id;


void walk_chunks(region *rptr) {
	

	atomic_int *chunk_pool_refcnt = 0;


	chunk *cptr = (void *)rptr+(int)rptr->head;
	// protect w/ atomic inc before stepping into the loop

	for (int cp_idx = 0; cp_idx < NUM_CHUNK_POOLS; ++cp_idx) {
		if ( (void *)cptr == &(rptr->chunk_pools[cp_idx].pool) ) {
			chunk_pool_refcnt = &(rptr->chunk_pools[cp_idx].ref_count);
			atomic_fetch_add(&(rptr->chunk_pools[cp_idx].ref_count), 1);
		}
	}
	while (cptr != (void *)rptr) {
		
		if (chunk_id < cptr->id) {
			if (cptr->id > chunk_id+1) {
				fprintf(stderr, "Yikes, we missed %u logs\n", cptr->id-chunk_id);
			}
			chunk_id = cptr->id;
			fprintf(stdout, "%u %s\n", cptr->id, cptr->data);
		}

		cptr = (void *)rptr+atomic_load_explicit(&cptr->next, memory_order_relaxed);
		asm volatile("lfence" ::: "memory"); // test for correctness here

		
		
		for (int cp_idx = 0; cp_idx < NUM_CHUNK_POOLS; ++cp_idx) { // countdown pointer loop?

			if ( (void *)cptr == &(rptr->chunk_pools[cp_idx].pool) ) {
				atomic_fetch_add(&(rptr->chunk_pools[cp_idx].ref_count), 1); 
				atomic_fetch_sub(chunk_pool_refcnt, 1);	
				chunk_pool_refcnt = &(rptr->chunk_pools[cp_idx].ref_count);
				// memory order ?
			}
		}

		usleep(5000);
	}

	


	if (chunk_pool_refcnt) {
		atomic_fetch_sub(chunk_pool_refcnt, 1);
	}

	// fprintf(stdout, "  got null \n");
}

  
int
main(int argc, char * const *argv)
{
	
	chunk_id = 0;
	// log_file_fp = fopen("fulllog.log", "w");
	// if (log_file_fp == NULL) {
	// 	printf("fopen fail");
	// 	exit(1);
	// }

	int fd;

	fd = shm_open("/myregion", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		fprintf(stdout, "shm_open error...\n");
	}

	if (ftruncate(fd, sizeof(struct region)) == -1) {
		fprintf(stdout, "ftruncate error...\n");
	}

	rptr = mmap(NULL, sizeof(struct region), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // less PROT_WRITE

	if (rptr == MAP_FAILED) {
		fprintf(stdout, "mmap error...\n");
	}

	for (int cp_idx = 0; cp_idx < NUM_CHUNK_POOLS; ++cp_idx) {
		fprintf(stdout, "%td\n", (void *)&rptr->chunk_pools[cp_idx].pool-(void *)rptr);
	}

	// if (mprotect(rptr, 1, PROT_WRITE) == -1) {
	// 	perror("mprotect error");
	// }

	unsigned long last_non_zero = 0;
	unsigned long pre = 0;
	unsigned long cur = 0;
	while (1) {


		walk_chunks(rptr);

		// usleep(100);

		usleep(100);
	}
	// fclose(log_file_fp);
	exit(1);
}