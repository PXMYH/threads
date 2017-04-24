/*
 * utils.h
 *
 */

#ifndef INCLUDE_UTILS_H_
#define INCLUDE_UTILS_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <semaphore.h>

/*
 * function: random_data_pkt_generator
 * generate random number of random sized data packets
 * return if random number generation success or not
 * */
int random_data_pkt_generator(char* data_pkt_buf, unsigned int * buffer_size_in_bytes);

/*
 * function: data_pkt_generator
 * generate fixed size data packet
 * return size of generated packet in Byte
 * */
int data_pkt_generator (char* data, unsigned int buffer_size_in_bytes);

/* smq_msg_destroy options */
#define SMQ_CONTAINER_ONLY      0
#define SMQ_DESTROY_ALL         1


typedef struct _smq *            smq;

struct smq_msg {
	void                    *data;
	size_t                   data_len;
};

smq              smq_create(void);
int              smq_send(smq, struct smq_msg *);
struct smq_msg  *smq_receive(smq);
size_t           smq_len(smq);
void             smq_settimeout(smq, struct timeval *);
int              smq_dup(smq);
int              smq_destroy(smq);

struct smq_msg  *smq_msg_create(void *, size_t);
int              smq_msg_destroy(struct smq_msg *, int);

#endif /* INCLUDE_UTILS_H_ */
