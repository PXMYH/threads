#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

//TODO Define global data structures to be used
// TODO: MH use unsigned integers
//int number_of_writers = 0;
//int number_of_readers = 0;

pthread_rwlock_t rw_lock_mutex;
unsigned int counter;

# define packet_size 64
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
	struct timeval ts;

	pthread_t thread_id = pthread_self();

	/* obtain read/write lock for reader to lock out other writers */
	int try_read_lock_status = pthread_rwlock_tryrdlock(&rw_lock_mutex);
	printf("try_read_lock_status=%d\n",try_read_lock_status);
	if (!try_read_lock_status) {
		printf ("successfully obtained read/write lock for reader\n");
	}
	else {
		printf ("failed to obtain read/write lock for reader\n");
	}


	printf("Reading Operations\n");
	while(1) {
		//TODO: Define data extraction (queue) and processing
        gettimeofday(&ts, NULL);
        printf("%06lu.%06lu: --- reader_thread %d run_cnt = %d\n",\
                                           (unsigned int)ts.tv_sec,\
                                           (unsigned int)ts.tv_usec,\
										   thread_id, run_cnt);
        run_cnt += 1;
//		printf("currently message buffer content: %d\n", message_buffer.packet_content);
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
	struct timeval ts;

	pthread_t thread_id = pthread_self();

	/* obtain read/write lock to lock out other writers as well as readers */
	int try_write_lock_status = pthread_rwlock_trywrlock(&rw_lock_mutex);
	printf("try_write_lock_status=%d\n",try_write_lock_status);
	if (!try_write_lock_status) {
		printf ("successfully obtained read/write lock for writer\n");
	}
	else {
		printf ("failed to obtain read/write lock for writer\n");
	}


	/* write data operations */
	printf("Writing Operations\n");
	while(1) {
		//TODO: Define data extraction (device) and storage

		// data storage
        gettimeofday(&ts, NULL);
        printf("%06lu.%06lu: --- XXXXXX --- writer_thread %d run_cnt = %d --- XXXXXX ---\n",\
                                           (unsigned int)ts.tv_sec,\
                                           (unsigned int)ts.tv_usec,\
										   thread_id, run_cnt);
        run_cnt ++;
//		message_buffer.packet_content = d;
	}

	return NULL;
}


#define M 10
#define N 20
int main(int argc, char **argv) {
	unsigned int i;
	pthread_t thread_tid[N+M];

	counter = 0;

	signal(SIGINT, sig_handler);

	/* initialize read/write lock */
	int lock_status = pthread_rwlock_init (&rw_lock_mutex, NULL);
	printf("initiate lock status = %d\n", lock_status);



	/* create reader threads */
	for(i = 0; i < N; i++) {
		pthread_create(&thread_tid[i], NULL, reader_thread, NULL);
		printf("tid of Reader %d = %d\n", i+1, thread_tid[i]);
	}

	/* create writer threads */
	for(i = 0; i < M; i++) {
		pthread_create(&thread_tid[N+i], NULL, writer_thread, NULL);
		printf("tid of Writer %d = %d\n", i+1, thread_tid[N+i]);
	}

	/* wait until all threads complete */
	for (i = 0; i < M+N; i ++) {
		pthread_join(thread_tid[i], NULL);
	}

	/* release read/write lock */
	pthread_rwlock_unlock (&rw_lock_mutex);

	return 0;
}
