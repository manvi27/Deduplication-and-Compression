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
#define MAX_CHUNK_SIZE 4096
#define LZW_HW_KER
#define NUM_PACKETS 8
#define pipe_depth 4
#define DONE_BIT_L (1 << 7)
#define DONE_BIT_H (1 << 15)
#define NUM_CU 4
using namespace std;
// auto constexpr NUM_CU = 4;

std::unordered_map <string, int> dedupTable1;
static std::ifstream Input;
std::vector<cl::Event> write_event(1);
std::vector<cl::Event> compute_event(1);
std::vector<cl::Event> done_event(1);

cl::Buffer in_buf[NUM_CU];
cl::Buffer out_buf[NUM_CU];
cl::Buffer out_buf_len[NUM_CU];
char* inputChunk[NUM_CU];/*Pointer to in_buff: input buffer for kernel_lzw*/
char* outputChunk[NUM_CU];
unsigned int* outputChunk_len[NUM_CU] ;
extern uint32_t TableSize;
unsigned char* file;

#define WIN_SIZE 16
#define PRIME 3
#define MODULUS 256
#define TARGET 0


int sha_done = 0;
int cdc_done = 0;
int lzw_done = 0;
int done = 0;
int offset = 0;
int header;//header for writing back to file 
int dupFlag = 0;
int flag1 = 0, flag2 = 0;
int ModFlag = 0;
unsigned int ModVal[9] = {256, 256, 512, 512, 1024, 1024, 2048, 2048, 4096};
stopwatch ethernet_timer, cdc_timer, sha_timer, lzw_timer, total_timer;

vector <int> DuplicateChunkId;
int ChunksCount = 0;
void core_1_process(string input, unsigned int *ChunkBoundary, int* dupFlag)
{
	cout<<"SHA : Core 1 getting executed"<<endl;
	static int  i = 0;
	stopwatch time;
	while(!sha_done)
	{
		// cout << "\nSHA: i = "<<i<< " "<<"Chunk size - "<<(ChunksCount)<<endl;
		
		if((i < ChunksCount - 1)) 
		{
			cout<<"SHA : Boundary : "<<ChunkBoundary[i + 1]<<" "<<ChunkBoundary[i]<<endl;
			sha_timer.start();
			time.start();
			DuplicateChunkId[i] = runSHA(dedupTable1, input.substr(ChunkBoundary[i], ChunkBoundary[i+1] - ChunkBoundary[i]) , input.length());
			cout<<"SHA : Duplicate chunk ID : "<<DuplicateChunkId[i]<<endl;
			time.stop();
			sha_timer.stop();
			cout<<"SHA : Time taken by core 1 : SHA - "<<time.latency()<<endl;
			if(DuplicateChunkId[i] == -1)
			{
				*dupFlag = -1;
				cout<<"SHA : Chunk unique - "<<i<<"dupFlag : "<<*dupFlag<<endl;
				// dupFlag = -1;
			}
			else
			{
				*dupFlag = i;
				cout<<"SHA : SHA:Chunk duplicate : "<<i<<"dupFlag : "<<*dupFlag<<endl;
				flag1++;
				cout<<"SHA : Flag 1 : "<<flag1<<endl;
				// dupFlag = i;
			}
			i++;

		}
		if((cdc_done == 1) && (i >= (ChunksCount-1)))
		{
			// cout << "i = "<<i<< " "<<"Chunk size - "<<ChunkBoundary.size()<<"sha_done - "<<sha_done<<endl;
			i = 0;
			sha_done = 1;
		}
		
	}

	// cout<<"-----SHA Dupvalue - "<<*dupFlag<<endl;	
	cout << "SHA : Core 1 complete"<<endl;
}

void core_2_process(vector<cl::Kernel> kernel,cl::CommandQueue q,string input,unsigned int *ChunkBoundary, int *dupFlag)
{
	cout<<"LZW: Core 2 getting executed"<<endl;

	// cout<<"-----LZW Dupvalue - "<<*dupFlag<<endl;
	// if(*dupFlag != -1) {
	// 	cout<<"LZW Dupvalue - "<<*dupFlag<<endl;
	// 	flag2++;
	// 	cout<<"Flag 2 : "<<flag2<<endl;
	// }

	static int  i = 0;
	unsigned int inputChunklen[NUM_CU] = {0};
	char *str = (char*)input.c_str();
	unsigned int lzwChunkCount = 0;
	stopwatch time;
	time.start();
	lzw_timer.start();
	cout<<"LZW: Chunk count : "<<ChunksCount<<endl;
	while(!lzw_done)
	{
		cout<<"LZW: LZW Dupvalue - "<<*dupFlag<<endl;
		cout<<"LZW: Begin, i - "<<i<<endl;
		if(i < ChunksCount - 1)
		{
			lzwChunkCount++;
			
			cout<<"LZW: i = "<<i<<" Chunk size - "<<ChunksCount<<endl;
			inputChunklen[i%NUM_CU] =  ChunkBoundary[i + 1] - ChunkBoundary[i];
			cout<<"LZW: ChunkBoundary[i + 1] "<<ChunkBoundary[i + 1]<<" "<<ChunkBoundary[i]<<"Input chunk length = "<<inputChunklen[i%NUM_CU]<<endl;;
			for(unsigned int j = 0; j < inputChunklen[i%NUM_CU];j++)
			{
				inputChunk[i][j] = (str + ChunkBoundary[i])[j];
				// cout<<inputChunk[0];
			}
			cout<<"LZW: LZW Chunk Count - "<<lzwChunkCount<<endl;
			if(!(lzwChunkCount%NUM_CU))
			{
				/*set arguments and enqueue tasks*/
				for(int cu_idx = 0; cu_idx < NUM_CU; cu_idx++)
				{
					kernel[cu_idx].setArg(0, in_buf[cu_idx]);
					kernel[cu_idx].setArg(1, inputChunklen[cu_idx]);
					kernel[cu_idx].setArg(2, out_buf[cu_idx]);
					kernel[cu_idx].setArg(3, out_buf_len[cu_idx]);
					q.enqueueMigrateMemObjects({in_buf[cu_idx]}, 0 /* 0 means from host, NULL, &write_event[0]*/);
 					q.enqueueTask(kernel[cu_idx]/*, &write_event, &compute_event[0]*/);
				}
				/*Wait for kernel execution*/
				q.finish();
				/*Migrate output produced by kernel*/
				for (int cu_idx = 0; cu_idx < NUM_CU; cu_idx++) 
				{
					q.enqueueMigrateMemObjects({out_buf[cu_idx], out_buf_len[cu_idx]}, CL_MIGRATE_MEM_OBJECT_HOST/*, &compute_event, &done_event[0]*/);
				}
				q.finish();
				/*Writing compressed data back to file*/
				for(int cu_idx = 0; cu_idx < NUM_CU; cu_idx++) {
					if(DuplicateChunkId[i - NUM_CU + cu_idx] == -1)
					{
						header = ((*outputChunk_len[cu_idx])<<1);
						cout<<"LZW: Output chunk length = "<<*outputChunk_len[cu_idx]<<endl;
						memcpy(&file[offset], &header, sizeof(header));
						// cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
						offset += sizeof(header);
						// cout<<"Chuck position : "<<ChunkBoundary[j]<<" chunk size = "<<ChunkBoundary[i + 1] - ChunkBoundary[j]<<" LZW size " <<payloadlen<<" Table Size : "<<TableSize<<endl;
						memcpy(&file[offset], outputChunk[cu_idx], *outputChunk_len[cu_idx]);
						offset += *outputChunk_len[cu_idx];
					}
					else
					{
						cout<<"LZW: Chunk duplicate"<<i<<endl;
						header = (((DuplicateChunkId[i - NUM_CU + cu_idx])<<1) | 1);
						memcpy(&file[offset], &header, sizeof(header));
						// cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
						offset +=  sizeof(header);
					}
				}
			}
			
			
			i++;
		}
		cout<<"LZW: Intermediate"<<endl;
		if((cdc_done == 1) && ((i >= ChunksCount-1)))
		{
			cout<<"LZW: remaining chunks : "<<lzwChunkCount<<endl;
			/*set arguments and enqueue tasks*/
			for(int cu_idx = 0; cu_idx < lzwChunkCount%NUM_CU; cu_idx++)
			{
				kernel[cu_idx].setArg(0, in_buf[cu_idx]);
				kernel[cu_idx].setArg(1, inputChunklen[cu_idx]);
				kernel[cu_idx].setArg(2, out_buf[cu_idx]);
				kernel[cu_idx].setArg(3, out_buf_len[cu_idx]);
				q.enqueueMigrateMemObjects({in_buf[cu_idx]}, 0 /* 0 means from host, NULL, &write_event[0]*/);
				q.enqueueTask(kernel[cu_idx]/*, &write_event, &compute_event[0]*/);
			}
			/*Wait for kernel execution*/
			q.finish();
			/*Migrate output produced by kernel*/
			for (int cu_idx = 0; cu_idx < lzwChunkCount%NUM_CU; cu_idx++) 
			{
				q.enqueueMigrateMemObjects({out_buf[cu_idx], out_buf_len[cu_idx]}, CL_MIGRATE_MEM_OBJECT_HOST/*, &compute_event, &done_event[0]*/);
			}
			q.finish();
			/*Writing compressed data back to file*/
			for(int cu_idx = 0; cu_idx < lzwChunkCount%NUM_CU; cu_idx++) {
				cout<<"LZW: Checking duplicacy for idx : "<<i - (lzwChunkCount % NUM_CU) + cu_idx<<endl;
				if(DuplicateChunkId[i - lzwChunkCount%NUM_CU + cu_idx] == -1)
				{
					header = ((*outputChunk_len[cu_idx])<<1);
					cout<<"LZW: Output chunk length = "<<*outputChunk_len[cu_idx]<<endl;
					memcpy(&file[offset], &header, sizeof(header));
					// cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
					offset += sizeof(header);
					// cout<<"Chuck position : "<<ChunkBoundary[j]<<" chunk size = "<<ChunkBoundary[i + 1] - ChunkBoundary[j]<<" LZW size " <<payloadlen<<" Table Size : "<<TableSize<<endl;
					memcpy(&file[offset], outputChunk[cu_idx], *outputChunk_len[cu_idx]);
					offset += *outputChunk_len[cu_idx];
				}
				else
				{
					cout<<"LZW: Chunk duplicate : "<<i<<endl;
					header = (((DuplicateChunkId[i - lzwChunkCount%NUM_CU + cu_idx])<<1) | 1);
					memcpy(&file[offset], &header, sizeof(header));
					// cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
					offset +=  sizeof(header);
				}
			}
			lzw_done = 1;
			i = 0;
		}
		cout<<"LZW: End"<<endl;
	}
	time.stop();
	lzw_timer.stop();
	cout<<"\nLZW: Time taken by core 2 : LZW - "<<time.latency()/1000<<endl;
	cout<<"LZW: Core 2 complete!"<<endl;
}


void core_0_process(unsigned int *ChunkBoundary, string buff, unsigned int buff_size,\
vector<cl::Kernel> kernel,cl::CommandQueue q)
{
	/*CDC calculate on input buffer: 1 chunk -> SHA and LZW call simultaneously + Dedup on core_2_process*/
	// put your cdc implementation here	
	uint64_t i = 0;
    uint32_t prevBoundary = ChunkBoundary[0];
	stopwatch time;
	bool Chunk_start = false;
	uint64_t hash = 0;
	time.start();
	cdc_timer.start();	
	ChunksCount = 1;/*initialize Chunks count to 1*/
	thread core_1_thread;/*thread for SHA executed on core1*/
	thread core_2_thread;/*thread for LZW executed on core2*/
	/*Flags to synchronize between multiple threads*/
	cdc_done = 0;
	sha_done = 0;
	lzw_done = 0;

	// *dupFlag = -1;
	/*Empty the vector storing chunk status for dedup for each chunk*/
	DuplicateChunkId.clear();

	/*Initialize hash value*/
	hash = buff[WIN_SIZE-1]*1 + buff[ WIN_SIZE-2]*3 + buff[ WIN_SIZE-3]*9 + buff[ WIN_SIZE-4]*27 + buff[ WIN_SIZE-5]*81 + buff[ WIN_SIZE-6]*243 + buff[ WIN_SIZE-7]*729 + buff[ WIN_SIZE-8]*2187 + buff[ WIN_SIZE-9]*6561 + buff[ WIN_SIZE-10]*19683 + buff[ WIN_SIZE-11]*59049 + buff[ WIN_SIZE-12]*177147 + buff[ WIN_SIZE-13]*531441 + buff[ WIN_SIZE-14]*1594323 + buff[ WIN_SIZE-15]*4782969 + buff[ WIN_SIZE-16]*14348907;
	/*Compute rolling hash and define chunk boundaries when last 12-bits are zero(for avg chunk size: 4096)*/
	for(i = WIN_SIZE + 1; i < buff_size - WIN_SIZE; i++)
	{
		if(i - prevBoundary == MAX_CHUNK_SIZE) {
		// if(i - prevBoundary == ModVal[ModFlag]) {
			// printf("CDC: Chunk boundary at %ld with length %d\n", i, i - prevBoundary);
			/*Push an initial value to vector storing status of current chunk:unique(1) or duplicate(0) or not verified(-1)*/
			DuplicateChunkId.push_back(-1);
			ChunkBoundary[(ChunksCount)++] = i;
			if(!Chunk_start)
			{
				/*Call to Core 1 thread for SHA calculation as soon as first chunk is defined*/
				core_1_thread = thread(&core_1_process, buff, ChunkBoundary, &dupFlag);
				pin_thread_to_cpu(core_1_thread, 1);
				/*Call to Core 2 thread for LZW calculation as soon as first chunk is defined*/
				core_2_thread = thread(&core_2_process, kernel, q, buff, ChunkBoundary, &dupFlag);
				pin_thread_to_cpu(core_2_thread, 2);
			}
			prevBoundary = i;
			Chunk_start = true;
		} else {
			hash = (hash - buff[i+WIN_SIZE-17] * 14348907) * 3 + buff[i+WIN_SIZE-1];
			if(hash & 0x0FFF == 0)
			{
				// printf("CDC: Chunk boundary at %ld with length %d\n", i, i - prevBoundary);
				/*Push an initial value to vector storing status of current chunk:unique(1) or duplicate(0) or not verified(-1)*/
				DuplicateChunkId.push_back(-1);
				ChunkBoundary[(ChunksCount)++] = i;
				if(!Chunk_start)
				{
					/*Call to Core 1 thread for SHA calculation as soon as first chunk is defined*/
					core_1_thread = thread(&core_1_process, buff, ChunkBoundary, &dupFlag);
					pin_thread_to_cpu(core_1_thread, 1);
					/*Call to Core 2 thread for LZW calculation as soon as first chunk is defined*/
					core_2_thread = thread(&core_2_process, kernel, q, buff, ChunkBoundary, &dupFlag);
					pin_thread_to_cpu(core_2_thread, 2);
				}
				prevBoundary = i;
				Chunk_start = true;
			}
		}
	}

	if(done)
	{
		cout<<"CDC: DONE!"<<endl;
		DuplicateChunkId.push_back(-1);
		ChunkBoundary[(ChunksCount)++] = buff_size;
		for(int i = 0; i < ChunksCount; ++i) {
			cout<<"CDC: Chunk boundary at "<<ChunkBoundary[i]<<endl;
		}
	}
	time.stop();
	cdc_timer.stop();	
	// cout<<"CDC: time taken by core 0 : CDC"<<time.latency()/1000<<endl;
	cdc_done = 1; 
	/*Wait for core 1 to finish compute*/
	core_1_thread.join();
	/*Wait for core 2 to finish compute*/
	core_2_thread.join();
}

int main(int argc, char* argv[])
{
	pin_main_thread_to_cpu0();
	EventTimer timer1, timer2;
	
	unsigned char* input[NUM_PACKETS];
	int writer = 0;
	// int done = 0;
	int length = 0;
	int fileSize = 0;
	int count = 0;
	int blocksize = BLOCKSIZE;/*default is 2k*/ 
    char* filename = strdup("vmlinuz.tar");
	unsigned int ChunkBoundary[MAX_CHUNK_SIZE]; /*Array storing the indices of chunks defined by cdc*/
	char payload[MAX_CHUNK_SIZE];/*Array storing thr lzw compression for chunks*/
	unsigned int payloadlen = 0;/*Length of lzw compression array*/
	int UniqueChunkId; /*Stores -1 value if chunk passed is unique else the id reference for duplicate chunk*/
	int TotalChunksrcvd = 0;/*Stores total chunks defined for the entire file received over ethernet:
							 this includes unique and duplicate chunk*/
	int pos = 0;/*This stores final pos in the input buffer which goes through the compression pipeline*/
	unsigned long startTime, stopTime;/*var for profiling*/
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
	std::vector<cl::Kernel> kernel_lzw(NUM_CU);
    cout<<"Declaring variables"<<endl;
	for(int i = 0 ; i < NUM_CU; i++)
	{
		/*Declare the NUM_CU kernels for encoding */
		kernel_lzw[i] = cl::Kernel(program, "encoding", &err);
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
	fileSize += length;
	writer++;
	timer1.add("Main program");
	std::cout << "--------------- Program Start ---------------" << std::endl<<endl;
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
		ModFlag = (ModFlag < 8) ? ModFlag++ : 8;

		// get packet
		unsigned char* buffer = input[writer];

		// decode
		done = buffer[1] & DONE_BIT_L;
		length = buffer[0] | (buffer[1] << 8);
		length &= ~DONE_BIT_H;
        input_buffer.insert(pos,(const char*)(buffer + 2));
		pos += length;
		fileSize += length;
		if((pos >= 8192+WIN_SIZE)| (done))
		{
		    // cout << "----------done value "<<done <<endl;
			ChunkBoundary[0] = 0;
			ChunksCount = 1;/*Count of total chunks for the current buffer*/
			const char *str = input_buffer.c_str();
			total_timer.start();
			timer2.add("CDC");
			// cdc_timer.start();/*Stopwatch for core 0 */
			/*Core 0: Calls CDC and schedules SHA and LZW calculation on Core 1 and LZW on Core 2 */
			core_0_process(ChunkBoundary, input_buffer ,pos, kernel_lzw, q);
			// cdc_timer.stop();/*Stopwatch for core 0 */
			TotalChunksrcvd += ChunksCount;
			cout<<"Number of chunks "<<ChunksCount<<"remaining buffer to be cat to next input rcvd"<<pos<<"Last Chunk Boundary"<<ChunkBoundary[ChunksCount - 1]<<endl;
			pos = pos - ChunkBoundary[ChunksCount - 1];/*For the next buffer, update the last pos*/
			cout<<"Number of chunks "<<ChunksCount<<"remaining buffer to be cat to next input rcvd"<<pos<<endl;
			input_buffer = input_buffer.substr(ChunkBoundary[ChunksCount - 1],pos);
		}
		writer++;
	}
	
	for(int i = 0 ; i < NUM_CU ; i++)
	{
		q.enqueueUnmapMemObject(in_buf[i],inputChunk[i] );
	    q.enqueueUnmapMemObject(out_buf[i], outputChunk[i]);
    	q.enqueueUnmapMemObject(out_buf_len[i], outputChunk_len[i]);
	}
	timer2.finish();
	std::cout << "--------------- Analysis ---------------" << std::endl<<endl;
	std::cout << "--------------- Chunks ---------------" << std::endl;
	cout<<"Total chunks received = "<< (TotalChunksrcvd - 1)<< " Unique chunks recieved = "<<dedupTable1.size()<<endl;
    cout << "--------------- File ---------------"<<endl<<file[0];
	int bytes_written = fwrite(&file[0], 1, offset, outfd);
	printf("Compressed file size = %d\n", bytes_written);
	cout<<"Input File size = "<<fileSize<<endl;
	cout<<"Compression Percent = "<<(fileSize - bytes_written) * 100 / fileSize <<" %"<<endl;
	total_timer.stop();
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

	std::cout << "--------------- Individual compute time - profiling---------------"
    << std::endl;

	for(int i = 0; i < DuplicateChunkId.size(); ++i) {
		cout<<"______Duplicate chunk IDs "<<i<<" Value "<<DuplicateChunkId[i]<<endl;
	}

	std::cout << "CDC Average latency: " << cdc_timer.avg_latency()<< " ms." << " Latency : "<<cdc_timer.latency()<<" Throughput : "<<fileSize * 8 / (cdc_timer.latency() * 1000)<<" Mbps"<<std::endl;
	std::cout << "SHA Average latency: " << sha_timer.avg_latency()<< " ms." << " Latency : "<<sha_timer.latency()<<" Throughput : "<<fileSize * 8 / (sha_timer.latency() * 1000)<<" Mbps"<<std::endl;
	std::cout << "LZW Average latency: " << lzw_timer.avg_latency()<< " ms." << " Latency : "<<lzw_timer.latency()<<" Throughput : "<<fileSize * 8 / (lzw_timer.latency() * 1000)<<" Mbps"<<std::endl;
	std::cout << "Total latency: " <<total_timer.latency()<<" Throughput : "<<fileSize * 8 / (total_timer.latency() * 1000)<<" Mbps"<<std::endl;
	std::cout << "Flag 1 : " <<flag1<<" Flag 2 : "<<flag2<<std::endl;
	
	return 0;
}