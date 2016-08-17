#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>


#include "logger.h"


region *rptr;

pthread_mutex_t region_write_lock;

unsigned chunk_id;

region * logger_init() {
	int fd;

	chunk_id = 0;

	pthread_mutexattr_t mattr;
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ADAPTIVE_NP);


	pthread_mutex_init(&region_write_lock, &mattr);

	fd = shm_open("/myregion", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		fprintf(stdout, "shm_open error...\n");
	}

	if (ftruncate(fd, sizeof(struct region)) == -1) {
		fprintf(stdout, "ftruncate error...\n");
	}

	rptr = mmap(NULL, sizeof(struct region), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (rptr == MAP_FAILED) {
		fprintf(stdout, "mmap error...\n");
	}

	memset(rptr, 0, sizeof(struct region));

	// initalize region
	rptr->head = 0;
	rptr->tail = 0;
	for (int cp_idx = 0; cp_idx < NUM_CHUNK_POOLS; ++cp_idx) { // countdown pointer loop?
		rptr->chunk_pools[cp_idx].ref_count = ATOMIC_VAR_INIT(0);
	}

	rptr->head = (void *)&rptr->chunk_pools[0].pool-(void *)rptr;
	rptr->tail = (void *)&rptr->chunk_pools[0].pool-(void *)rptr;
	
	return rptr;	
}


int logger_write(char *msg) {
	// usleep(1);
	unsigned int num_readers;
	chunk *chnk;
	int switch_chunks = 0;
	size_t msg_size = strlen(msg)+1;
	size_t pad_size = 0;
	if (msg_size % 8 != 0) {
		pad_size = 8-msg_size%8;
	}
	pthread_mutex_lock(&region_write_lock);
	chunk *tail = (ptrdiff_t)rptr->tail+(void *)rptr;
	if (tail->len == 0 && tail->id == 0 && tail->next == 0) {
		// no chunks
		tail = NULL;
		chnk = (ptrdiff_t)rptr->tail+(void *)rptr;
	} else {
		chnk = (ptrdiff_t)rptr->tail+sizeof(chunk)+tail->len+(void *)rptr;
	}
	
	if ((void *)((void *)chnk+sizeof(chunk)+msg_size+pad_size) >= rptr->head+(void *)rptr+sizeof(rptr->chunk_pools[0].pool)) {
		// fprintf(stdout, "Too big...\n");

		for (int cp_idx = 0; cp_idx < NUM_CHUNK_POOLS; ++cp_idx) { // countdown pointer loop?
			if (rptr->head == (void *)&(rptr->chunk_pools[cp_idx].pool)-(void *)rptr) {
				// fprintf(stdout, "currently using chunk pool %d ...\n", cp_idx);
				// fprintf(stdout, "will switch to chunk pool %d ...\n", (cp_idx + 1) % NUM_CHUNK_POOLS);
				if (num_readers = atomic_load(&(rptr->chunk_pools[(cp_idx + 1) % NUM_CHUNK_POOLS].ref_count)) > 0) {
					fprintf(stdout, " !!! potentially corrupting %d bad readers, wait just a hot minute...   \n", num_readers);
					usleep(100);
					if (num_readers = atomic_load(&(rptr->chunk_pools[(cp_idx + 1) % NUM_CHUNK_POOLS].ref_count)) > 0) {
						fprintf(stdout, " !!! damn, hot mess: %d bad readers, continuing anyways...   \n", num_readers);
						// exit(1);
					} else {
						fprintf(stdout, " !!! fixed, thank god  \n", num_readers);
					}
				}
				chnk = (chunk *)&rptr->chunk_pools[(cp_idx + 1) % NUM_CHUNK_POOLS].pool;
			}
		}
		switch_chunks = 1;
	}


	chnk->len = msg_size+pad_size;
	chnk->next = 0;
	chnk->id = ++chunk_id;
	memcpy(&chnk->data, msg, msg_size);
	memset((void *)(&chnk->data)+msg_size, 0, pad_size);
	chnk->data[chnk->len-1] = '\0';
	asm volatile("mfence" ::: "memory"); // test for correctness here
	if (tail != NULL) {
		atomic_store_explicit(&tail->next, (void *)chnk-(void *)rptr, memory_order_relaxed);
	}
	
	atomic_store_explicit(&rptr->tail, (void *)chnk-(void *)rptr, memory_order_relaxed);
	if (switch_chunks) {
		atomic_store_explicit(&rptr->head, (void *)chnk-(void *)rptr, memory_order_relaxed);
	}
	pthread_mutex_unlock(&region_write_lock);
	return strlen(msg)+1;
	// fprintf(stdout, "\n");
}

int logger_write_no_lock_or_mb(char *msg) {
	// usleep(1);
	unsigned int num_readers;
	chunk *chnk;
	int switch_chunks = 0;
	size_t msg_size = strlen(msg)+1;
	size_t pad_size = 0;
	if (msg_size % 8 != 0) {
		pad_size = 8-msg_size%8;
	}
	// pthread_mutex_lock(&region_write_lock);
	chunk *tail = (ptrdiff_t)rptr->tail+(void *)rptr;
	if (tail->len == 0 && tail->id == 0 && tail->next == 0) {
		// no chunks
		tail = NULL;
		chnk = (ptrdiff_t)rptr->tail+(void *)rptr;
	} else {
		chnk = (ptrdiff_t)rptr->tail+sizeof(chunk)+tail->len+(void *)rptr;
	}
	
	if ((void *)((void *)chnk+sizeof(chunk)+msg_size+pad_size) >= rptr->head+(void *)rptr+sizeof(rptr->chunk_pools[0].pool)) {
		// fprintf(stdout, "Too big...\n");

		for (int cp_idx = 0; cp_idx < NUM_CHUNK_POOLS; ++cp_idx) { // countdown pointer loop?
			if (rptr->head == (void *)&(rptr->chunk_pools[cp_idx].pool)-(void *)rptr) {
				// fprintf(stdout, "currently using chunk pool %d ...\n", cp_idx);
				// fprintf(stdout, "will switch to chunk pool %d ...\n", (cp_idx + 1) % NUM_CHUNK_POOLS);
				if (num_readers = atomic_load(&(rptr->chunk_pools[(cp_idx + 1) % NUM_CHUNK_POOLS].ref_count)) > 0) {
					fprintf(stdout, " !!! potentially corrupting %d bad readers, wait just a hot minute...   \n", num_readers);
					usleep(100);
					if (num_readers = atomic_load(&(rptr->chunk_pools[(cp_idx + 1) % NUM_CHUNK_POOLS].ref_count)) > 0) {
						fprintf(stdout, " !!! damn, hot mess: %d bad readers, continuing anyways...   \n", num_readers);
						// exit(1);
					} else {
						fprintf(stdout, " !!! fixed, thank god  \n", num_readers);
					}
				}
				chnk = (chunk *)&rptr->chunk_pools[(cp_idx + 1) % NUM_CHUNK_POOLS].pool;
			}
		}
		switch_chunks = 1;
	}


	chnk->len = msg_size+pad_size;
	chnk->next = 0;
	chnk->id = ++chunk_id;
	memcpy(&chnk->data, msg, msg_size);
	memset((void *)(&chnk->data)+msg_size, 0, pad_size);
	chnk->data[chnk->len-1] = '\0';
	// asm volatile("mfence" ::: "memory"); // test for correctness here
	if (tail != NULL) {
		atomic_store_explicit(&tail->next, (void *)chnk-(void *)rptr, memory_order_relaxed);
	}
	
	atomic_store_explicit(&rptr->tail, (void *)chnk-(void *)rptr, memory_order_relaxed);
	if (switch_chunks) {
		atomic_store_explicit(&rptr->head, (void *)chnk-(void *)rptr, memory_order_relaxed);
	}
	// pthread_mutex_unlock(&region_write_lock);
	return strlen(msg)+1;
	// fprintf(stdout, "\n");
}
