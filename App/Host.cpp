#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <CL/cl2.hpp>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <vector>
#include "../Decoder/Decoder.h"
#include "../SHA_algorithm/SHA256.h"
#include "../Server/encoder.h"
#include "Utilities.h"
#include "EventTimer.h"
#include "Host.h"
#define MAX_CHUNK_SIZE 256
#define LZW_HW_KER
#define NUM_PACKETS 8
#define pipe_depth 4
#define DONE_BIT_L (1 << 7)
#define DONE_BIT_H (1 << 15)
using namespace std;
auto constexpr num_cu = 4;

std::unordered_map <string, int> dedupTable1;
static std::ifstream Input;
vector<cl::Buffer> in_buf(num_cu);
vector<cl::Buffer> out_buf(num_cu);
vector<cl::Buffer> out_buf_len(num_cu);
char* inputChunk[num_cu];/*Pointer to in_buff: input buffer for kernel_lzw*/
char* outputChunk[num_cu];
unsigned int* outputChunk_len[num_cu] ;
extern uint32_t TableSize;
unsigned char* file;

#define WIN_SIZE 16
#define PRIME 3
#define MODULUS 4096
#define TARGET 0


int sha_done = 0;
int cdc_done = 0;
int lzw_done = 0;
int done = 0;
int offset = 0;
int header;//header for writing back to file 
int TotalChunksCount = 0;
vector <int> DuplicateChunkId;/*used to store status corresponding to each chunk(unqiue + duplicate)*/
vector <int> ChunkstatusId(1000);/*Used to store status of all chunks*/
int ChunksCount = 0;/* chunk count for given buffer processed by cdc at a given time*/
unsigned int UniqueChunkLength[100];/*Used to store unique chunk length to be passed to lzw function*/
thread core_2_thread;/*thread for LZW executed on core2*/
unsigned int primepow[WIN_SIZE] = {3,9 ,27 ,81 ,243 ,729 ,2187 ,6561 ,19683 ,59049 ,177147 ,531441 ,1594323 ,4782969 ,14348907,43046721};

void core_2_process(vector<cl::Kernel> kernel,cl::CommandQueue q,string input,unsigned int *ChunkBoundary)
{
	cout<<"core 2 getting executed"<<endl;
	static int  i = 0;
	static int UniqueId = 0;
	static int ChunkWrItr = 0;/*Used to store count of chunks for which compression data is written in file*/
	unsigned int inputChunklen[100] = {0};
	char *str = (char*)input.c_str();
	stopwatch time;
	stopwatch time_lzw;
	stopwatch timeIntermediate;
	time.start();
	while(!lzw_done)
	{		
		if(i < ChunksCount-1)
		{
			inputChunklen[i] =  ChunkBoundary[i + 1] - ChunkBoundary[i];
			cout<<"\nBoundaries: "<<ChunkBoundary[i]<<".."<<ChunkBoundary[i+1]<<"for chunk id"<<i<<endl;
			timeIntermediate.start();
			DuplicateChunkId[i] = runSHA(dedupTable1,input.substr(ChunkBoundary[i],ChunkBoundary[i+1]-ChunkBoundary[i]) , ChunkBoundary[i+1]-ChunkBoundary[i]);
			cout<<"\nDuplicate chunk id = "<<DuplicateChunkId[i]<<"Unique id "<<UniqueId<<endl;
			timeIntermediate.stop();
			if(DuplicateChunkId[i] == -1)
			{
				for(int j = 0; j < inputChunklen[i];j++)
				{
					inputChunk[UniqueId%num_cu][j] = (str + ChunkBoundary[i])[j];
				}
				UniqueChunkLength[UniqueId] = inputChunklen[i];
				UniqueId++;
				cout<<"unique chunk"<<endl;
			}
			else
			{
				cout<<"Duplicate chunk"<<endl;
			}
			
			if((UniqueId%num_cu == 0) && (UniqueId > 0))
			{
				std::vector<cl::Event> write_event(100);
				std::vector<cl::Event> compute_event(100);
				std::vector<cl::Event> done_event(100);
				std::vector<std::vector<cl::Event>> write_list(100);
				std::vector<std::vector<cl::Event>> compute_list(100);
				std::vector<std::vector<cl::Event>> done_list(100);

				time_lzw.start();
			    /*Running 4 chunks*/
				for(int cu_idx = 0; cu_idx < num_cu; cu_idx++)
				{
					cout<<"kernel set arg "<<cu_idx<<"Unique chunk length "<< UniqueChunkLength[cu_idx + (UniqueId/num_cu)-1]<<"for index"<<cu_idx + (UniqueId/num_cu)-1<<endl;
					kernel[cu_idx].setArg(0, in_buf[cu_idx]);
					kernel[cu_idx].setArg(1, UniqueChunkLength[cu_idx + (UniqueId/num_cu)-1]);
					kernel[cu_idx].setArg(2, out_buf[cu_idx]);
					kernel[cu_idx].setArg(3, out_buf_len[cu_idx]);
					q.enqueueMigrateMemObjects({in_buf[cu_idx]}, 0 /* 0 means from host*/, NULL, &write_event[cu_idx]);
					write_list[cu_idx].push_back(write_event[cu_idx]);
					q.enqueueTask(kernel[cu_idx], &write_list[cu_idx], &compute_event[cu_idx]);
					compute_list[cu_idx].push_back(compute_event[cu_idx]);
					q.enqueueMigrateMemObjects({out_buf[cu_idx], out_buf_len[cu_idx]}, CL_MIGRATE_MEM_OBJECT_HOST, &compute_list[cu_idx], &done_event[cu_idx]);

				}
				/*Migrate output produced by kernel*/
				for (int cu_idx = 0; cu_idx < num_cu; cu_idx++) 
				{
					done_event[cu_idx].wait();
				}

				time_lzw.stop();
				UniqueId = 0;
				
			}
			
			/*Update chunk iterator*/
			i++;
		}

		if((cdc_done) && (i >= (ChunksCount - 1)))
		{
			lzw_done = 1;
			if(done)
			{
				std::vector<cl::Event> write_event(100);
				std::vector<cl::Event> compute_event(100);
				std::vector<cl::Event> done_event(100);
				std::vector<std::vector<cl::Event>> write_list(100);
				std::vector<std::vector<cl::Event>> compute_list(100);
				std::vector<std::vector<cl::Event>> done_list(100);

				time_lzw.start();
				/*Running 4 chunks*/
				for(int cu_idx = 0; cu_idx < (UniqueId); cu_idx++)
				{
					cout<<"kernel set arg "<<cu_idx<<"for chunk length"<<UniqueChunkLength[cu_idx]<<"for index"<<cu_idx <<endl;
					kernel[cu_idx].setArg(0, in_buf[cu_idx]);
					kernel[cu_idx].setArg(1, UniqueChunkLength[cu_idx]);
					kernel[cu_idx].setArg(2, out_buf[cu_idx]);
					kernel[cu_idx].setArg(3, out_buf_len[cu_idx]);
					q.enqueueMigrateMemObjects({in_buf[cu_idx]}, 0 /* 0 means from host*/, NULL, &write_event[cu_idx]);
					write_list[cu_idx].push_back(write_event[cu_idx]);
					q.enqueueTask(kernel[cu_idx], &write_list[cu_idx], &compute_event[cu_idx]);
					compute_list[cu_idx].push_back(compute_event[cu_idx]);
					q.enqueueMigrateMemObjects({out_buf[cu_idx], out_buf_len[cu_idx]}, CL_MIGRATE_MEM_OBJECT_HOST, &compute_list[cu_idx], &done_event[cu_idx]);
				}
				for(int cu_idx = 0; cu_idx < (UniqueId);cu_idx++)
				{
					done_event[cu_idx].wait();
				}
				time_lzw.stop();
			}
			i = 0;
		}
	}
	
	time.stop();
	cout<<"SHA execution time:"<<timeIntermediate.latency()/1000<<endl;
	cout<<"Lzw execution time"<<time_lzw.latency()/1000<<endl;
	cout<<"core2 execution time:"<<time.latency()/1000<<endl;
}


void core_0_process(unsigned int *ChunkBoundary,string buff, unsigned int buff_size,\
vector<cl::Kernel> kernel,cl::CommandQueue q)
{
	/*CDC calculate on input buffer: 1 chunk -> SHA and LZW call simultaneously + Dedup on core_2_process*/
	// put your cdc implementation here	
	uint64_t i;
    uint32_t prevBoundary = ChunkBoundary[0];
	stopwatch time,time_core2;
	bool Chunk_start = 0;
	uint64_t hash = 0;
	time.start();
	time_core2.start();	
	ChunksCount = 1;/*initialize Chunks count to 1*/
	// thread core_1_thread;/*thread for SHA executed on core1*/
	
	/*Flags to synchronize between multiple threads*/
	cdc_done = 0;
	sha_done = 0;
	lzw_done = 0;
	/*Empty the vector storing chunk status for dedup for each chunk*/
	DuplicateChunkId.clear();
	/*Initialize hash value*/
	for(int i = WIN_SIZE; i < 2*WIN_SIZE; i++)
    {
        hash += buff[i]*primepow[2*WIN_SIZE - i - 1];
    }
	/*Compute rolling hash and define chunk boundaries when last 12-bits are zero(for avg chunk size: 4096)*/
	for(i = WIN_SIZE + 1; i < buff_size - WIN_SIZE; i++)
	{
		if((hash % MODULUS == 0) || (i - prevBoundary - 1 == 4096))
        {
			// printf("chunk boundary at%ld with length %d\n",i -1 ,i -prevBoundary);
			/*Push an initial value to vector storing status of current chunk:unique(1) or duplicate(0) or not verified(-1)*/
			DuplicateChunkId.push_back(-1);/*Used for deciding whether to compute lzw*/
			ChunkstatusId[TotalChunksCount++] = -1;/*Used for writing back to file*/
			ChunkBoundary[(ChunksCount)++] = i - 1;
			if(!Chunk_start)
			{
				/*Call to Core 2 thread for LZW calculation as soon as first chunk is defined*/
				core_2_thread = thread(&core_2_process,kernel,q,buff,ChunkBoundary);
				pin_thread_to_cpu(core_2_thread,2);
			}
			prevBoundary = i - 1;
			Chunk_start = 1;
		}
		hash = (hash - buff[i - 1]*43046721)*3 + buff[i+WIN_SIZE-1]*3;
	}
	if(done)
	{
		DuplicateChunkId.push_back(-1);
		ChunkBoundary[(ChunksCount)++] = buff_size;
	}
	time.stop();	
	cout<<"time taken by core 0 : CDC"<<time.latency()/1000<<endl;
	cdc_done = 1; 
	/*Wait for core 2 to finish compute*/
	core_2_thread.join();
	time_core2.stop();
	cout<<"total time taken to process a buffer"<<time_core2.latency()/1000<<endl;
}

int main(int argc, char* argv[])
{
	pin_main_thread_to_cpu0();
	EventTimer timer1, timer2;
	
	stopwatch ethernet_timer, cdc_timer, sha_timer, lzw_timer;
	unsigned char* input[NUM_PACKETS];
	int writer = 0;
	// int done = 0;
	int length = 0;
	int count = 0;
	int blocksize = BLOCKSIZE;/*default is 2k*/ 
    char* filename = strdup("vmlinuz.tar");
	unsigned int ChunkBoundary[4096]; /*Array storing the indices of chunks defined by cdc*/
	char payload[4096];/*Array storing thr lzw compression for chunks*/
	unsigned int payloadlen = 0;/*Length of lzw compression array*/
	int UniqueChunkId; /*Stores -1 value if chunk passed is unique else the id reference for duplicate chunk*/
	int TotalChunksrcvd = 0;/*Stores total chunks defined for the entire file received over ethernet:
							 this includes unique and duplicate chunk*/
	int pos = 0;/*This stores final pos in the input buffer which goes through the compression pipeline*/
	unsigned long startTime, stopTime, totalTime = 0;/*var for profiling*/
	ESE532_Server server;

	//======================================================================================================================
	//
	// OPENCL INITIALIZATION
	//
	// ------------------------------------------------------------------------------------
	// Step 1: Initialize the OpenCL environment
	// ------------------------------------------------------------------------------------
	cout<<"start"<<endl;
	cl_int err;
	std::string binaryFile = "kernel.xclbin";
	unsigned fileBufSize;
	std::vector<cl::Device> devices = get_xilinx_devices();
	devices.resize(1);
	cl::Device device = devices[0];
	cl::Context context(device, NULL, NULL, NULL, &err);
	char *fileBuf = read_binary_file(binaryFile, fileBufSize);
	cout<<"start"<<endl;
	cl::Program::Binaries bins{{fileBuf, fileBufSize}};
	cl::Program program(context, devices, bins, NULL, &err);
	cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
	/*Multiple compute units declaration*/
	std::vector<cl::Kernel> kernel_lzw(num_cu);
    cout<<"Declaring variables"<<endl;
	kernel_lzw[0] = cl::Kernel(program, "encoding:{encoding_1}", &err);
	kernel_lzw[1] = cl::Kernel(program, "encoding:{encoding_2}", &err);
	kernel_lzw[2] = cl::Kernel(program, "encoding:{encoding_3}", &err);
	kernel_lzw[3] = cl::Kernel(program, "encoding:{encoding_4}", &err);
	for(int i = 0 ; i < num_cu; i++)
	{
		/*Declare the num_cu kernels for encoding */
		// kernel_lzw[i] = cl::Kernel(program, "encoding", &err);
		/*Define input and output buffers for kernel*/
		in_buf[i] = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(char) * MAX_CHUNK_SIZE, NULL, &err);
		out_buf[i] = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof( char) * (MAX_CHUNK_SIZE *2), NULL, &err);
		out_buf_len[i] = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned  int) , NULL, &err);
		/*Set pointers to buffers for kernel*/
		inputChunk[i] = (char *)q.enqueueMapBuffer(in_buf[i], CL_TRUE, CL_MAP_WRITE,
                                                       0, sizeof( char)*MAX_CHUNK_SIZE);
    	outputChunk[i] = (char *)q.enqueueMapBuffer(out_buf[i], CL_TRUE, CL_MAP_READ,
                                                       0, sizeof( char)*(MAX_CHUNK_SIZE*2));
    	outputChunk_len[i] = (unsigned int*)q.enqueueMapBuffer(out_buf_len[i], CL_TRUE, CL_MAP_READ,
                                                       0, sizeof( unsigned int));
	}
    cout<<"Declaring buffers"<<endl;
	
	// set blocksize if declared through command line
	blocksize = (argc < 3)?blocksize:(*(int*)argv[2]);
    FILE *outfd = fopen(argv[1], "wb");
    if(outfd == NULL)
   {
	    perror("invalid output file");
		exit(EXIT_FAILURE);
   }
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
	/*Setup the server*/
	server.setup_server(blocksize);
	writer = pipe_depth;
	server.get_packet(input[writer]);
	count++;
	unsigned char* buffer = input[writer];

	// decode
	done = buffer[1] & DONE_BIT_L;
	length = buffer[0] | (buffer[1] << 8);
	length &= ~DONE_BIT_H;

  
    std::string input_buffer;
	input_buffer.insert(0,(const char*)(buffer+2));
	pos += length;
	writer++;
	timer1.add("Main program");
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
        input_buffer.insert(pos,(const char*)(buffer + 2));
		pos += length;
		if((pos >= 8192+WIN_SIZE)| (done))
		{
		    cout << "----------done value "<<done <<endl;
			ChunkBoundary[0] = 0;
			ChunksCount = 1;/*Count of total chunks for the current buffer*/
			const char *str = input_buffer.c_str();
			timer2.add("CDC");
			cdc_timer.start();/*Stopwatch for core 0 */
			/*Core 0: Calls CDC and schedules SHA and LZW calculation on Core 1 and LZW on Core 2 */
			core_0_process(ChunkBoundary, input_buffer ,pos, kernel_lzw, q);
			cdc_timer.stop();/*Stopwatch for core 0 */
			TotalChunksrcvd += ChunksCount;
			cout<<"Number of chunks "<<ChunksCount<<"remaining buffer to be cat to next input rcvd"<<pos<<"Last Chunk Boundary"<<ChunkBoundary[ChunksCount - 1]<<endl;
			pos = pos - ChunkBoundary[ChunksCount - 1];/*For the next buffer, update the last pos*/
			cout<<"Number of chunks "<<ChunksCount<<"remaining buffer to be cat to next input rcvd"<<pos<<endl;
			input_buffer = input_buffer.substr(ChunkBoundary[ChunksCount - 1],pos);
		}
		writer++;
	}
	
	for(int i = 0 ; i < num_cu ; i++)
	{
		q.enqueueUnmapMemObject(in_buf[i],inputChunk[i] );
	    q.enqueueUnmapMemObject(out_buf[i], outputChunk[i]);
    	q.enqueueUnmapMemObject(out_buf_len[i], outputChunk_len[i]);
	}
	// core_2_thread.join();
	timer2.finish();
	cout<<"Total chunks rcvd = "<< (TotalChunksrcvd - 1)<< "unique chunks rcvd = "<<dedupTable1.size()<<endl;
    cout << "-------------file---------------"<<endl<<file[0];
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

	// timer2.finish();
    std::cout << "--------------- Key execution times ---------------"
    << std::endl;
    timer2.print();

    timer1.finish();
    std::cout << "--------------- Total time ---------------"
    << std::endl;
    timer1.print();

	std::cout << "--------------- Total compute time - profiling---------------"
    << std::endl;
    cout<<"Time : "<<totalTime<<endl;

	std::cout << "CDC Average latency: " << cdc_timer.avg_latency()<< " ms." << " Latency : "<<cdc_timer.latency()<<std::endl;
	// stdss::cout << "SHA Average latency: " << sha_timer.avg_latency()<< " ms." << " Latency : "<<sha_timer.latency()<<std::endl;
	// std::cout << "LZW Average latency: " << lzw_timer.avg_latency()<< " ms." << " Latency : "<<lzw_timer.latency()<<std::endl;
	
	return 0;
}