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

//#define ORIG
// int host(void)
// {

//    //======================================================================================================================
//    //
//    // OPENCL INITIALIZATION
//    //
//    // ------------------------------------------------------------------------------------
//    // Step 1: Initialize the OpenCL environment
//     // ------------------------------------------------------------------------------------
//    cout<<"start"<<endl;
//    cl_int err;
//    std::string binaryFile = "kernel.xclbin";
//    unsigned fileBufSize;
//    std::vector<cl::Device> devices = get_xilinx_devices();
//    devices.resize(1);
//    cl::Device device = devices[0];
//    cl::Context context(device, NULL, NULL, NULL, &err);
//    char *fileBuf = read_binary_file(binaryFile, fileBufSize);
//    cout<<"start"<<endl;
//    cl::Program::Binaries bins{{fileBuf, fileBufSize}};
//    cl::Program program(context, devices, bins, NULL, &err);
//    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
//    cl::Kernel kernel_lzw(program, "encoding", &err);

//    std::vector<cl::Event> write_event(1);
//    std::vector<cl::Event> compute_event(1);
//    std::vector<cl::Event> done_event(1);

//     cl::Buffer in_buf;
//     cl::Buffer out_buf;
//     cl::Buffer out_buf_len;

//     cout<<"Declaring variables"<<endl;

//     in_buf = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(char) * MAX_CHUNK_SIZE, NULL, &err);
//     out_buf = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof( char) * (MAX_CHUNK_SIZE *2), NULL, &err);
//     out_buf_len = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned  int) , NULL, &err);

//     cout<<"Declaring buffers"<<endl;
//     char* inputChunk = (char *)q.enqueueMapBuffer(in_buf, CL_TRUE, CL_MAP_WRITE,
//                                                        0, sizeof( char)*MAX_CHUNK_SIZE);
//     char* outputChunk = (char *)q.enqueueMapBuffer(out_buf, CL_TRUE, CL_MAP_READ,
//                                                        0, sizeof( char)*(MAX_CHUNK_SIZE*2));
//     unsigned int* outputChunk_len = (unsigned int*)q.enqueueMapBuffer(out_buf_len, CL_TRUE, CL_MAP_READ,
//                                                        0, sizeof( unsigned int));
//     int offset = 0;
// 	unsigned char* file;
// 	std::ifstream myfile;
// //	 myfile.open("ESE532_fall.html");
//     // myfile.open("ESE532_syllabus.html");
// 	 myfile.open("LittlePrince.txt");
// //	 myfile.open("BenjiBro.txt");
// 	std::string s;
// 	file = (unsigned char*) malloc(sizeof(unsigned char) * 70000000);
// 	char c;

// 	if ( myfile.is_open() ) { // always check whether the file is open
// 		while(myfile.get(c))
// 		{
// 		    s += c;
// 		    // std::cout << s<<endl; // pipe stream's content to standard output
// 		}
// 	} 
// 	else 
// 	{
// 		std::cout << ":("<<endl;
// 	}
//     cout<<"File read"<<endl;
// 	cout<<s.length();
// 	vector<unsigned int> ChunkBoundary;
// 	// char payload[4096];
// 	// unsigned int payloadlen;
// 	//string s = "The Little Prince Chapter IOnce when I was six years old I saw a magnificent picture in a book, called True Stories from Nature, about the primeval forest. It was a picture of a boa constrictor in the act of swallowing an animal. Here is a copy of the drawing.BoaIn the book it said: \"Boa constrictors swallow their prey whole, without chewing it. After that they are not able to move, and they sleep through the six months that they need for digestion.\"I pondered deeply, then, over the adventures of the jungle. And after some work with a colored pencil I succeeded in making my first drawing. My Drawing Number One. It looked something like this:Hat";
//     int UniqueChunkId;
//     int header;
//     unsigned int payloadlen;
//     ChunkBoundary.push_back(0);
//     cdc(ChunkBoundary, s ,s.length());
//     ChunkBoundary.push_back(s.length());
//     cout<<"cdc done"<<endl;
// 	//TotalChunksrcvd += ChunkBoundary.size() ;
// 	const char *str = s.c_str();
	
//     cout<<ChunkBoundary.size()<<"....\n";
//     for(int i = 0; i < ChunkBoundary.size() - 1; i++)
// 	{
// 		/*reference for using chunks */
// 		UniqueChunkId = runSHA(dedupTable1, s.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),
// 	    ChunkBoundary[i + 1] - ChunkBoundary[i]);
       
//         //cout<<"Input chunk : "<<inputChunk<<"..."<<inputChunklen<<endl;
//         cout<<"SHA and dedup done"<<endl;
        
// 		if(-1 == UniqueChunkId)
// 		{   
//             int inputChunklen = ChunkBoundary[i + 1] - ChunkBoundary[i];
//         for(int j = 0; j < inputChunklen;j++)
//         {
//             inputChunk[j] = (str + ChunkBoundary[i])[j];
//             cout<<inputChunk[j];
//         }
//             kernel_lzw.setArg(0, in_buf);
//             kernel_lzw.setArg(1, inputChunklen);
//             kernel_lzw.setArg(2, out_buf );
//             kernel_lzw.setArg(3, out_buf_len);
//             cout<<"arguments set"<<endl;
//             q.enqueueMigrateMemObjects({in_buf}, 0 /* 0 means from host*/, NULL);
//             q.finish();
//             q.enqueueTask(kernel_lzw);
//             q.finish();
//             q.enqueueMigrateMemObjects({out_buf,out_buf_len}, CL_MIGRATE_MEM_OBJECT_HOST);
//             q.finish();
//             cout<<"encoding done"<<endl;
//             header = ((*outputChunk_len)<<1);
//             cout<<"output chunk length = "<<*outputChunk_len<<endl;
//             // for(int k = 0 ; k < *outputChunk_len;k++)
//             // {
//             //     printf("%x ",outputChunk[k]);
//             // }
//             memcpy(&file[offset], &header, sizeof(header));
// 		    // cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
// 		    offset +=  sizeof(header);
// 		    //  cout<<"Chuck position : "<<ChunkBoundary[i]<<" chunk size = "<<ChunkBoundary[i + 1] - ChunkBoundary[i]<<" LZW size " <<payloadlen<<" Table Size : "<<TableSize<<endl;
// 		    memcpy(&file[offset], outputChunk, *outputChunk_len);
// 		    offset +=  *outputChunk_len;
// 		}
// 		else
// 		{
// 			cout<<"Duplicate chunk ... "<<UniqueChunkId<<endl;
// 			header = (((UniqueChunkId)<<1) | 1);
//             memcpy(&file[offset], &header, sizeof(header));
// 		    // cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
// 		    offset +=  sizeof(header);
// 		}
//         cout<<endl;
		
// 	}
// 	myfile.close();

// 	/*write compressed file*/
// 	FILE *outfd = fopen("output_cpu.bin", "wb");
// 	int bytes_written = fwrite(&file[0], 1, offset, outfd);
// 	printf("write file with %d\n", bytes_written);
// 	fclose(outfd);

// 	// main2("output_cpu.bin", "output_fpga.txt");
//    return 0;
// }

// int main() {
//     host();   
//     return 0;
// }

int main(int argc, char* argv[])
{
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
	handle_input(argc, argv, &filename,&blocksize);
    FILE *outfd = fopen(filename, "wb");
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

	writer = pipe_depth;
	server.get_packet(input[writer]);
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
            cdc(ChunkBoundary, input_buffer ,pos);
			
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
				UniqueChunkId = runSHA(dedupTable1, input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),
                        ChunkBoundary[i + 1] - ChunkBoundary[i]);
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
                    kernel_lzw.setArg(0, in_buf);
                    kernel_lzw.setArg(1, inputChunklen);
                    kernel_lzw.setArg(2, out_buf );
                    kernel_lzw.setArg(3, out_buf_len);
                    cout<<"arguments set"<<endl;
                    q.enqueueMigrateMemObjects({in_buf}, 0 /* 0 means from host*/, NULL);
                    q.finish();
                    q.enqueueTask(kernel_lzw);
                    q.finish();
                    q.enqueueMigrateMemObjects({out_buf,out_buf_len}, CL_MIGRATE_MEM_OBJECT_HOST);
                    q.finish();
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

	return 0;

}