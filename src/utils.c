/*
 * utils.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
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
