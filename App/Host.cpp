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
using namespace std;


std::unordered_map <string, int> dedupTable1;
static std::ifstream Input;
std::vector<cl::Event> write_event(1);
std::vector<cl::Event> compute_event(1);
std::vector<cl::Event> done_event(1);

cl::Buffer in_buf;
cl::Buffer out_buf;
cl::Buffer out_buf_len;

char* outputChunk;
unsigned int* outputChunk_len ;
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
vector <int> UniqueChunk;
int UniqueChunkId;
int ChunksCount = 0;
void core_1_process(string input,unsigned int *ChunkBoundary)
{
	cout<<"core 1 getting executed"<<endl;
	static int  i = 0;
	stopwatch time;
	while(!sha_done)
	{
		cout << "\nSHA: i="<<i<< " "<<"Chunk size"<<(ChunksCount)<<endl;
		
		if((i < ChunksCount-1)) 
		{
			cout<<ChunkBoundary[i + 1]<<" "<<ChunkBoundary[i]<<endl;
			time.start();
			UniqueChunkId = runSHA(dedupTable1,input.substr(ChunkBoundary[i],ChunkBoundary[i+1]-ChunkBoundary[i]) , input.length());
			time.stop();
			cout<<"time taken by core 1 : SHA"<<time.latency()/1000<<endl;
			if(UniqueChunkId == -1)
			{
				UniqueChunk[i] = 1;
				cout<<"SHA:Chunk unique"<<UniqueChunk[i]<<" "<<i<<endl;
			}
			else
			{
				UniqueChunk[i] = 0;
				cout<<"SHA:Chunk duplicate"<<UniqueChunk[i]<<" "<<i<<endl;
			}
			i++;

		}
		if((cdc_done == 1) && (i >= (ChunksCount-1)))
		{
			// cout << "i="<<i<< " "<<"Chunk size"<<ChunkBoundary.size()<<endl;
			i = 0;
			sha_done = 1;
		}
		
	}
	
	cout << "core 1 complete"<<endl;
}

void core_2_process(cl::Kernel kernel,cl::CommandQueue q,string input,unsigned int *ChunkBoundary,char *inputChunk)
{
	cout<<"core 2 getting executed"<<endl;
	static int  i = 0;
	unsigned int inputChunklen = 0;
	char *str = (char*)input.c_str();
	stopwatch time;
	while(!lzw_done)
	{
		if(i < ChunksCount-1)
		{
			time.start();
			cout<<"LZW: i = "<<i<<" Chunk size "<<ChunksCount<<endl;
			inputChunklen =  ChunkBoundary[i + 1] - ChunkBoundary[i];
			cout<<ChunkBoundary[i + 1]<<" "<<ChunkBoundary[i]<<"input chunk length = "<<inputChunklen<<endl;;
			for(int j = 0; j < inputChunklen;j++)
			{
				inputChunk[j] = (str + ChunkBoundary[i])[j];
				// cout<<inputChunk[j];
			}
			kernel.setArg(0, in_buf);
			kernel.setArg(1, inputChunklen);
			kernel.setArg(2, out_buf );
			kernel.setArg(3, out_buf_len);
			q.enqueueMigrateMemObjects({in_buf}, 0 /* 0 means from host*/, NULL, &write_event[0]);
			q.enqueueTask(kernel, &write_event, &compute_event[0]);
			q.enqueueMigrateMemObjects({out_buf, out_buf_len}, CL_MIGRATE_MEM_OBJECT_HOST, &compute_event, &done_event[0]);
			clWaitForEvents(1, (const cl_event *)&done_event[0]);
			if(UniqueChunk[i] == 1)
			{
				cout<<"LZW:Chunk unique"<<i<<endl;
				header = ((*outputChunk_len)<<1);
				cout<<"LZW:output chunk length = "<<*outputChunk_len<<endl;
				memcpy(&file[offset], &header, sizeof(header));
				// cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
				offset +=  sizeof(header);
				// cout<<"Chuck position : "<<ChunkBoundary[i]<<" chunk size = "<<ChunkBoundary[i + 1] - ChunkBoundary[i]<<" LZW size " <<payloadlen<<" Table Size : "<<TableSize<<endl;
				memcpy(&file[offset], outputChunk, *outputChunk_len);
				offset +=  *outputChunk_len;
			}
			else
			{
				// cout<<"Duplicate chunk\n";
				cout<<"LZW:Chunk duplicate"<<i<<endl;
                header = (((UniqueChunkId)<<1) | 1);
                memcpy(&file[offset], &header, sizeof(header));
		        // cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
		        offset +=  sizeof(header);
			}
			time.stop();
			cout<<"\ntime taken by core 2 : LZW"<<time.latency()/1000<<endl;
			i++;
		}
		if((cdc_done == 1) && ((i >= ChunksCount-1)))
		{
			lzw_done = 1;
			i = 0;
		}
	}
	
	cout<<"core 2 complete"<<endl;
}


void core_0_process(unsigned int *ChunkBoundary,string buff, unsigned int buff_size,\
cl::Kernel kernel,cl::CommandQueue q,char* inputChunk)
{
	/*CDC calculate on input buffer: 1 chunk -> SHA and LZW call simultaneously + Dedup on core_2_process*/
	// put your cdc implementation here	
	uint64_t i;
    uint32_t prevBoundary = ChunkBoundary[0];
	stopwatch time;
	bool Chunk_start = 0;
	uint64_t hash = 0;
	time.start();	
	ChunksCount = 1;/*initialize Chunks count to 1*/
	thread core_1_thread;/*thread for SHA executed on core1*/
	thread core_2_thread;/*thread for LZW executed on core2*/
	/*Flags to synchronize between multiple threads*/
	cdc_done = 0;
	sha_done = 0;
	lzw_done = 0;
	/*Empty the vector storing chunk status for dedup for each chunk*/
	UniqueChunk.clear();
	/*Initialize hash value*/
	hash = buff[WIN_SIZE-1]*1 + buff[ WIN_SIZE-2]*3 + buff[ WIN_SIZE-3]*9 + buff[ WIN_SIZE-4]*27 + buff[ WIN_SIZE-5]*81 + buff[ WIN_SIZE-6]*243 + buff[ WIN_SIZE-7]*729 + buff[ WIN_SIZE-8]*2187 + buff[ WIN_SIZE-9]*6561 + buff[ WIN_SIZE-10]*19683 + buff[ WIN_SIZE-11]*59049 + buff[ WIN_SIZE-12]*177147 + buff[ WIN_SIZE-13]*531441 + buff[ WIN_SIZE-14]*1594323 + buff[ WIN_SIZE-15]*4782969 + buff[ WIN_SIZE-16]*14348907;
	/*Compute rolling hash and define chunk boundaries when last 12-bits are zero(for avg chunk size: 4096)*/
	for(i = WIN_SIZE + 1; i < buff_size - WIN_SIZE; i++)
	{
		hash = (hash - buff[i+WIN_SIZE-17]*14348907)*3 + buff[i+WIN_SIZE-1];
		if((hash & 0xFFF == 0) || (i - prevBoundary == 4096))
        {
			printf("chunk boundary at%ld with length %d\n",i,i-prevBoundary);
			/*Push an initial value to vector storing status of current chunk:unique(1) or duplicate(0) or not verified(-1)*/
			UniqueChunk.push_back(-1);
			ChunkBoundary[(ChunksCount)++] = i;
			if(!Chunk_start)
			{
				/*Call to Core 1 thread for SHA calculation as soon as first chunk is defined*/
				core_1_thread = thread(&core_1_process,buff,ChunkBoundary);
				pin_thread_to_cpu(core_1_thread,1);
				/*Call to Core 2 thread for LZW calculation as soon as first chunk is defined*/
				core_2_thread = thread(&core_2_process,kernel,q,buff,ChunkBoundary,inputChunk);
				pin_thread_to_cpu(core_2_thread,2);
			}
			prevBoundary = i;
			Chunk_start = 1;
		}
	}
	if(done)
	{
		UniqueChunk.push_back(-1);
		ChunkBoundary[(ChunksCount)++] = buff_size;
	}
	time.stop();	
	cout<<"time taken by core 0 : CDC"<<time.latency()/1000<<endl;
	cdc_done = 1; 
	/*Wait for core 1 to finish compute*/
	core_1_thread.join();
	/*Wait for core 2 to finish compute*/
	core_2_thread.join();
}

int powArr2[WIN_SIZE] = {1,3,9,27,81,243,729,2187,6561,19683,59049,177147,531441,1594323,4782969,14348907};

uint64_t hash_func2(string input, unsigned int pos)
{
	// put your hash function implementation here
    uint64_t hash = 0;
    hash += input[pos+WIN_SIZE-1]*1;
    hash += input[pos+WIN_SIZE-2]*3;
    hash += input[pos+WIN_SIZE-3]*9;
    hash += input[pos+WIN_SIZE-4]*27;
    hash += input[pos+WIN_SIZE-5]*81;
    hash += input[pos+WIN_SIZE-6]*243;
    hash += input[pos+WIN_SIZE-7]*729;
    hash += input[pos+WIN_SIZE-8]*2187;
    hash += input[pos+WIN_SIZE-9]*6561;
    hash += input[pos+WIN_SIZE-10]*19683;
    hash += input[pos+WIN_SIZE-11]*59049;
    hash += input[pos+WIN_SIZE-12]*177147;
    hash += input[pos+WIN_SIZE-13]*531441;
    hash += input[pos+WIN_SIZE-14]*1594323;
    hash += input[pos+WIN_SIZE-15]*4782969;
    hash += input[pos+WIN_SIZE-16]*14348907;
	// std::cout << "............HASH : " << hash << std::endl;
    return hash;
}

int CDC(string inputBuf, int *outputBuf, int *lastChunkIdx)
{
	// put your cdc implementation here
	int i, chunkSize = 0;
	stopwatch time;
	time.start();
	uint64_t currHash = hash_func2(inputBuf, *lastChunkIdx);

	for(i = *lastChunkIdx; i < *lastChunkIdx + WIN_SIZE; i++)
	{
		outputBuf[chunkSize++] = inputBuf[i];
	}

	*lastChunkIdx = *lastChunkIdx + WIN_SIZE;

	static unsigned long p = powArr2[WIN_SIZE-1];

	for(i = *lastChunkIdx; i < inputBuf.length()-WIN_SIZE ; i++)
	{
	if((currHash % MODULUS) == TARGET || chunkSize >= MAX_CHUNK_SIZE)
	{
	break;
	}

	outputBuf[chunkSize++] = inputBuf[i];

	(*lastChunkIdx)++;

		currHash = currHash * PRIME - ((uint64_t)inputBuf[i] * p) + ((uint64_t)inputBuf[i + WIN_SIZE] * PRIME);
	}

	outputBuf[chunkSize++] = '\0';
	time.stop();
	cout<<"CDC latency "<<time.latency()<<endl;
	return chunkSize;
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
   cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
   cl::Kernel kernel_lzw(program, "encoding", &err);

    cout<<"Declaring variables"<<endl;

    in_buf = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(char) * MAX_CHUNK_SIZE, NULL, &err);
    out_buf = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof( char) * (MAX_CHUNK_SIZE *2), NULL, &err);
    out_buf_len = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned  int) , NULL, &err);

    cout<<"Declaring buffers"<<endl;
    char* inputChunk = (char *)q.enqueueMapBuffer(in_buf, CL_TRUE, CL_MAP_WRITE,
                                                       0, sizeof( char)*MAX_CHUNK_SIZE);
    outputChunk = (char *)q.enqueueMapBuffer(out_buf, CL_TRUE, CL_MAP_READ,
                                                       0, sizeof( char)*(MAX_CHUNK_SIZE*2));
    outputChunk_len = (unsigned int*)q.enqueueMapBuffer(out_buf_len, CL_TRUE, CL_MAP_READ,
                                                       0, sizeof( unsigned int));
    

	// default is 2k
	int blocksize = BLOCKSIZE;
    char* filename = strdup("vmlinuz.tar");
	// set blocksize if decalred through command line
	// handle_input(argc, argv, &filename,&blocksize);
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
	server.setup_server(blocksize);
	// timer2.add("Get packet");
	writer = pipe_depth;
	// ethernet_timer.start();
	server.get_packet(input[writer]);
	// ethernet_timer.stop();
	count++;
	// get packet
	unsigned char* buffer = input[writer];

	// decode
	done = buffer[1] & DONE_BIT_L;
	length = buffer[0] | (buffer[1] << 8);
	length &= ~DONE_BIT_H;

    int TotalChunksrcvd = 0;
    std::string input_buffer;
	int pos = 0;
	// printf("copying string");
	input_buffer.insert(0,(const char*)(buffer+2));
	pos += length;
	// cout << input_buffer<< endl;
	unsigned long startTime, stopTime, totalTime = 0;

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

        unsigned int ChunkBoundary[4096];
        input_buffer.insert(pos,(const char*)(buffer + 2));
		char payload[4096];
        unsigned int payloadlen = 0;
		pos += length;
		int UniqueChunkId; 
		
		
		if((pos >= 8192+16)| (done))
		{
		    cout << "----------done value "<<done <<endl;
			ChunkBoundary[0] = 0;
			ChunksCount = 1;
			const char *str = input_buffer.c_str();
			cdc_timer.start();
			timer2.add("CDC");
<<<<<<< HEAD
			cout<<"...................Pos : "<<pos<<endl;
            cdc(ChunkBoundary, input_buffer ,pos);
			timer2.add("CDC end");
			cdc_timer.stop();
            if(128 == done)
			{
				ChunkBoundary.push_back(pos);
			}
			TotalChunksrcvd += ChunkBoundary.size() ;
			for(int i = 0; i < ChunkBoundary.size() - 1; i++)
            {
                // printf("Point5\n");
                /*reference for using chunks */
                // cout <<ChunkBoundary[i + 1] - ChunkBoundary[i];
				sha_timer.start();
				timer2.add("SHA_Dedup");
				UniqueChunkId = runSHA(dedupTable1, input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),
                        ChunkBoundary[i + 1] - ChunkBoundary[i]);
				timer2.add("SHA_Dedup end");
				sha_timer.stop();
				
				if(-1 == UniqueChunkId)
				{
					// encoding(str + ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i],payload,&payloadlen);
					// encoding(input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),payload);
					// header = (payloadlen<<1);
					// cout<<"Unique chunk\n";
                    int inputChunklen = ChunkBoundary[i + 1] - ChunkBoundary[i];
                    for(int j = 0; j < inputChunklen;j++)
                    {
                        inputChunk[j] = (str + ChunkBoundary[i])[j];
                        cout<<inputChunk[j];
                    }
					lzw_timer.start();
					timer2.add("Running kernel");
                    kernel_lzw.setArg(0, in_buf);
                    kernel_lzw.setArg(1, inputChunklen);
                    kernel_lzw.setArg(2, out_buf );
                    kernel_lzw.setArg(3, out_buf_len);
                    cout<<"arguments set"<<endl;
                    q.enqueueMigrateMemObjects({in_buf}, 0 /* 0 means from host*/, NULL, &write_event[0]);
					q.enqueueTask(kernel_lzw, &write_event, &compute_event[0]);
					q.enqueueMigrateMemObjects({out_buf, out_buf_len}, CL_MIGRATE_MEM_OBJECT_HOST, &compute_event, &done_event[0]);
					clWaitForEvents(1, (const cl_event *)&done_event[0]);
                    // q.finish();
					timer2.add("Running kernel end");
					lzw_timer.stop();

					compute_event[0].getProfilingInfo<unsigned long> (CL_PROFILING_COMMAND_START, &startTime);
					compute_event[0].getProfilingInfo<unsigned long> (CL_PROFILING_COMMAND_END, &stopTime);
					totalTime += (stopTime - startTime);
					cout<<"Start Time : "<<startTime<<endl;
					cout<<"Stop Time : "<<stopTime<<endl;
					cout<<"Total Time : "<<totalTime<<endl;
                    cout<<"encoding done"<<endl;
                    header = ((*outputChunk_len)<<1);
                    cout<<"output chunk length = "<<*outputChunk_len<<endl;
                    memcpy(&file[offset], &header, sizeof(header));
		            // cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
		            offset +=  sizeof(header);
		            //  cout<<"Chuck position : "<<ChunkBoundary[i]<<" chunk size = "<<ChunkBoundary[i + 1] - ChunkBoundary[i]<<" LZW size " <<payloadlen<<" Table Size : "<<TableSize<<endl;
		            memcpy(&file[offset], outputChunk, *outputChunk_len);
		            offset +=  *outputChunk_len;
				}
				else
				{
					// cout<<"Duplicate chunk\n";
                    header = (((UniqueChunkId)<<1) | 1);
                    memcpy(&file[offset], &header, sizeof(header));
		            // cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
		            offset +=  sizeof(header);
				}
			}
			pos = pos - ChunkBoundary[ChunkBoundary.size() - 1];
			input_buffer = input_buffer.substr(ChunkBoundary[ChunkBoundary.size() - 1],pos);
=======
			core_0_process(ChunkBoundary, input_buffer ,pos,kernel_lzw,q,inputChunk);
			cdc_timer.stop();
			TotalChunksrcvd += ChunksCount;
			cout<<"Number of chunks "<<ChunksCount<<"remaining buffer to be cat to next input rcvd"<<pos<<"Last Chunk Boundary"<<ChunkBoundary[ChunksCount - 1]<<endl;
			pos = pos - ChunkBoundary[ChunksCount - 1];
			cout<<"Number of chunks "<<ChunksCount<<"remaining buffer to be cat to next input rcvd"<<pos<<endl;
			input_buffer = input_buffer.substr(ChunkBoundary[ChunksCount - 1],pos);
>>>>>>> 92fb430 (Multi threading and SHA NEON add)
		}
		writer++;
	}
	
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

<<<<<<< HEAD
	std::cout << "CDC Average latency: " << cdc_timer.avg_latency()<< " ms." << " Latency : "<<cdc_timer.latency()<<std::endl;
	std::cout << "SHA Average latency: " << sha_timer.avg_latency()<< " ms." << " Latency : "<<sha_timer.latency()<<std::endl;
	std::cout << "LZW Average latency: " << lzw_timer.avg_latency()<< " ms." << " Latency : "<<lzw_timer.latency()<<std::endl;
=======
	// std::cout << "CDC Average latency: " << cdc_timer.avg_latency()<< " ms." << " Latency : "<<cdc_timer.latency()<<std::endl;
	// std::cout << "SHA Average latency: " << sha_timer.avg_latency()<< " ms." << " Latency : "<<sha_timer.latency()<<std::endl;
	// std::cout << "LZW Average latency: " << lzw_timer.avg_latency()<< " ms." << " Latency : "<<lzw_timer.latency()<<std::endl;
>>>>>>> 92fb430 (Multi threading and SHA NEON add)
	
	return 0;
}