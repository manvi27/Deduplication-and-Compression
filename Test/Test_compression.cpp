//#include <stdio.h>
//#include <stdint.h>
//#include <stdlib.h>
//#include <string.h>
//#include <iostream>
//#include <unistd.h>
//#include <fcntl.h>
//#include <errno.h>
//#include <fcntl.h>
//#include <fstream>
//#include <vector>
//#include <math.h>
//#include "../SHA_algorithm/SHA256.h"
//#include <unordered_map>
//
//using namespace std;
//
//#define WIN_SIZE 16
//#define PRIME 3
//#define MODULUS 1024
//#define TARGET 0
//uint64_t hash_func(string input, unsigned int pos)
//{
//	// put your hash function implementation here
//    uint64_t hash = 0;
//	uint8_t i = 0;
//
//	for(i = 0; i < WIN_SIZE; i++)
//	{
//		hash += input[pos+WIN_SIZE-1-i]*(pow(PRIME,i+1));
//	}
//	return hash;
//}
//
//void cdc(vector<unsigned int> &ChunkBoundary, string buff, unsigned int buff_size)
//{
//	// put your cdc implementation here
//    uint32_t i;
//	unsigned int ChunkCount = 0;
//    // printf("buff length passed = %d\n",buff_size);
//
//	for(i = WIN_SIZE; i < buff_size - WIN_SIZE; i++)
//	{
//		if((hash_func(buff,i)%MODULUS) == 0)
//        {
//			// printf("chunk boundary at%d\n",i);
//			ChunkBoundary.push_back(i);
//		}
//	}
//}
//
//
//
//int main()
//{
//    // Creation of ifstream class object to read the file
//    ifstream fin;
//    // by default open mode = ios::in mode
//    fin.open("D:/MastersUpenn/ESE5320_SOC/Assignments/Project/Test/LittlePrince.txt");
//    string input_buffer = "";
//    vector<unsigned int> ChunkBoundary;
//    int pos = 0;
//    std::unordered_map <std::string, int> dedupTable;
//    /*read char by char till end of file*/
//    while(false == fin.eof())
//    {
//        input_buffer += fin.get();
//        /*Since file size is 14247, I've taken buffer size as 8096 which is the amount
//        of data to be collected before passing it to cdc*/
//        if((input_buffer.length() >= 8096) || (true == fin.eof()))
//        {
//		    pos = input_buffer.length();
//            /*For ease of use, push 0 to vector because cdc function takes starting address
//            of the buffer and buffer length*/
//            ChunkBoundary.push_back(0);
//			cdc(ChunkBoundary, input_buffer ,pos );
//            for(int i = 0; i < ChunkBoundary.size() - 1; i++)
//            {
//                /*reference for using chunks */
//                //cout <<input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]);
//                runSHA(dedupTable, input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),
//                        sizeof(input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i])));
//            }
//			if(false == fin.eof())
//            {
//                /*If not eof, update pos to accomodate remaining characters of current buffer
//                for next chunking operation*/
//                pos = pos - ChunkBoundary[ChunkBoundary.size() - 1];
//			    input_buffer = input_buffer.substr(ChunkBoundary[ChunkBoundary.size() - 1],pos);
//                /*Clear vector after using chunks and buffer for performing SHA and LZW
//                This can be different in final implementation. We may want to save chunking
//                vector longer*/
//                ChunkBoundary.clear();
//            }
//        }
//    }
//    /*Last chunk */
//    cout<<input_buffer.substr(ChunkBoundary[ChunkBoundary.size()-1], input_buffer.length()-ChunkBoundary[ChunkBoundary.size()-1]);
//    fin.close();
//    return 0;
//}
