#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <vector>
#include <math.h>
using namespace std;

#define WIN_SIZE 16
#define PRIME 3
#define MODULUS 1024
#define TARGET 0
uint64_t hash_func(string input, unsigned int pos)
{
	// put your hash function implementation here
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
    uint32_t i;
	unsigned int ChunkCount = 0;
	printf("buff length passed = %d\n",buff_size);

	for(i = WIN_SIZE; i < buff_size - WIN_SIZE; i++)
	{
		if((hash_func(buff,i)%MODULUS) == 0)
        {
			printf("chunk boundary at%d\n",i);
			ChunkBoundary.push_back(i);
		}
	} 
}



int main()
{
    // Creation of ifstream class object to read the file
    ifstream fin;
    // by default open mode = ios::in mode
    fin.open("D:/MastersUpenn/ESE5320_SOC/Assignments/Project/Test/LittlePrince.txt");
    string input_buffer = "";
    vector<unsigned int> ChunkBoundary;
    int pos = 0;
    while(false == fin.eof())
    {
        input_buffer += fin.get();
        // cout<<input_buffer.length()<<endl;
        if((input_buffer.length() >= 8096) || (true == fin.eof()))
        {
		    pos = input_buffer.length() ;
            cout << "input buffer = "<<input_buffer << endl;
            printf("pos before chunking = %d\n",pos);
			cdc(ChunkBoundary, input_buffer ,pos );
			if(false == fin.eof())
            {
                pos = pos - ChunkBoundary[ChunkBoundary.size() - 1];
                printf("pos after chunking = %d\n",pos);
            
			    input_buffer = input_buffer.substr(ChunkBoundary[ChunkBoundary.size() - 1],pos);
            
                cout << input_buffer.length()<<endl;
            }
        }
    }
    fin.close();
    return 0;
    
}