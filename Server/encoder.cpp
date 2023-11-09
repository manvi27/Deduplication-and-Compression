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
#include <vector>
#include <math.h>
#include "../SHA_algorithm/SHA256.h"
#include <unordered_map>
#define NUM_PACKETS 8
#define pipe_depth 4
#define DONE_BIT_L (1 << 7)
#define DONE_BIT_H (1 << 15)
#define usr_code
using namespace std;
int offset = 0;
unsigned char* file;

#ifdef usr_code

#define WIN_SIZE 16
#define PRIME 3
#define MODULUS 1024
#define TARGET 0

std::unordered_map <string, int> dedupTable;
uint64_t hash_func(string input, unsigned int pos)
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

void cdc(vector<unsigned int> &ChunkBoundary, string buff, unsigned int buff_size)
{
	// put your cdc implementation here
    uint64_t i;
	//unsigned int ChunkCount = 0;
	// printf("buff length passed = %d\n",buff_size);

	for(i = WIN_SIZE; i < buff_size - WIN_SIZE; i++)
	{
		if((hash_func(buff,i)%MODULUS) == 0)
        {
			// printf("chunk boundary at%ld\n",i);
			ChunkBoundary.push_back(i);
		}
	} 
}

void encoding(string s1,vector <char> &output)
{
    unordered_map<string, int> table;
    for (int i = 0; i <= 255; i++) {
        string ch = "";
        ch += char(i);
        table[ch] = i;
    }
 
    string p = "", c = "";
    p += s1[0];
    int code = 256;
    vector<int> output_code;
    //cout << "String\tOutput_Code\tAddition\n";
    for (int i = 0; i < s1.length(); i++) {
        if (i != s1.length() - 1)
            c += s1[i + 1];
        if (table.find(p + c) != table.end()) {
            p = p + c;
        }
        else {
            //cout << p << "\t" << table[p] << "\t\t"
              //   << p + c << "\t" << code << endl;
            output_code.push_back(table[p]);
            table[p + c] = code;
            code++;
            p = c;
        }
        c = "";
    }
    //cout << p << "\t" << table[p] << endl;
    output_code.push_back(table[p]);
    // array <char,4096> output;
    int k =0;
    for(int i =0;i< output_code.size();i++)
    {
       /*Change the endianness of output code*/
	   char data1 = (output_code[i] & 0x0ff0) >> 4;
       char data2 = (output_code[i] & 0x000f) << 4;
	   output_code[i] = (data2<<8)|(data1);
	   if(!(i & 0x01))
       {
        //    /*lower 8-bits of 1st code*/
        //    output[k] = output_code[i] & 0xFF;
        //    printf("%d ",output[k]);
        //    k++;
        //    /*Upper 4-bits in next output*/
        //    output[k] = (output_code[i] & 0xF00)>>4;

           /*lower 8-bits of 1st code*/
		   output.push_back(output_code[i] & 0xFF);
        //    output[k] = output_code[i] & 0xFF;
        //    printf("%d ",output[k]);
           k++;
           /*Upper 4-bits in next output*/
		   output.push_back((output_code[i] & 0xF00)>>4);
        //    output[k] = (output_code[i] & 0xF00)>>4;
       }
       else
       {
           output[k] |= (output_code[i] & 0x0F);
        //    printf("%d ",output[k]);
           k++;
           output.push_back(output_code[i]>>4);
        //    printf("%d ",output[k]);
           k++;
       }
       
    }
    // printf("\n");
    //return output_code;
}


#endif

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
	printf("Let's gooo\n");
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
	printf("help cleared \n");

	for (int i = 0; i < NUM_PACKETS; i++) {
		input[i] = (unsigned char*) malloc(
				sizeof(unsigned char) * (NUM_ELEMENTS + HEADER));
		if (input[i] == NULL) {
			std::cout << "aborting " << std::endl;
			return 1;
		}
	}
	printf("Let's gooo x 2\n");

	server.setup_server(blocksize);

	writer = pipe_depth;
	server.get_packet(input[writer]);
	count++;
	printf("Let's gooo x 3\n");
	// get packet
	unsigned char* buffer = input[writer];

	// decode
	done = buffer[1] & DONE_BIT_L;
	length = buffer[0] | (buffer[1] << 8);
	length &= ~DONE_BIT_H;

#ifdef usr_code
    std::string input_buffer;
	int pos = 0;
	// printf("copying string");
	input_buffer.insert(0,(const char*)(buffer+2));
	pos += length;
	// cout << input_buffer<< endl;
#endif

	// printing takes time so be weary of transfer rate
	//printf("length: %d offset %d\n",length,offset);

	// we are just memcpy'ing here, but you should call your
	// top function here.
	// memcpy(&file[offset], &buffer[HEADER], length);

	offset += length;
	writer++;
	printf("Let's gooo x 6\n");
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
		done = buffer[1] & DONE_BIT_L;
		length = buffer[0] | (buffer[1] << 8);
		length &= ~DONE_BIT_H;

#ifdef usr_code
        vector<unsigned int> ChunkBoundary;
        input_buffer.insert(pos,(const char*)(buffer + 2));
		vector <char> payload;
		pos += length;
		int UniqueChunkId; 
		int header;//header for writing back to file 
		if((pos >= 8096)| (done))
		{
		    //cout << input_buffer <<endl;
			ChunkBoundary.push_back(0);
			cdc(ChunkBoundary, input_buffer ,pos );
			for(int i = 0; i < ChunkBoundary.size() - 1; i++)
            {
                // printf("Point5\n");
                /*reference for using chunks */
                // cout <<ChunkBoundary[i + 1] - ChunkBoundary[i]<<"\n";
				UniqueChunkId = runSHA(dedupTable, input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),
                        ChunkBoundary[i + 1] - ChunkBoundary[i]);
				if(-1 == UniqueChunkId)
				{
					encoding(input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),payload);
					header = ((ChunkBoundary[i + 1] - ChunkBoundary[i])<<1);
				}
				else
				{
					header = (((ChunkBoundary[i + 1] - ChunkBoundary[i])<<1) | 1);
					payload.push_back(UniqueChunkId & 0xFF);
					payload.push_back((UniqueChunkId>>8) & 0xFF);
					payload.push_back((UniqueChunkId>>16) & 0xFF);
					payload.push_back((UniqueChunkId>>24) & 0xFF);

				}
				memcpy(&file[offset], &header, sizeof(header));
				// cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
				offset +=  sizeof(header);
				memcpy(&file[offset], &payload, payload.size());
				offset +=  payload.size();
				payload.clear();

            }
			if(true == done)
            {  
                // printf("Point6\n");
                /*If not eof, update pos to accomodate remaining characters of current buffer
                for next chunking operation*/
                pos = pos - ChunkBoundary[ChunkBoundary.size() - 1];
			    input_buffer = input_buffer.substr(ChunkBoundary[ChunkBoundary.size() - 1],pos);
                /*Clear vector after using chunks and buffer for performing SHA and LZW
                This can be different in final implementation. We may want to save chunking
                vector longer*/
                ChunkBoundary.clear();
            }

			pos = pos - ChunkBoundary[ChunkBoundary.size() - 1];
			input_buffer = input_buffer.substr(ChunkBoundary[ChunkBoundary.size() - 1],pos);

		}
#endif
		//printf("length: %d offset %d\n",length,offset);
		// memcpy(&file[offset], &buffer[HEADER], length);

		offset += length;
		writer++;
	}
    cout << "-------------file---------------"<<endl<<file[0];
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
