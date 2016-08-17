#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>

#include <time.h>



// #include "../logger.h"

#include "../logger.c"

#define TEST_ITERS 100000000
#define BILLION 1000000000L



int main(int argc, char * const *argv) {
	logger_init();


	struct timespec ts0, ts1;
	uint64_t diff;
	clock_gettime(CLOCK_MONOTONIC, &ts0);
	for (int i = 0; i < TEST_ITERS; ++i) {
		// fprintf(stdout, "%d %d", rptr->head, rptr->tail);
		// for (int cp_idx = 0; cp_idx < NUM_CHUNK_POOLS; ++cp_idx) { // countdown pointer loop?
		// 	fprintf(stdout, " %d", atomic_load(&(rptr->chunk_pools[cp_idx].ref_count)));
		// }
		// fprintf(stdout, "\n");

		// size_t size = sizeof(*rptr);
		// for (unsigned char *byte = (unsigned char *)rptr; size--; ++byte) {
		// 	fprintf(stdout, "%d ", *byte);
		// }
		// fprintf(stdout, "\n");


		// sprintf(b, "test-log-linffe-%d", i++);

		// if (i % 100000 == 0) {
		// 	fprintf(stdout, "%d\n", i);
		// }

		logger_write("Blah blah blah");
	}
	clock_gettime(CLOCK_MONOTONIC, &ts1);

	diff = BILLION * (ts1.tv_sec - ts0.tv_sec) + ts1.tv_nsec - ts0.tv_nsec;
	printf("%-48s%llu ns/op\n", "logger_write", (long long unsigned int) diff / TEST_ITERS);
	// printf("%Lf s (wall)\n", (long double) diff / BILLION);
	// printf("%llu ns (wall)\n", (long long unsigned int) diff);

	clock_gettime(CLOCK_MONOTONIC, &ts0);
	for (int i = 0; i < TEST_ITERS; ++i) {
		logger_write_no_lock_or_mb("Blah blah blah");
	}
	clock_gettime(CLOCK_MONOTONIC, &ts1);
	diff = BILLION * (ts1.tv_sec - ts0.tv_sec) + ts1.tv_nsec - ts0.tv_nsec;
	printf("%-48s%llu ns/op\n", "logger_write_no_lock_or_mb", (long long unsigned int) diff / TEST_ITERS);



	
	exit(1);
}