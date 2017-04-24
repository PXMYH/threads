#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include "threads.h"
#include "utils.h"

// steps:
// thread synchronization                                                DONE
// shared buffer - integer                                               DONE
// get external data - fixed size                                        DONE
// process data - fixed size                                             DONE
// get external data - various size (pointer + calloc)                   REQUIRE API PARAM CHANGE (buffer_size_in_bytes)
// process data - various size                                           REQUIRE API PARAM CHANGE (buffer_size_in_bytes)
// add more information to data packet structure for better tracking     DONE
// thread synchronization optimization                                   DONE
// message queue for passing data


// Define global data structures to be used
// original requirement
#define M 10
#define N 20

// Testing
//#define M 2 // writer
//#define N 3 // reader

#define DEBUG_MODE

pthread_t wr_thread_tid[M];
pthread_t rd_thread_tid[N];

#define TIMESTAMP_LEN 40
struct MSG_BUF {
	char *data_pkt;
	unsigned int buf_size;
	char last_update_timestamp[TIMESTAMP_LEN];
} message_buffer;

pthread_rwlock_t rw_lock_mutex;
pthread_mutex_t data_generator_mutex = PTHREAD_MUTEX_INITIALIZER;

/* function: sig_handler
 * handle system signal such as Ctrl + C
 * */
void sig_handler(int signum) {
    if (signum != SIGINT) {
        printf("Received invalid signum = %d in sig_handler()\n", signum);
        assert(signum == SIGINT);
    }

    printf("Received SIGINT %d. Exiting Application\n", signum);

	for (unsigned int i = 0; i < N; i ++) {
		pthread_cancel(rd_thread_tid[i]);
	}
	for (unsigned i = 0; i < M; i ++) {
		pthread_cancel(wr_thread_tid[i]);
	}

    exit(signum);
}

static int get_external_data(char *buffer, int bufferSizeInBytes) {
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

static int process_data(char *buffer, int bufferSizeInBytes) {
	if (buffer == NULL) {
		printf("ERR: Buffer is NULL!\n");
		return -1;
	}

	if (bufferSizeInBytes <= 0) {
		printf("ERR: Invalid buffer size specified!\n");
		return -1;
	}

	pthread_t thread_id = pthread_self();
#ifdef DEBUG_MODE
	printf("Pseudo process reading data thread tid=%d\n", thread_id);
#endif
	for (int i = 0; i < bufferSizeInBytes; i++) {
		printf("[%s][line:%d][%s]: tid=%d buffer[%d]=%d\n", __FILE__, __LINE__, __func__, thread_id, i, buffer[i]);
	}

	return EXIT_SUCCESS;
}

/**
 * This thread is responsible for pulling data off of the shared data
 * area and processing it using the process_data() API.
 */
void *reader_thread(void *arg) {
	pthread_t thread_id = pthread_self();
	while(1) {

		/* obtain read/write lock for reader to lock out other writers */
		int try_read_lock_status = pthread_rwlock_tryrdlock(&rw_lock_mutex);
#ifdef DEBUG_MODE
		printf("reader {tid=%d} try_read_lock_status=%d\n",thread_id, try_read_lock_status);

		if (!try_read_lock_status) {
			printf ("***** successfully obtained read/write lock for reader tid=%d\n", thread_id);
		}
		else {
			printf ("!!!!! failed to obtain read/write lock for reader %d\n", thread_id);
		}

		printf("Obtained lock, reader {tid=%d} starts reading operations\n", thread_id);
#endif
		process_data(message_buffer.data_pkt, message_buffer.buf_size);

		/* unlock reader/writer lock */
#ifdef DEBUG_MODE
		printf("Reader {tid=%d} is done, releasing rw lock\n", thread_id);
#endif
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
	// Define set-up required
	time_t timeofday;
	unsigned int bytes_written;
	pthread_t thread_id = pthread_self();

	/* Initialize random number generator */
	srand((unsigned) time(&timeofday));

	while(1) {

		/* obtain read/write lock to lock out other writers as well as readers */
		int write_lock_status = pthread_rwlock_wrlock(&rw_lock_mutex);
#ifdef DEBUG_MODE
		printf("writer {tid=%d} write_lock_status=%d\n",thread_id, write_lock_status);
		if (!write_lock_status) {
			printf ("***** successfully obtained read/write lock for writer tid=%d\n", thread_id);
		}
		else {
			printf ("!!!!! failed to obtain read/write lock for writer %d\n", thread_id);
		}

		// Define data extraction (device) and storage

		printf("Obtained lock, writer {tid=%d} starts writing operations\n", thread_id);
#endif
		/* data storage */
		bytes_written = get_external_data(message_buffer.data_pkt, message_buffer.buf_size);
#ifdef DEBUG_MODE
		printf("bytes_written=%d\n", bytes_written);
#endif

		/* update timestamp */
		strncpy(message_buffer.last_update_timestamp, asctime(localtime (&timeofday)), TIMESTAMP_LEN);

		/* unlock reader/writer lock */
#ifdef DEBUG_MODE
		printf("Writer {tid=%d} is done, releasing rw lock\n", thread_id);
#endif
		pthread_rwlock_unlock(&rw_lock_mutex);
		usleep(1);
	}

	return NULL;
}

int main(int argc, char **argv) {
	unsigned int i;
	int ret;

	pthread_rwlockattr_t rwlock_attr;
	pthread_attr_t thread_attr;

	signal(SIGINT, sig_handler);

	/* initialize read/write lock with private mode and monotonic clock */
	pthread_rwlockattr_init(&rwlock_attr);
	pthread_rwlockattr_setpshared(&rwlock_attr, PTHREAD_PROCESS_PRIVATE);
	pthread_rwlockattr_setclock(&rwlock_attr, CLOCK_MONOTONIC);
	int lock_status = pthread_rwlock_init (&rw_lock_mutex, &rwlock_attr);
	printf("initiate lock status = %d\n", lock_status);

	/* initialize and allocate shared buffer */
//	message_buffer.buf_size = 1024;  // 1M buffer
	message_buffer.buf_size = 16;  // 16B buffer
	message_buffer.data_pkt = (char*) calloc(message_buffer.buf_size, sizeof(char));

	pthread_attr_init(&thread_attr);

	/* create reader threads */
	for(i = 0; i < N; i++) {
		ret = pthread_create(&rd_thread_tid[i], &thread_attr, reader_thread, NULL);
		if(ret) {
			printf("!!!!! Failed to create reader thread tid=%d\n", rd_thread_tid[i]);
			return EXIT_FAILURE;
		}
		printf("***** successfully created reader thread tid=%d\n", rd_thread_tid[i]);
	}

	/* create writer threads */
	for(i = 0; i < M; i++) {
		ret = pthread_create(&wr_thread_tid[i], &thread_attr, writer_thread, NULL);
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

	sig_handler(SIGINT);

	return EXIT_SUCCESS;
}
