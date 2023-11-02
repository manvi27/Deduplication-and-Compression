#include "encoder.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "server.h"
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "stopwatch.h"



//Check
#include <math.h>
#include <time.h>
#include <vector>
#include "SHA256.h"
using namespace std;
#define WIN_SIZE 16
#define PRIME 3
#define MODULUS 256
#define TARGET 0

#define usr_code

#ifdef usr_code
uint64_t hash_func(unsigned char *input, unsigned int pos)
{
	// put your hash function implementation here
	/*
	hash = 0
    for i in range(0, win_size):
        hash += ord(input[pos+win_size-1-i])*(pow(prime, i+1))
    return hash
	*/
    uint64_t hash = 0;
	uint8_t i = 0;
	for(i = 0; i < WIN_SIZE; i++)
	{
		hash += input[pos+WIN_SIZE-1-i]*(pow(PRIME,i+1));
	}
	return hash;
}

void cdc(vector<unsigned int> &ChunkBoundary, unsigned char *buff, unsigned int buff_size)
{
	// put your cdc implementation here
    uint64_t i;
	unsigned int ChunkCount = 0;
	for(i = WIN_SIZE; i < buff_size - WIN_SIZE; i++)
	{
		if((hash_func(buff,i)%MODULUS) == 0)
        {
			printf("%ld\n",i);
			ChunkBoundary[ChunkCount++] = i;
		}
	} 
}

int Deduplicate(std::vector<std::array<uint8_t,32>> &ChunkHashTable,std::array<uint8_t,32> hash)
{
    
	/*Brute force approach: false for a match and true for unique chunk encountered*/
	
	for(int ChunkIdx = 0;ChunkIdx < ChunkHashTable.size(); ChunkIdx++)
	{
		if(hash == ChunkHashTable[ChunkIdx])
		{
			return ChunkIdx;
		}
	}
    ChunkHashTable.push_back(hash);
	return -1;
}

void ComputeLZW(std::vector<unsigned int> &ChunkLZW, uint8_t *Chunk,uint16_t ChunkLength)
{
    
}

#endif

//Original
#define NUM_PACKETS 8
#define pipe_depth 4
#define DONE_BIT_L (1 << 7)
#define DONE_BIT_H (1 << 15)

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

int main(int argc, char* argv[]) {
	stopwatch ethernet_timer;
	unsigned char* input[NUM_PACKETS];
	int writer = 0;
	int done = 0;
	int length = 0;
	int count = 0;
	ESE532_Server server;

	// default is 2k
	int blocksize = BLOCKSIZE;

	// set blocksize if decalred through command line
	handle_input(argc, argv, &blocksize);

	file = (unsigned char*) malloc(sizeof(unsigned char) * 70000000);
	if (file == NULL) {
		printf("help\n");
	}

	for (int i = 0; i < NUM_PACKETS; i++) {
		input[i] = (unsigned char*) malloc(
				sizeof(unsigned char) * (NUM_ELEMENTS + HEADER));
		if (input[i] == NULL) {
			std::cout << "aborting " << std::endl;
			return 1;
		}
	}

	
	server.setup_server(blocksize);

	writer = pipe_depth;
	server.get_packet(input[writer]);
	count++;

	// get packet
	unsigned char* buffer = input[writer];

	// decode
#ifdef usr_code
	vector<unsigned int> ChunkBoundary[NUM_PACKETS];/*Stores chunk Id for NUM_PACKETS*/
	std::vector<std::array<uint8_t, 32>> ChunkhashTbl;
	/*Compute chunk boundaries on block rcvd*/
	cdc(ChunkBoundary[writer],buffer,blocksize);
    /*Perform SHA on subsequent chunks*/
	for(int chunkIdx = 0;chunkIdx < ChunkBoundary[writer].size();chunkIdx++)
	{
	   SHA256 sha;
	   int IsChunkUnique = -1;
	   std::vector<unsigned int> ChunkLZW;
	   sha.update(&buffer[ChunkBoundary[writer][chunkIdx]],ChunkBoundary[writer][chunkIdx + 1] - ChunkBoundary[writer][chunkIdx]);
	   std::array<uint8_t,32> hash = sha.digest();
       /*Deduplicate using look-up for SHA 256*/
       IsChunkUnique = Deduplicate(ChunkhashTbl, hash);
	   /*If IsChunkUnique == true, compute LZW else send Chunk Id */
       if(true == IsChunkUnique)
	   {
		  ComputeLZW(ChunkLZW,&buffer[ChunkBoundary[writer][chunkIdx]],ChunkBoundary[writer][chunkIdx + 1] - ChunkBoundary[writer][chunkIdx]);
	   }
	}

#endif

	done = buffer[1] & DONE_BIT_L;
	length = buffer[0] | (buffer[1] << 8);
	length &= ~DONE_BIT_H;
	// printing takes time so be weary of transfer rate
	//printf("length: %d offset %d\n",length,offset);

	// we are just memcpy'ing here, but you should call your
	// top function here.
	memcpy(&file[offset], &buffer[HEADER], length);

	offset += length;
	writer++;

	//last message
	while (!done) {
		// reset ring buffer
		if (writer == NUM_PACKETS) {
			writer = 0;
		}

		ethernet_timer.start();
		server.get_packet(input[writer]);
		ethernet_timer.stop();

		count++;

		// get packet
		unsigned char* buffer = input[writer];

		// decode
	    	
#ifdef usr_code
    /*Compute chunk boundaries on block rcvd*/
	cdc(ChunkBoundary[writer],buffer,blocksize);
    /*Perform SHA on subsequent chunks*/
	for(int chunkIdx = 0;chunkIdx < ChunkBoundary[writer].size();chunkIdx++)
	{
	   SHA256 sha;
	   int IsChunkUnique = -1;
	   std::vector<unsigned int> ChunkLZW;
	   sha.update(&buffer[ChunkBoundary[writer][chunkIdx]],ChunkBoundary[writer][chunkIdx + 1] - ChunkBoundary[writer][chunkIdx]);
	   std::array<uint8_t,32> hash = sha.digest();
       /*Deduplicate using look-up for SHA 256*/
       IsChunkUnique = Deduplicate(ChunkhashTbl, hash);
	   /*If IsChunkUnique == true, compute LZW else send Chunk Id */
       if(true == IsChunkUnique)
	   {
		  ComputeLZW(ChunkLZW,&buffer[ChunkBoundary[writer][chunkIdx]],ChunkBoundary[writer][chunkIdx + 1] - ChunkBoundary[writer][chunkIdx]);
	   }
	}
#endif

		
		done = buffer[1] & DONE_BIT_L;
		length = buffer[0] | (buffer[1] << 8);
		length &= ~DONE_BIT_H;
		//printf("length: %d offset %d\n",length,offset);
		memcpy(&file[offset], &buffer[HEADER], length);

		offset += length;
		writer++;
	}

	// write file to root and you can use diff tool on board
	FILE *outfd = fopen("output_cpu.bin", "wb");
	int bytes_written = fwrite(&file[0], 1, offset, outfd);
	printf("write file with %d\n", bytes_written);
	fclose(outfd);

	for (int i = 0; i < NUM_PACKETS; i++) {
		free(input[i]);
	}

	free(file);
	std::cout << "--------------- Key Throughputs ---------------" << std::endl;
	float ethernet_latency = ethernet_timer.latency() / 1000.0;
	float input_throughput = (bytes_written * 8 / 1000000.0) / ethernet_latency; // Mb/s
	std::cout << "Input Throughput to Encoder: " << input_throughput << " Mb/s."
			<< " (Latency: " << ethernet_latency << "s)." << std::endl;

	return 0;
}

