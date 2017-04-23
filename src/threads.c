#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <mqueue.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

//TODO Define global data structures to be used
// TODO: MH use unsigned integers
//int number_of_writers = 0;
//int number_of_readers = 0;

pthread_rwlock_t rw_lock_mutex;

mqd_t message_queue_descriptor;
#define MY_MQ_NAME "/default_message_queue"

unsigned int counter;

/* handle system signal such as Ctrl + C */
void sig_handler(int signum) {
    if (signum != SIGINT) {
        printf("Received invalid signum = %d in sig_handler()\n", signum);
        assert(signum == SIGINT);
    }

    printf("Received SIGINT. Exiting Application\n");

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
	int status;
	int recv_counter;
	status = mq_receive(message_queue_descriptor, (char*)&recv_counter, sizeof(recv_counter), NULL);
	if (status > 0) {
		printf("RECVd MSG in reader_thread: %d\n", recv_counter);
		counter++;
	}
//	while(1) {
//		//TODO: Define data extraction (queue) and processing
//	}

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
	int status;
	status = mq_send(message_queue_descriptor, (const char*)&counter, sizeof(counter), 1);
	assert(status != -1);
//	while(1) {
//		//TODO: Define data extraction (device) and storage
//	}

	return NULL;
}


#define M 10
#define N 20
int main(int argc, char **argv) {
	unsigned int i;
	pthread_t thread_tid[N+M];

	counter = 0;

	/* initialize read/write lock */
	int lock_status = pthread_rwlock_init (&rw_lock_mutex, NULL);
	printf("initiate lock status = %d\n", lock_status);

	/* initialize message queue */
	// TODO: MH change message queue attributes
	message_queue_descriptor = mq_open(MY_MQ_NAME, O_RDWR | O_CREAT | O_NONBLOCK, 0755, NULL);
	if (message_queue_descriptor == -1) {
		printf("creating message queue status code: %d, errno=%s\n", message_queue_descriptor, strerror(errno));
		perror("mq_open");
		exit(EXIT_FAILURE);
	} else {

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
	}

	return 0;
}