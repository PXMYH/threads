/*
 * utils.c
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <semaphore.h>
#include "utils.h"

/*
 * function: random_data_pkt_generator
 * generate random number of random sized data packets
 * */
int random_data_pkt_generator(char* data_pkt_buf, unsigned int * buffer_size_in_bytes) {
	time_t timeofday;
	srand((unsigned) time(&timeofday));

	// generate size of packet, from instruction "from a few bytes to a few kilobytes."
	// [M, N] = [1, 2000] i.e. 1 Byte to 2kB
	*buffer_size_in_bytes = 1 + rand() / (RAND_MAX / 2000  + 1);
	printf("generated packet size = %d", *buffer_size_in_bytes);

	// generate actual packet
	for (unsigned int i = 0; i < *buffer_size_in_bytes; i++) {
		data_pkt_buf[i] = (char) rand () % 256;
	}

	return EXIT_SUCCESS;
}

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
		printf("[%s][line:%d][%s]: buffer[%d]=%d\n", __FILE__, __LINE__, __func__, i, data[i]);
	}
	return buffer_size_in_bytes;
}




struct smq_entry {
	void                    *data;
	size_t                   data_len;
	TAILQ_ENTRY(smq_entry)   entries;
};
TAILQ_HEAD(tq_msg, smq_entry);

struct _smq {
	struct tq_msg           *queue;
	size_t                   queue_len;
	sem_t                   *sem;
	struct timeval           timeo;
	size_t                   refs;
};


static int lock_queue(smq);
static int unlock_queue(smq);
static int queue_locked(smq);
static struct smq_msg *smq_entry_to_msg(struct smq_entry *);
static struct smq_entry *msg_to_smq_entry(struct smq_msg *);


/*
 * smq_create Initializes and returns a new, empty message queue.
 */
smq
smq_create()
{
	smq queue;
	int sem_error = -1;

	queue = NULL;
	queue = (smq)malloc(sizeof(struct _smq));
	if (NULL == queue)
		return NULL;
        if (NULL == (queue->sem = (sem_t *)malloc(sizeof(sem_t))))
                return NULL;
	queue->queue = (struct tq_msg *)malloc(sizeof(struct tq_msg));
	if (NULL != queue->queue) {
		TAILQ_INIT(queue->queue);
		queue->queue_len = 0;
                queue->refs = 1;
                queue->timeo.tv_sec = 0;
                queue->timeo.tv_usec = 10 * 1000;
		sem_error = sem_init(queue->sem, 0, 0);
                if (0 == sem_error) {
                        sem_error = unlock_queue(queue);
                }
	}

	if (sem_error) {
		free(queue->queue);
		free(queue);
		return NULL;
	}

	return queue;
}


/*
 * smq_send adds a new message to the queue. The message structure will
 * be freed if the message was successfully added to the queue.
 */
int smq_send(smq queue, struct smq_msg *message) {
	struct smq_entry        *entry;
	int                      retval;

	if (queue == NULL) {
		printf("queue is NULL\n");
		return -1;
	}

	entry = msg_to_smq_entry(message);
	if (entry == NULL) {
		printf("entry is NULL\n");
		return -1;
        } else if (entry->data == NULL) {
        	printf("data is NULL\n");
			return -1;
        }

	retval = lock_queue(queue);
	if (retval == 0) {
		TAILQ_INSERT_TAIL(queue->queue, entry, entries);
		queue->queue_len++;
		free(message);
		return unlock_queue(queue);
	}
	printf("lock queue failed\n");
	return -1;
}


/*
 * smq_receive retrieves the next message from the queue.
 */
struct smq_msg *
smq_receive(smq queue)
{
	struct smq_msg *message = NULL;
	struct smq_entry *entry = NULL;

	if (queue == NULL) {
		printf("QUEUE is NULL\n");
		return NULL;
	}

	if (lock_queue(queue) == 0) {
		printf("lock queue successful\n");
		entry = TAILQ_FIRST(queue->queue);
		if (entry != NULL) {
			printf("decoding message queue\n");
			message = smq_entry_to_msg(entry);
			TAILQ_REMOVE(queue->queue, entry, entries);
			free(entry);
			unlock_queue(queue);
			queue->queue_len--;
		} else {
			printf("entry is NULL\n");
		}
	}

	if (queue_locked(queue))
		unlock_queue(queue);

	return message;
}


/*
 * smq_destroy carries out the proper destruction of a message queue. All
 * messages are fully destroyed.
 */
int
smq_destroy(smq queue)
{
	struct smq_entry *entry;
	int retval;

        if (NULL == queue)
                return 0;

	if (0 != (retval = (lock_queue(queue))))
		return retval;

        queue->refs--;

        if (queue->refs > 0)
                return unlock_queue(queue);

	while (NULL != (entry = TAILQ_FIRST(queue->queue))) {
		free(entry->data);
		TAILQ_REMOVE(queue->queue, entry, entries);
		free(entry);
	}

	free(queue->queue);
        while (unlock_queue(queue)) ;
        retval = sem_destroy(queue->sem);
        free(queue->sem);
	free(queue);
	return retval;
}


/*
 * smq_len returns the number of messages in the queue.
 */
size_t
smq_len(smq queue)
{
	if (NULL == queue) {
		return 0;
	} else {
		return queue->queue_len;
	}
}


/*
 * smq_settimeout sets the queue's timeout to the new timeval.
 */
void
smq_settimeout(smq queue, struct timeval *timeo)
{
        if (NULL == timeo)
                return;
        queue->timeo.tv_sec = timeo->tv_sec;
        queue->timeo.tv_usec = timeo->tv_usec;
        return;
}


/*
 * duplicate a message queue for use in another thread.
 */
int smq_dup(smq queue) {
        if (queue == NULL)
			return -1;
        if (lock_queue(queue)) {
			return -1;
        }
        queue->refs++;
        while (queue_locked(queue))
			unlock_queue(queue);
        return 0;
}


/*
 * msg_create creates a new message structure for passing into a message queue.
 */
struct smq_msg *
smq_msg_create(void *message_data, size_t message_len)
{
	struct smq_msg *message;

        if (NULL == message_data || message_len == 0)
                return NULL;

	message = (struct smq_msg *)malloc(sizeof(struct smq_msg));
	if (NULL == message) {
		return NULL;
	}
	message->data = message_data;
	message->data_len = message_len;
	return message;
}


/*
 * msg_destroy cleans up a message. If the integer parameter != 0, it will
 * free the data stored in the message.
 */
int
smq_msg_destroy(struct smq_msg *message, int opts)
{
	if (NULL != message && SMQ_CONTAINER_ONLY != opts)
		free(message->data);
	free(message);
	return 0;
}


/*
 * smq_entry_to_msg takes a smq_entry and returns a struct smq_msg *from it.
 */
struct smq_msg *
smq_entry_to_msg(struct smq_entry *entry)
{
	struct smq_msg *message;

        if (NULL == entry)
                return NULL;
	message = (struct smq_msg *)malloc(sizeof(struct smq_msg));
	if (NULL == message)
		return message;
	message->data = entry->data;
	message->data_len = entry->data_len;
	return message;
}


/*
 * msg_to_smq_entry converts a struct smq_msg *to a smq_entry.
 */
struct smq_entry *
msg_to_smq_entry(struct smq_msg *message)
{
	struct smq_entry *entry;

        if (NULL == message)
                return NULL;
        if (NULL == message->data || 0 == message->data_len)
                return NULL;
	entry = (struct smq_entry *)malloc(sizeof(struct smq_entry));
	if (NULL == entry)
		return entry;
	entry->data = message->data;
	entry->data_len = message->data_len;
	return entry;
}


/*
 * attempt to acquire the lock for the message queue. If doblock is set
 * to 1, lock_queue will block until the mutex is locked.
 */
int
lock_queue(smq queue)
{
        struct timeval   ts;
        int              retval;

        ts.tv_sec = queue->timeo.tv_sec;
        ts.tv_usec = queue->timeo.tv_usec;

        retval = sem_trywait(queue->sem);
        if (-1 == retval) {
                select(0, NULL, NULL, NULL, &ts);
                return sem_trywait(queue->sem);
        }
        return retval;
}


/*
 * release the lock on a queue. only unlocks if queue is locked.
 */
int
unlock_queue(smq queue)
{
        if (queue_locked(queue))
	        return sem_post(queue->sem);
        return 0;
}


/*
 * returns 1 if the queue is locked, and 0 if it is unlocked.
 */
static int
queue_locked(smq queue)
{
        int     retval;
        int     semval;

        retval = sem_getvalue(queue->sem, &semval);
        if (-1 == retval)
                return retval;
        return semval == 0;
}
