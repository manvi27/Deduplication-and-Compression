#include "./Server/encoder.h"
#include "Host.h"

#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "./Server/server.h"
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "stopwatch.h"
#include <thread>

#define PIPELINE_STAGES 4

int offset = 0;
unsigned char* file;

void handle_input(int argc, char* argv[], int* blocksize) {
	int x;
	extern char *optarg;

	while ((x = getopt(argc, argv, ":b:")) != -1) {
		switch (x) {
		case 'b':
			*blocksize = atoi(optarg);
			printf("blocksize is set to %d optarg\n", *blocksize);
			break;
		case ':':
			printf("-%c without parameter\n", optopt);
			break;
		}
	}
}

void pin_thread_to_cpu(std::thread &t, int cpu_num);
void pin_main_thread_to_cpu0();

void Core1 () {

}

void Core2 () {

}

void Core3 () {

}

int main(int argc, char* argv[]) {
	stopwatch ethernet_timer;
	unsigned char* input[10];
	int packets = 0;
	int done = 0;
	int length = 0;
	int count = 0;
	int total_length = 0;
	ESE532_Server serverObj;
	float runtime = 0;
	int bytes_dropped = 0;
	float kernel_time = 0;
	float non_lzw = 0;

    pin_main_thread_to_cpu0();

    core_1_thread = std::thread(&Core1);
    pin_thread_to_cpu(core_1_thread, 1);

    core_2_thread = std::thread(&Core2);
    pin_thread_to_cpu(core_2_thread, 1);

    core_3_thread = std::thread(&Core3);
    pin_thread_to_cpu(core_3_thread, 1);

	if (argc != 2) {
		printf("Usage: ./encoder <output_filename>\n");
		return -1;
	}

	int blocksize = BLOCKSIZE;

	handle_input(argc, argv, &blocksize);

	file = (unsigned char*) malloc(sizeof(unsigned char) * 70000000);
	if (file == NULL) {
		printf("Error file size\n");
	}

	for (int i = 0; i < 10; i++) {
		input[i] = (unsigned char*) malloc(
				sizeof(unsigned char) * (NUM_ELEMENTS + HEADER));
		if (input[i] == NULL) {
			std::cout << "aborting " << std::endl;
			return 1;
		}
	}

	serverObj.setup_server(blocksize);

	packets = PIPELINE_STAGES;
	serverObj.get_packet(input[packets]);
	count++;
    unsigned char* buffer = input[packets];
    done = buffer[1] & (1<<7));
	length = buffer[0] | (buffer[1] << 8);
	length &= ~(1 << 15);
	total_length += length;
	
	// Call implementation, need to move it to separate function

    packets++;

	while (!done) {
		// reset ring buffer
		if (packets == 10) {
			packets = 0;
		}

		serverObj.get_packet(input[packets]);
        count++;
        unsigned char* buffer = input[packets];

		done = buffer[1] & (1<<7);
		length = buffer[0] | (buffer[1] << 8);
		length &= ~(1 << 15);
		total_length += length;
		
        // Call implementation, need to move it to separate function

		packets++;
	}

	// write file to root and you can use diff tool on board
	FILE *outfd = fopen(argv[1], "wb");
	int bytes_written = fwrite(&file[0], 1, offset, outfd);
	printf("write file with %d\n", bytes_written);
	fclose(outfd);

	for (int i = 0; i < 10; i++) {
		free(input[i]);
	}

	free(file);

	return 0;
}