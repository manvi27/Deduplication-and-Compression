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

extern uint32_t TableSize;
unsigned char* file;

int main(int argc, char* argv[])
{
	EventTimer timer1, timer2;
    timer1.add("Main program");
	
	stopwatch ethernet_timer;
	unsigned char* input[NUM_PACKETS];
	int writer = 0;
	int done = 0;
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

	// timer2.add("Allocate contiguous OpenCL buffers");


   std::vector<cl::Event> write_event(1);
   std::vector<cl::Event> compute_event(1);
   std::vector<cl::Event> done_event(1);

    cl::Buffer in_buf;
    cl::Buffer out_buf;
    cl::Buffer out_buf_len;

    cout<<"Declaring variables"<<endl;

    in_buf = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(char) * MAX_CHUNK_SIZE, NULL, &err);
    out_buf = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof( char) * (MAX_CHUNK_SIZE *2), NULL, &err);
    out_buf_len = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned  int) , NULL, &err);

    cout<<"Declaring buffers"<<endl;
    char* inputChunk = (char *)q.enqueueMapBuffer(in_buf, CL_TRUE, CL_MAP_WRITE,
                                                       0, sizeof( char)*MAX_CHUNK_SIZE);
    char* outputChunk = (char *)q.enqueueMapBuffer(out_buf, CL_TRUE, CL_MAP_READ,
                                                       0, sizeof( char)*(MAX_CHUNK_SIZE*2));
    unsigned int* outputChunk_len = (unsigned int*)q.enqueueMapBuffer(out_buf_len, CL_TRUE, CL_MAP_READ,
                                                       0, sizeof( unsigned int));
    int offset = 0;

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

        vector<unsigned int> ChunkBoundary;
        input_buffer.insert(pos,(const char*)(buffer + 2));
		char payload[4096];
        unsigned int payloadlen = 0;
		pos += length;
		int UniqueChunkId; 
		
		int header;//header for writing back to file 
		if((pos >= 8192)| (done))
		{
		    // cout << "----------done value "<<done <<endl;
			ChunkBoundary.push_back(0);
			const char *str = input_buffer.c_str();
			timer2.add("CDC");
            cdc(ChunkBoundary, input_buffer ,pos);
			timer2.add("CDC end");
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
				timer2.add("SHA_Dedup");
				UniqueChunkId = runSHA(dedupTable1, input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),
                        ChunkBoundary[i + 1] - ChunkBoundary[i]);
				timer2.add("SHA_Dedup end");
				
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
		}
		writer++;
	}
	
	timer2.add("Writing output to output_fpga.bin");
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

	timer2.finish();
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
	
	return 0;
}