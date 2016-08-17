package logger

import "io"
import "fmt"
// import "os"
import "errors"
import "unsafe"


/*
#cgo LDFLAGS:  -lrt
#include "logger.h"
*/
import "C"

const NUM_CHUNK_POOLS = 4


var rptr *C.region

var chunk_id C.ulong

type LoggerWriter interface {
	io.Writer
	Sync() error
	WriteCgo([]byte) (int, error)
	WriteNoCrazyNoReflect([]byte) (int, error)
}

type loggerWriter struct { }

func (lw *loggerWriter) Sync() error {
	return nil
}

func (lw *loggerWriter) Write(bytes []byte) (written int, err error) {
	switch_chunks := false

	// paddinging needed to assure atomic pointer writes!!!!

	bytes = append(bytes, byte(0))

	
	msg_size := C.size_t(len(bytes)) 

	var chnk *C.chunk


	
	tail := (*C.chunk)(unsafe.Pointer(uintptr(rptr.tail) + uintptr(unsafe.Pointer(rptr))))
	

	if tail.len == 0 && tail.id == 0 && tail.next == 0 {
		tail = nil
		chnk = (*C.chunk)(unsafe.Pointer(uintptr(rptr.tail) + uintptr(unsafe.Pointer(rptr))))
	} else {
		chnk = (*C.chunk)(unsafe.Pointer(uintptr(rptr.tail) + C.sizeof_struct_chunk + uintptr(tail.len) + uintptr(unsafe.Pointer(rptr))))
	}

	if uintptr(unsafe.Pointer(chnk))+uintptr(msg_size) >= uintptr(rptr.head)+uintptr(unsafe.Pointer(rptr))+unsafe.Sizeof(rptr.chunk_pools[0].pool) {
		// fmt.Println("Too big....")

		for cp_idx := 0; cp_idx < NUM_CHUNK_POOLS; cp_idx++ {
			if uintptr(rptr.head) == uintptr(unsafe.Pointer(&rptr.chunk_pools[cp_idx].pool))-uintptr(unsafe.Pointer(rptr)) {
				// fmt.Printf("currently using chunk pool %d ...\n", cp_idx)
				// fmt.Printf("will switch to chunk pool %d ...\n", (cp_idx + 1) % NUM_CHUNK_POOLS)

				// READER DECTION LOGIC HERE
				// 			if (num_readers = atomic_load(&(rptr->chunk_pools[(cp_idx + 1) % NUM_CHUNK_POOLS].ref_count)) > 0) {
				// 				fprintf(stdout, " !!! potentially corrupting %d bad readers, wait just a hot minute...   \n", num_readers);
				// 				usleep(100);
				// 				if (num_readers = atomic_load(&(rptr->chunk_pools[(cp_idx + 1) % NUM_CHUNK_POOLS].ref_count)) > 0) {
				// 					fprintf(stdout, " !!! damn, hot mess: %d bad readers, continuing anyways...   \n", num_readers);
				// 					// exit(1);
				// 				} else {
				// 					fprintf(stdout, " !!! fixed, thank god  \n", num_readers);
				// 				}
				// 			}
				chnk = (*C.chunk)(unsafe.Pointer(&rptr.chunk_pools[(cp_idx + 1) % NUM_CHUNK_POOLS].pool))

			}
		}
		switch_chunks = true
	}

	chnk.len = msg_size // + pad_size
	chnk.next = 0
	chunk_id++
	chnk.id = chunk_id

	// copy bytes
	data_slice := (*[(1 << 31) / unsafe.Sizeof([]byte{})]byte)(unsafe.Pointer(uintptr(unsafe.Pointer(chnk)) + C.sizeof_struct_chunk))[:len(bytes):len(bytes)]

	written_bytes := copy(data_slice, bytes)

	// null out pad_size at end

	if tail != nil {
		tail.next = (C.ptrdiff_t)(uintptr(unsafe.Pointer(chnk)) - uintptr(unsafe.Pointer(rptr)))
	}
	rptr.tail = (C.ptrdiff_t)(uintptr(unsafe.Pointer(chnk)) - uintptr(unsafe.Pointer(rptr)))
	if switch_chunks {
		// fmt.Println("switch chunks")
		rptr.head = (C.ptrdiff_t)(uintptr(unsafe.Pointer(chnk)) - uintptr(unsafe.Pointer(rptr)))
	}
	

	if written_bytes != int(msg_size) {
		return written_bytes, errors.New("logger_write error")
	}

	return int(msg_size)-1, nil


}

func (lw *loggerWriter) WriteNoCrazyNoReflect(bytes []byte) (written int, err error) {
	switch_chunks := false
	bytes = append(bytes, byte(0))
	msg_size := C.size_t(len(bytes)) 
	var chnk *C.chunk
	tail := (*C.chunk)(unsafe.Pointer(uintptr(rptr.tail) + uintptr(unsafe.Pointer(rptr))))
	if tail.len == 0 && tail.id == 0 && tail.next == 0 {
		tail = nil
		chnk = (*C.chunk)(unsafe.Pointer(uintptr(rptr.tail) + uintptr(unsafe.Pointer(rptr))))
	} else {
		chnk = (*C.chunk)(unsafe.Pointer(uintptr(rptr.tail) + C.sizeof_struct_chunk + uintptr(tail.len) + uintptr(unsafe.Pointer(rptr))))
	}
	if uintptr(unsafe.Pointer(chnk))+uintptr(msg_size) >= uintptr(rptr.head)+uintptr(unsafe.Pointer(rptr))+unsafe.Sizeof(rptr.chunk_pools[0].pool) {
		for cp_idx := 0; cp_idx < NUM_CHUNK_POOLS; cp_idx++ {
			if uintptr(rptr.head) == uintptr(unsafe.Pointer(&rptr.chunk_pools[cp_idx].pool))-uintptr(unsafe.Pointer(rptr)) {
				chnk = (*C.chunk)(unsafe.Pointer(&rptr.chunk_pools[(cp_idx + 1) % NUM_CHUNK_POOLS].pool))
			}
		}
		switch_chunks = true
	}
	chnk.len = msg_size
	chnk.next = 0
	chunk_id++
	chnk.id = chunk_id
	data_slice := C.GoBytes(unsafe.Pointer(uintptr(unsafe.Pointer(chnk)) + C.sizeof_struct_chunk), C.int(msg_size))
	written_bytes := copy(data_slice, bytes)
	if tail != nil {
		tail.next = (C.ptrdiff_t)(uintptr(unsafe.Pointer(chnk)) - uintptr(unsafe.Pointer(rptr)))
	}
	rptr.tail = (C.ptrdiff_t)(uintptr(unsafe.Pointer(chnk)) - uintptr(unsafe.Pointer(rptr)))
	if switch_chunks {
		rptr.head = (C.ptrdiff_t)(uintptr(unsafe.Pointer(chnk)) - uintptr(unsafe.Pointer(rptr)))
	}
	if written_bytes != int(msg_size) {
		return written_bytes, errors.New("logger_write error")
	}
	return int(msg_size)-1, nil
}


func (lw *loggerWriter) WriteCgo(bytes []byte) (written int, err error) {
	bytes = append(bytes, byte(0))

	lb := len(bytes)
	lw_ret := C.logger_write((*C.char)(unsafe.Pointer(&bytes[0])))
	if lw_ret == C.int(lb) {
		return lb-1, nil
	}
	return 0, errors.New("logger_write error")

}

func (lw *loggerWriter) stub(bytes []byte) (written int, err error) {
	fmt.Println("stub")
	return 0, errors.New("logger_write error")

}

func NewLoggerWriter() LoggerWriter {
	rptr = C.logger_init()

	// fmt.Println(rptr.head)
	loggerWriter := loggerWriter{ }
	return &loggerWriter
}

