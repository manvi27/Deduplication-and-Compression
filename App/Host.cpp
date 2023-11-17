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

#define MAX_CHUNK_SIZE 4096
#define LZW_HW_KER

using namespace std;


std::unordered_map <string, int> dedupTable1;
static std::ifstream Input;

extern uint32_t TableSize;


//#define ORIG
int host(std::vector<unsigned int> inputBuf, unsigned char* outputBuf, int packet_length)
{

   //======================================================================================================================
   //
   // OPENCL INITIALIZATION
   //
   // ------------------------------------------------------------------------------------
   // Step 1: Initialize the OpenCL environment
    // ------------------------------------------------------------------------------------
   cl_int err;
   std::string binaryFile = "ESE532_syllabus.html";
   unsigned fileBufSize;
   std::vector<cl::Device> devices = get_xilinx_devices();
   devices.resize(1);
   cl::Device device = devices[0];
   cl::Context context(device, NULL, NULL, NULL, &err);
   char *fileBuf = read_binary_file(binaryFile, fileBufSize);
   cl::Program::Binaries bins{{fileBuf, fileBufSize}};
   cl::Program program(context, devices, bins, NULL, &err);
   cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
   cl::Kernel kernel_lzw(program, "mmult_lzw", &err);

   std::vector<cl::Event> write_event(1);
   std::vector<cl::Event> compute_event(1);
   std::vector<cl::Event> done_event(1);

    cl::Buffer in_buf;
    cl::Buffer out_buf;
    cl::Buffer inputSize_buffer;

   in_buf = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(unsigned char) * MAX_CHUNK_SIZE, NULL, &err);
   inputSize_buffer = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(unsigned char), NULL, &err);
   out_buf = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned char) * (MAX_CHUNK_SIZE / 8) * 13, NULL, &err);

   unsigned char* outputChunk = (unsigned char *)q.enqueueMapBuffer(in_buf, CL_TRUE, CL_MAP_WRITE,
                                                       0, sizeof(unsigned char)*MAX_CHUNK_SIZE);
   unsigned char* tempbuf = (unsigned char *)q.enqueueMapBuffer(out_buf, CL_TRUE, CL_MAP_READ,
                                                       0, sizeof(unsigned char)*(MAX_CHUNK_SIZE / 8) * 13);
#if 0

   char SHA_Buffer[256];
   static std::unordered_map<std::vector<char>, int, VectorHasher> DedupTable;
   static int index = 0;
   uint32_t packetCount = 0;

   while(packetCount < packet_length)
   {
       cdc(inputBuf, outputChunk, packet_length);

       UniqueChunkId = runSHA(dedupTable, input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),
               ChunkBoundary[i + 1] - ChunkBoundary[i]);
       // cout<<input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]);
       if(-1 == UniqueChunkId)
       {
           kernel_lzw.setArg(0, in_buf);
           kernel_lzw.setArg(1, chunksize);
           kernel_lzw.setArg(2, out_buf);

           encoding(input_buffer.substr(ChunkBoundary[i], ChunkBoundary[i + 1] - ChunkBoundary[i]), payload);

           q.enqueueMigrateMemObjects({in_buf}, 0 /* 0 means from host*/, NULL, &write_event[0]);
           q.enqueueTask(LZW_HW_KER, &write_event, &compute_event[0]);
           q.enqueueMigrateMemObjects({out_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &compute_event, &done_event[0]);
           clWaitForEvents(1, (const cl_event *)&done_event[0]);
           header = ((payload.size())<<1);
       }
       else
       {
           // cout<<"Duplicate chunk\n";
           header = (((UniqueChunkId)<<1) | 1);

       }

       memcpy(&file[offset], &header, sizeof(header));
       // cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
       offset +=  sizeof(header);
       // cout<<"chunk size = "<<ChunkBoundary[i + 1] - ChunkBoundary[i]<<"LZW size " <<payload.size()<<endl;
       memcpy(&file[offset], &payload[0], payload.size());
       offset +=  payload.size();
       payload.clear();

       pos = pos - ChunkBoundary[ChunkBoundary.size() - 1];
       input_buffer = input_buffer.substr(ChunkBoundary[ChunkBoundary.size() - 1],pos);
   }
#else
    int offset = 0;
	unsigned char* file;
	std::ifstream myfile;
//	 myfile.open("ESE532_fall.html");
    myfile.open("ESE532_syllabus.html");
//	 myfile.open("LittlePrince.txt");
//	 myfile.open("BenjiBro.txt");
	std::string s;
	file = (unsigned char*) malloc(sizeof(unsigned char) * 70000000);
	char c;

	if ( myfile.is_open() ) { // always check whether the file is open
		while(myfile.get(c))
		{
		    s += c;
		    // std::cout << c; // pipe stream's content to standard output
		}
	} 
	else 
	{
		std::cout << ":("<<endl;
	}
	cout<<s.length();
	vector<unsigned int> ChunkBoundary;
	char payload[4096];
	unsigned int payloadlen;
	//string s = "The Little Prince Chapter IOnce when I was six years old I saw a magnificent picture in a book, called True Stories from Nature, about the primeval forest. It was a picture of a boa constrictor in the act of swallowing an animal. Here is a copy of the drawing.BoaIn the book it said: \"Boa constrictors swallow their prey whole, without chewing it. After that they are not able to move, and they sleep through the six months that they need for digestion.\"I pondered deeply, then, over the adventures of the jungle. And after some work with a colored pencil I succeeded in making my first drawing. My Drawing Number One. It looked something like this:Hat";
    int UniqueChunkId;
    int header;
    ChunkBoundary.push_back(0);
    cdc(ChunkBoundary, s ,s.length());
    ChunkBoundary.push_back(s.length());
	//TotalChunksrcvd += ChunkBoundary.size() ;
	const char *str = s.c_str();
    cout<<ChunkBoundary.size()<<"....\n";
    for(int i = 0; i < ChunkBoundary.size() - 1; i++)
	{
		/*reference for using chunks */
		UniqueChunkId = runSHA(dedupTable1, s.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),
	    ChunkBoundary[i + 1] - ChunkBoundary[i]);
		if(-1 == UniqueChunkId)
		{
// 			//encoding(s.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),payload);
// 			//const char *str = s.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]).c_str();
// 			encoding(str + ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i],payload,&payloadlen);
// 			cout<<"Chuck position : "<<ChunkBoundary[i]<<" chunk size = "<<ChunkBoundary[i + 1] - ChunkBoundary[i]<<" LZW size " <<payloadlen<<" Table Size : "<<TableSize<<endl;
// //			payloadlen = (TableSize > 1) ? payloadlen + 1 : payloadlen;
// 			header = ((payloadlen)<<1);
// 			cout<<"Unique chunk ... "<<UniqueChunkId<<endl;

            kernel_lzw.setArg(0, in_buf);
            kernel_lzw.setArg(1, inputSize_buffer);
            kernel_lzw.setArg(2, out_buf);

            encoding(str + ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i],payload,&payloadlen);
// 			cout<<"Chuck position : "<<ChunkBoundary[i]<<" chunk size = "<<ChunkBoundary[i + 1] - ChunkBoundary[i]<<" LZW size " <<payloadlen<<" Table Size : "<<TableSize<<endl;
            q.enqueueMigrateMemObjects({in_buf}, 0 /* 0 means from host*/, NULL, &write_event[0]);
            q.enqueueTask(kernel_lzw, &write_event, &compute_event[0]);
            q.enqueueMigrateMemObjects({out_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &compute_event, &done_event[0]);
            clWaitForEvents(1, (const cl_event *)&done_event[0]);
            header = ((payloadlen)<<1);
		}
		else
		{
			cout<<"Duplicate chunk ... "<<UniqueChunkId<<endl;
			header = (((UniqueChunkId)<<1) | 1);

		}
		memcpy(&file[offset], &header, sizeof(header));
		// cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
		offset +=  sizeof(header);
		//  cout<<"Chuck position : "<<ChunkBoundary[i]<<" chunk size = "<<ChunkBoundary[i + 1] - ChunkBoundary[i]<<" LZW size " <<payloadlen<<" Table Size : "<<TableSize<<endl;
		memcpy(&file[offset], &payload[0], payloadlen);
		offset +=  payloadlen;
		payloadlen = 0;
	}
	myfile.close();

	/*write compressed file*/
	FILE *outfd = fopen("output_cpu.bin", "wb");
	int bytes_written = fwrite(&file[0], 1, offset, outfd);
	printf("write file with %d\n", bytes_written);
	fclose(outfd);

	// main2("output_cpu.bin", "output_fpga.txt");
#endif
   return 0;
}

int main() {
    return 0;
}