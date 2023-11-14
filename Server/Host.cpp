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

int host(vector<unsigned int> inputBuf, unsigned char* outputBuf, int packet_length)
{ 
    
    //======================================================================================================================
    //
    // OPENCL INITIALIZATION
    //   
    std::cout << "Running " << CHUNKS << "x" <<NUM_TESTS << " iterations of " << N << "x" << N
    << " task pipelined floating point mmult..." << std::endl;
    // ------------------------------------------------------------------------------------
    // Step 1: Initialize the OpenCL environment
     // ------------------------------------------------------------------------------------
    cl_int err;
    std::string binaryFile = argv[1];
    unsigned fileBufSize;
    std::vector<cl::Device> devices = get_xilinx_devices();
    devices.resize(1);
    cl::Device device = devices[0];
    cl::Context context(device, NULL, NULL, NULL, &err);
    char *fileBuf = read_binary_file(binaryFile, fileBufSize);
    cl::Program::Binaries bins{{fileBuf, fileBufSize}};
    cl::Program program(context, devices, bins, NULL, &err);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    cl::Kernel krnl_lzw(program, "mmult_lzw", &err);

    std::vector<cl::Event> write_event(1);
    std::vector<cl::Event> compute_event(1);
    std::vector<cl::Event> done_event(1);


    in_buf = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(unsigned char) * MAX_CHUNK_SIZE, NULL, &err);
    out_buf = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned char) * (MAX_CHUNK_SIZE / 8) * 13, NULL, &err);

    unsigned char* outputChunk = (unsigned char *)q.enqueueMapBuffer(in_buf, CL_TRUE, CL_MAP_WRITE, 
                                                        0, sizeof(unsigned char)*MAX_CHUNK_SIZE);
    unsigned char* tempbuf = (unsigned char *)q.enqueueMapBuffer(out_buf, CL_TRUE, CL_MAP_READ, 
                                                        0, sizeof(unsigned char)*(MAX_CHUNK_SIZE / 8) * 13);

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

            encoding(input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),payload);

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

    return;
}