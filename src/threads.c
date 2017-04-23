#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "threads.h"

// steps:
// thread synchronization                                                DONE
// shared buffer - integer                                               DONE
// get external data - fixed size                                        DONE
// process data - fixed size                                             DONE
// get external data - various size (pointer + calloc)
// process data - various size
// add more information to data packet structure for better tracking
// thread synchronization optimization
// message queue for passing data


//TODO Define global data structures to be used
pthread_rwlock_t rw_lock_mutex;
pthread_mutex_t data_generator_mutex = PTHREAD_MUTEX_INITIALIZER;

struct MSG_BUF {
	char *data_pkt;
	unsigned int buf_size;
} message_buffer;

/*
 * function: data_pkt_generator
 * generate fixed size data packet
 * return size of generated packet in Byte
 * */
int data_pkt_generator (char* data, unsigned int buffer_size_in_bytes) {
	time_t timeofday;
	srand((unsigned) time(&timeofday));

	for (unsigned int i = 0; i < buffer_size_in_bytes; i++) {
		data[i] = (char) rand () % 256; // generate 1 byte random number
	}
	return buffer_size_in_bytes;
}

int get_external_data(char *buffer, int bufferSizeInBytes) {
	if (buffer == NULL) {
		printf("ERR: Buffer is NULL!\n");
		return -1;
	}

	if (bufferSizeInBytes <= 0) {
		printf("ERR: Invalid buffer size specified!\n");
		return -1;
	}

	pthread_mutex_init(&data_generator_mutex, NULL);
	pthread_mutex_lock(&data_generator_mutex);

	int bytes_written;
	bytes_written = data_pkt_generator(buffer, bufferSizeInBytes);

	pthread_mutex_unlock(&data_generator_mutex);
	pthread_mutex_destroy(&data_generator_mutex);

	return bytes_written;
}

int process_data(char *buffer, int bufferSizeInBytes) {
	if (buffer == NULL) {
		printf("ERR: Buffer is NULL!\n");
		return -1;
	}

	if (bufferSizeInBytes <= 0) {
		printf("ERR: Invalid buffer size specified!\n");
		return -1;
	}

	printf("Pseudo process reading data\n");
	for (int i = 0; i < bufferSizeInBytes; i++) {
		printf("[%s][line:%d][%s]: buffer[%d]=%d\n", __FILE__, __LINE__, __func__, i, buffer[i]);
	}

	return EXIT_SUCCESS;
}


/* handle system signal such as Ctrl + C */
void sig_handler(int signum) {
    if (signum != SIGINT) {
        printf("Received invalid signum = %d in sig_handler()\n", signum);
        assert(signum == SIGINT);
    }

    printf("Received SIGINT %d. Exiting Application\n", signum);

//    pthread_cancel(thread1);
//    pthread_cancel(thread2);

    exit(0);
}


/**
 * This thread is responsible for pulling data off of the shared data
 * area and processing it using the process_data() API.
 */
void *reader_thread(void *arg) {
	//TODO: Define set-up required

	pthread_t thread_id = pthread_self();
	while(1) {

		/* obtain read/write lock for reader to lock out other writers */
		int try_read_lock_status = pthread_rwlock_tryrdlock(&rw_lock_mutex);
		printf("reader {tid=%d} try_read_lock_status=%d\n",thread_id, try_read_lock_status);
		if (!try_read_lock_status) {
			printf ("***** successfully obtained read/write lock for reader tid=%d\n", thread_id);
		}
		else {
			printf ("!!!!! failed to obtain read/write lock for reader %d\n", thread_id);
		}

		printf("Obtained lock, reader {tid=%d} starts reading operations\n", thread_id);

		//TODO: Define data extraction (queue) and processing
		process_data(message_buffer.data_pkt, message_buffer.buf_size);

		// unlock reader/writer lock
		printf("Reader {tid=%d} is done, releasing rw lock\n", thread_id);
		pthread_rwlock_unlock(&rw_lock_mutex);
		usleep(1);
	}

	return NULL;
}


/**
 * This thread is responsible for pulling data from a device using
 * the get_external_data() API and placing it into a shared area
 * for later processing by one of the reader threads.
 */
void *writer_thread(void *arg) {
	//TODO: Define set-up required
	time_t timeofday;
	unsigned int bytes_written;

	pthread_t thread_id = pthread_self();
	/* Initialize random number generator */
	srand((unsigned) time(&timeofday));

	while(1) {

		/* obtain read/write lock to lock out other writers as well as readers */
		int write_lock_status = pthread_rwlock_wrlock(&rw_lock_mutex);
		printf("writer {tid=%d} write_lock_status=%d\n",thread_id, write_lock_status);
		if (!write_lock_status) {
			printf ("***** successfully obtained read/write lock for writer tid=%d\n", thread_id);
		}
		else {
			printf ("!!!!! failed to obtain read/write lock for writer %d\n", thread_id);
		}

		//TODO: Define data extraction (device) and storage

		printf("Obtained lock, writer {tid=%d} starts writing operations\n", thread_id);
		/* data storage */
		bytes_written = get_external_data(message_buffer.data_pkt, message_buffer.buf_size);
		printf("structure bytes_written=%d\n", bytes_written);

		/* unlock reader/writer lock */
		printf("Writer {tid=%d} is done, releasing rw lock\n", thread_id);
		pthread_rwlock_unlock(&rw_lock_mutex);
		usleep(1);
	}

	return NULL;
}

// original requirement
//#define M 10
//#define N 20

#define M 2 // writer
#define N 3 // reader
int main(int argc, char **argv) {
	unsigned int i;
	int ret;

	pthread_t wr_thread_tid[M];
	pthread_t rd_thread_tid[N];

	signal(SIGINT, sig_handler);

	/* initialize read/write lock */
	int lock_status = pthread_rwlock_init (&rw_lock_mutex, NULL);
	printf("initiate lock status = %d\n", lock_status);

	/* initialize and allocate shared buffer */
//	message_buffer.buf_size = 1024;  // 1M buffer
	message_buffer.buf_size = 16;  // 16B buffer
	message_buffer.data_pkt = (char*) calloc(message_buffer.buf_size, sizeof(char));

	/* create reader threads */
	for(i = 0; i < N; i++) {
		ret = pthread_create(&rd_thread_tid[i], NULL, reader_thread, NULL);
		if(ret) {
			printf("!!!!! Failed to create reader thread tid=%d\n", rd_thread_tid[i]);
			return EXIT_FAILURE;
		}
		printf("***** successfully created reader thread tid=%d\n", rd_thread_tid[i]);
	}

	/* create writer threads */
	for(i = 0; i < M; i++) {
		ret = pthread_create(&wr_thread_tid[i], NULL, writer_thread, NULL);
		if(ret) {
			printf("!!!!! Failed to create writer thread tid=%d\n", wr_thread_tid[i]);
			return EXIT_FAILURE;
		}
		printf("***** successfully created writer thread tid=%d\n", wr_thread_tid[i]);
	}

	/* wait until all threads complete */
	for (i = 0; i < N; i ++) {
		pthread_join(rd_thread_tid[i], NULL);
	}
	for (i = 0; i < M; i ++) {
		pthread_join(wr_thread_tid[i], NULL);
	}

	pthread_rwlock_destroy(&rw_lock_mutex);

	return EXIT_SUCCESS;
}
