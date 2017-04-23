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

//TODO Define global data structures to be used
// TODO: MH use unsigned integers
//int number_of_writers = 0;
//int number_of_readers = 0;

pthread_rwlock_t rw_lock_mutex;
unsigned int counter;

# define packet_size 2
struct MSG_BUF {
	int packet_content[packet_size];
} message_buffer;


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
	unsigned int run_cnt = 0;

	pthread_t thread_id = pthread_self();
	while(1) {

		/* obtain read/write lock for reader to lock out other writers */
		int try_read_lock_status = pthread_rwlock_tryrdlock(&rw_lock_mutex);
		printf("try_read_lock_status=%d\n",try_read_lock_status);
		if (!try_read_lock_status) {
			printf ("***** successfully obtained read/write lock for reader tid=%d\n", thread_id);
		}
		else {
			printf ("***** failed to obtain read/write lock for reader %d\n", thread_id);
		}

		printf("Obtained lock, reader {tid=%d} starts reading operations\n", thread_id);

		//TODO: Define data extraction (queue) and processing
		for (int i = 0; i< packet_size; i++) {
			printf("reader_thread {tid=%d} buffer[%d] = %d\n",thread_id, i, message_buffer.packet_content[i]);
		}
		run_cnt++;

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
/*
 * Writer thread synchronization
 * - lock out other writers while writing
 * - lock out other readers, prevent reader thread from reading data structure
 * - lock data structure while writing, protecting data integrity
 */
void *writer_thread(void *arg) {
	//TODO: Define set-up required
	unsigned int run_cnt = 0;
	time_t timeofday;

	pthread_t thread_id = pthread_self();
	/* Initialize random number generator */
	srand((unsigned) time(&timeofday));

	while(1) {

		/* obtain read/write lock to lock out other writers as well as readers */
		int write_lock_status = pthread_rwlock_wrlock(&rw_lock_mutex);
		printf("try_write_lock_status=%d\n",write_lock_status);
		if (!write_lock_status) {
			printf ("***** successfully obtained read/write lock for writer tid=%d\n", thread_id);
		}
		else {
			printf ("***** failed to obtain read/write lock for writer %d\n", thread_id);
		}

		/* write data operations */
		printf("Obtained lock, writer {tid=%d} starts writing operations\n", thread_id);


		//TODO: Define data extraction (device) and storage

		// data storage
		int buffer_idx;
		for (buffer_idx = 0; buffer_idx < packet_size; buffer_idx++) {
			message_buffer.packet_content[buffer_idx] = rand();
			printf("writing in progress: writer_thread {%d} buffer[%d] = %d\n",thread_id, buffer_idx, message_buffer.packet_content[buffer_idx]);
		}
		run_cnt ++;

		// unlock reader/writer lock
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
	pthread_t wr_thread_tid[M];
	pthread_t rd_thread_tid[N];

	counter = 0;

	signal(SIGINT, sig_handler);

	/* initialize read/write lock */
	int lock_status = pthread_rwlock_init (&rw_lock_mutex, NULL);
	printf("initiate lock status = %d\n", lock_status);

	/* create reader threads */
	for(i = 0; i < N; i++) {
		pthread_create(&rd_thread_tid[i], NULL, reader_thread, NULL);
		printf("tid of Reader %d = %d\n", i+1, rd_thread_tid[i]);
	}

	/* create writer threads */
	for(i = 0; i < M; i++) {
		pthread_create(&wr_thread_tid[N+i], NULL, writer_thread, NULL);
		printf("tid of Writer %d = %d\n", i+1, wr_thread_tid[N+i]);
	}



	/* wait until all threads complete */
	for (i = 0; i < N; i ++) {
		pthread_join(rd_thread_tid[i], NULL);
	}
	for (i = 0; i < M; i ++) {
		pthread_join(wr_thread_tid[i], NULL);
	}

	pthread_rwlock_destroy(&rw_lock_mutex);

	return 0;
}
