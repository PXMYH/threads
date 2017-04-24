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

#endif /* INCLUDE_UTILS_H_ */
