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
#include "../CDC/CDC_NEON.h"
#include "../CDC/CDC.h"
#include <unordered_map>
#include <arm_neon.h>
#define NUM_PACKETS 8
#define pipe_depth 4
#define DONE_BIT_L (1 << 7)
#define DONE_BIT_H (1 << 15)
#define usr_code
using namespace std;
int offset = 0;
// unsigned char* file;

#ifdef usr_code

#define WIN_SIZE 16
#define PRIME 3
#define MODULUS 4096
#define TARGET 0

uint32_t powArr[WIN_SIZE] = {1,3,9,27,81,243,729,2187,6561,19683,59049,177147,531441,1594323,4782969,14348907};

std::unordered_map <string, int> dedupTable;
// uint64_t hash_func(string input, unsigned int pos)
// {
// 	// put your hash function implementation here
//     uint64_t hash = 0;
// 	uint8_t i = 0;

// 	for(i = 0; i < WIN_SIZE; i++)
// 	{
// 		// hash += input[pos+WIN_SIZE-1-i]*(pow(PRIME,i+1));powArr
//         hash += input[pos+WIN_SIZE-1-i]*(powArr[i]);
// 	}
// 	return hash;
// }
// void XXH3_accumulate_512_neon(void* XXH_RESTRICT acc, const void* XXH_RESTRICT input, const void* XXH_RESTRICT secret);

uint64_t hash_func1(string input, unsigned int pos)
{

const char *p = input.c_str();
uint32_t arr1[16] =  {1,3,9,27,81,243,729,2187,6561,19683,59049,177147,531441,1594323,4782969,14348907};
uint32_t result[16];
uint32x4_t vec1_0 = vld1q_u32((const uint32_t*)p);
uint32x4_t vec2_0 = vld1q_u32(arr1);
uint32x4_t vec3_0 = vmulq_u32(vec1_0, vec2_0);
vst1q_u32(result, vec3_0);
uint32x4_t vec1_1 = vld1q_u32((const uint32_t*)p+4);
uint32x4_t vec2_1 = vld1q_u32(arr1+4);
uint32x4_t vec3_1 = vmulq_u32(vec1_1, vec2_1);
vst1q_u32(result+4, vec3_1);
uint32x4_t vec1_2 = vld1q_u32((const uint32_t*)p+8);
uint32x4_t vec2_2 = vld1q_u32(arr1+8);
uint32x4_t vec3_2 = vmulq_u32(vec1_2, vec2_2);
vst1q_u32(result+8, vec3_2);
uint32x4_t vec1_3 = vld1q_u32((const uint32_t*)p+12);
uint32x4_t vec2_3 = vld1q_u32(arr1+12);
uint32x4_t vec3_3 = vmulq_u32(vec1_3, vec2_3);
vst1q_u32(result+12, vec3_3);

uint64_t hash = 0;
for(int i =0; i<16;++i) {
    // cout<<".................result : "<<result[i]<<endl;
    hash+=result[i];
}
    return hash;
}

uint64_t HashCDC = 0;
#if 1
uint64_t hash_func(string input, unsigned int pos)
{
	// put your hash function implementation here
    uint64_t hash = (HashCDC - (input[pos+WIN_SIZE-17]*14348907)) / 3;
    hash += input[pos+WIN_SIZE-1];
    HashCDC = hash;
    // hash += input[pos+WIN_SIZE-1]*1;
    // hash += input[pos+WIN_SIZE-2]*3;
    // hash += input[pos+WIN_SIZE-3]*9;
    // hash += input[pos+WIN_SIZE-4]*27;
    // hash += input[pos+WIN_SIZE-5]*81;
    // hash += input[pos+WIN_SIZE-6]*243;
    // hash += input[pos+WIN_SIZE-7]*729;
    // hash += input[pos+WIN_SIZE-8]*2187;
    // hash += input[pos+WIN_SIZE-9]*6561;
    // hash += input[pos+WIN_SIZE-10]*19683;
    // hash += input[pos+WIN_SIZE-11]*59049;
    // hash += input[pos+WIN_SIZE-12]*177147;
    // hash += input[pos+WIN_SIZE-13]*531441;
    // hash += input[pos+WIN_SIZE-14]*1594323;
    // hash += input[pos+WIN_SIZE-15]*4782969;
    // hash += input[pos+WIN_SIZE-16]*14348907;
	// std::cout << "............HASH : " << hash << std::endl;
    // lastHash = hash;
    return hash;
}
#endif
uint64_t Hashprologue(string input) {
    //i = WIN_SIZE
    //i + WINSIZE = 32
    uint64_t hash = 0;
    hash += input[32-1];
    hash += input[32-2]*3;
    hash += input[32-3]*9;
    hash += input[32-4]*27;
    hash += input[32-5]*81;
    hash += input[32-6]*243;
    hash += input[32-7]*729;
    hash += input[32-8]*2187;
    hash += input[32-9]*6561;
    hash += input[32-10]*19683;
    hash += input[32-11]*59049;
    hash += input[32-12]*177147;
    hash += input[32-13]*531441;
    hash += input[32-14]*1594323;
    hash += input[32-15]*4782969;
    hash += input[32-16]*14348907;

    HashCDC = hash;

    return hash;
}
#define FIXED_CHUNKING 1
void cdc(vector<unsigned int> &ChunkBoundary, string buff, unsigned int buff_size)
{
#ifdef FIXED_CHUNKING
    uint64_t i = buff_size;
    uint32_t countChunk = 0;
    while(i > 4096) {
        countChunk++;
        cout<<"................Chunk Boundary : "<<countChunk * 4096;
        ChunkBoundary.push_back(countChunk*4096);
        i-=4096;
    }
    cout<<".................Chunk Boundary : "<<i;
    ChunkBoundary.push_back(buff_size);
    return;
#else
    // put your cdc implementation here
    uint64_t i;
	// printf("buff length passed = %d\n",buff_size);
    uint32_t prevBoundary = ChunkBoundary[0];

    if((Hashprologue(buff) & 0x0FFF) == 0)
    {
    //    printf("chunk boundary at%ld\n",i-prevBoundary);
        printf("-----------------------Do Hash\n");
        ChunkBoundary.push_back(WIN_SIZE);
        prevBoundary = WIN_SIZE;
    }

	for(i = WIN_SIZE + 1; i < buff_size - WIN_SIZE; i++)
	{
		
        if(i - prevBoundary == 4096) {
            printf(".....................>Skip hash\n");
			ChunkBoundary.push_back(i);
            prevBoundary = i;
            continue;
        }
        //if(((hash_func(buff,i) % MODULUS) == 0) || (i - prevBoundary == 4096))
        else if((hash_func(buff,i) & 0x0FFF) == 0)
        {
		//    printf("chunk boundary at%ld\n",i-prevBoundary);
            printf("-----------------------Do Hash\n");
			ChunkBoundary.push_back(i);
            prevBoundary = i;
            continue;
		}
	} 
#endif
}

void Swencoding(string s1,vector <char> &output)
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
	// printf("---------output code before endianness change-----------\n");
	// for(int i=0;i<output_code.size();i++)
	// {
	// 	printf("%x ",output_code[i]);
	// }
    int k =0;
    for(int i =0;i< output_code.size();i++)
    {
       /*Change the endianness of output code*/
	   char data1 = (output_code[i] & 0x0ff0) >> 4;
       char data2 = (output_code[i] & 0x000f) << 4;
	   output_code[i] = (data2<<8)|(data1);
	   if(!(i & 0x01))
       {           /*lower 8-bits of 1st code*/
		   output.push_back(output_code[i] & 0xFF);
           k++;
           /*Upper 4-bits in next output*/
		   output.push_back((output_code[i] & 0xFF00)>>8);
        //    output[k] = (output_code[i] & 0xF00)>>4;
       }
       else
       {
           output[k] |= ((output_code[i] & 0x00F0)>>4);
            //    printf("%d ",output[k]);
			k++;
			output.push_back(output_code[i]<<4);
		//    printf("%d ",output[k]);
			output[k] |= ((output_code[i] >> 12) & 0x000F);
			k++;
       }
       
    }
    // printf("\n");
    //return output_code;
	cout<<endl;
}

#define BUCKET_SIZE 2
#define CAPACITY (32768*BUCKET_SIZE) // hash output is 15 bits, and we have 1 entry per bucket, so capacity is 2^15
#define BUCKET_LIMIT 3
//#define CAPACITY 4096
// try  uncommenting the line above and commenting line 6 to make the hash table smaller
// and see what happens to the number of entries in the assoc mem
// (make sure to also comment line 27 and uncomment line 28)

unsigned int my_hash(unsigned long key)
{
    key &= 0xFFFFF; // make sure the key is only 20 bits

    unsigned int hashed = 0;

    for(int i = 0; i < 20; i++)
    {
        hashed += (key >> i)&0x01;
        hashed += hashed << 10;
        hashed ^= hashed >> 6;
    }
    hashed += hashed << 3;
    hashed ^= hashed >> 11;
    hashed += hashed << 15;
    return hashed & 0x7FFF;          // hash output is 15 bits
    //return hashed & 0xFFF;
}

void hash_lookup(unsigned long (&hash_table)[CAPACITY/BUCKET_SIZE][BUCKET_SIZE], unsigned int key, bool* hit, unsigned int* result)
{
    //std::cout << "hash_lookup():" << std::endl;
    key &= 0xFFFFF; // make sure key is only 20 bits

    unsigned long lookup = 0;//hash_table[my_hash(key)];
    unsigned int stored_key = 0;
    unsigned int value = 0;
    unsigned int valid = 0;
    for(int i = 0 ; i < BUCKET_SIZE; i++)
    {
        lookup = hash_table[my_hash(key)][i];
        // [valid][value][key]
        stored_key = lookup&0xFFFFF;       // stored key is 20 bits
        value = (lookup >> 20)&0xFFF;      // value is 12 bits
        valid = (lookup >> (20 + 12))&BUCKET_LIMIT; // valid is 1 bit
        if((valid == (1 << i)) && (key == stored_key))
        {
            *hit = true;
            *result = value;
            // std::cout << "\thit the hash" << std::endl;
            //std::cout << "\t(k,v,h) = " << key << " " << value << " " << my_hash(key) << std::endl;
            break;
        }
        else
        {
            *hit = false;
            *result = 0;
            //std::cout << "\tmissed the hash" << std::endl;
        }
    }
}

void hash_insert(unsigned long (&hash_table)[CAPACITY/BUCKET_SIZE][BUCKET_SIZE], unsigned int key, unsigned int value, bool* collision)
{
    //std::cout << "hash_insert():" << std::endl;
    key &= 0xFFFFF;   // make sure key is only 20 bits
    value &= 0xFFF;   // value is only 12 bits
    int Bucketfill = 0;
    unsigned long lookup;// = hash_table[my_hash(key)];
    unsigned int valid;// = (lookup >> (20 + 12))&0x1;
    for(int i = 0; i < BUCKET_SIZE; i++)
    {
        lookup = hash_table[my_hash(key)][i];
        valid = (lookup >> (20 + 12))&BUCKET_LIMIT; /*read last 2-bits: if 01 - entry 0 else entry 1*/
        //cout<<".....Valid "<<valid<<endl;
        if(valid == 1<<i)
        {
            // cout<<"avbnnm "<<i <<Bucketfill<<endl;
            Bucketfill++;
        }
        // printf("value checked before insertion = %lX. Bucketfill = %d valid = %x\n",hash_table[my_hash(key)][Bucketfill],Bucketfill,valid);
        //printf("value checked before insertion = 0x%lX... Bucketfill = %d valid = %x\n",lookup,Bucketfill,valid);
    }
   
    if(Bucketfill >= BUCKET_SIZE)
    {
        *collision = true;
        // printf("\nCollision\n");
    }
    else
    {
        hash_table[my_hash(key)][Bucketfill] = (1UL << (20 + 12 + (Bucketfill))) | (value << 20) | key;
        // printf("value checked after insertion = %lX. Bucketfill = %d valid = %x...myhashkey - %lX\n",hash_table[my_hash(key)][Bucketfill],Bucketfill,valid,my_hash(key));
        // cout<<"value inserted before insertion"<<hash_table[my_hash(key)][Bucketfill]<<"Bucketfill"<<Bucketfill<<endl;
        *collision = false;
    }
   
    /*if(valid)
    {
        *collision = 1;
        //std::cout << "\tcollision in the hash" << std::endl;
    }
    else
    {
        hash_table[my_hash(key)] = (1UL << (20 + 12)) | (value << 20) | key;
        *collision = 0;
        //std::cout << "\tinserted into the hash table" << std::endl;
        //std::cout << "\t(k,v,h) = " << key << " " << value << " " << my_hash(key) << std::endl;
    }*/
}
//****************************************************************************************************************
typedef struct
{
    // Each key_mem has a 9 bit address (so capacity = 2^9 = 512)
    // and the key is 20 bits, so we need to use 3 key_mems to cover all the key bits.
    // The output width of each of these memories is 64 bits, so we can only store 64 key
    // value pairs in our associative memory map.

    unsigned long upper_key_mem[512]; // the output of these  will be 64 bits wide (size of unsigned long).
    unsigned long middle_key_mem[512];
    unsigned long lower_key_mem[512];
    unsigned int value[64];    // value store is 64 deep, because the lookup mems are 64 bits wide
    unsigned int fill;         // tells us how many entries we've currently stored
} assoc_mem;

// cast to struct and use ap types to pull out various feilds.

void assoc_insert(assoc_mem* mem,  unsigned int key, unsigned int value, bool* collision)
{
    //std::cout << "assoc_insert():" << std::endl;
    key &= 0xFFFFF; // make sure key is only 20 bits
    value &= 0xFFF;   // value is only 12 bits

    if(mem->fill < 64)
    {
        mem->upper_key_mem[(key >> 18)%512] |= (1 << mem->fill);  // set the fill'th bit to 1, while preserving everything else
        mem->middle_key_mem[(key >> 9)%512] |= (1 << mem->fill);  // set the fill'th bit to 1, while preserving everything else
        mem->lower_key_mem[(key >> 0)%512] |= (1 << mem->fill);   // set the fill'th bit to 1, while preserving everything else
        mem->value[mem->fill] = value;
        mem->fill++;
        *collision = false;
        //std::cout << "\tinserted into the assoc mem" << std::endl;
        //std::cout << "\t(k,v) = " << key << " " << value << std::endl;
    }
    else
    {
        *collision = true;
       
        //std::cout << "\tcollision in the assoc mem" << std::endl;
    }
}

void assoc_lookup(assoc_mem* mem, unsigned int key, bool* hit, unsigned int* result)
{
    //std::cout << "assoc_lookup():" << std::endl;
    key &= 0xFFFFF; // make sure key is only 20 bits

    unsigned int match_high = mem->upper_key_mem[(key >> 18)%512];
    unsigned int match_middle = mem->middle_key_mem[(key >> 9)%512];
    unsigned int match_low  = mem->lower_key_mem[(key >> 0)%512];

    unsigned int match = match_high & match_middle & match_low;

    unsigned int address = 0;
    for(; address < 64; address++)
    {
        if((match >> address) & 0x1)
        {
            break;
        }
    }
    if(address != 64)
    {
        *result = mem->value[address];
        *hit = 1;
        //std::cout << "\thit the assoc" << std::endl;
        //std::cout << "\t(k,v) = " << key << " " << *result << std::endl;
    }
    else
    {
        *hit = 0;
        //std::cout << "\tmissed the assoc" << std::endl;
    }
}
//****************************************************************************************************************
void insert(unsigned long (&hash_table)[CAPACITY/BUCKET_SIZE][BUCKET_SIZE], assoc_mem* mem, unsigned int key, unsigned int value, bool* collision)
{
    hash_insert(hash_table, key, value, collision);
    if(*collision)
    {
        assoc_insert(mem, key, value, collision);
    }
}

void lookup(unsigned long (&hash_table)[CAPACITY/BUCKET_SIZE][BUCKET_SIZE], assoc_mem* mem, unsigned int key, bool* hit, unsigned int* result)
{
    hash_lookup(hash_table, key, hit, result);
    if(!*hit)
    {
        assoc_lookup(mem, key, hit, result);
    }
}
//****************************************************************************************************************
void encoding(const char* s1,int length,char *output_code,unsigned int *output_code_len)
{
    // create hash table and assoc mem
    unsigned long hash_table[CAPACITY/BUCKET_SIZE][BUCKET_SIZE];
    assoc_mem my_assoc_mem;
    unsigned int out_tmp[8192];
    // make sure the memories are clear
    for(int i = 0; i < (CAPACITY/BUCKET_SIZE); i++)
        for(int j =0; j < BUCKET_SIZE;j++)
            hash_table[i][j] = 0;
    my_assoc_mem.fill = 0;
    for(int i = 0; i < 512; i++)
    {
        my_assoc_mem.upper_key_mem[i] = 0;
        my_assoc_mem.middle_key_mem[i] = 0;
        my_assoc_mem.lower_key_mem[i] = 0;
    }

    // init the memories with the first 256 codes
    for(unsigned long i = 0; i < 256; i++)
    {
        bool collision = 0;
        unsigned int key = (i << 8) + 0UL; // lower 8 bits are the next char, the upper bits are the prefix code
        insert(hash_table, &my_assoc_mem, key, i, &collision);
    }
    int next_code = 256;


    unsigned int prefix_code = (unsigned char)s1[0];
    unsigned int code = 0;
    unsigned char next_char = 0;
    int codelength = 0; /*length of lzw code*/
    int i = 0;
while(i < length)
    {
        if(i + 1 == length)
        {
             std::cout << prefix_code;
             std::cout << " ";
out_tmp[codelength++] = (prefix_code)&0xFFF;
            break;
        }
        next_char = s1[i + 1];

        bool hit = 0;
        //std::cout << "prefix_code " << prefix_code << " next_char " << next_char << std::endl;
        lookup(hash_table, &my_assoc_mem, (prefix_code << 8) + next_char, &hit, &code);
        if(!hit)
        {
            out_tmp[codelength++] = prefix_code&0xFFF;
            std::cout << prefix_code;
            std::cout << " ";

            bool collision = 0;
            insert(hash_table, &my_assoc_mem, (prefix_code << 8) + next_char, next_code, &collision);

            if(collision)
            {
                std::cout << "ERROR: FAILED TO INSERT! NO MORE ROOM IN ASSOC MEM!" << std::endl;
                return;
            }
            next_code += 1;

            prefix_code = next_char;
        }
        else
        {
            prefix_code = code;
        }
        i += 1;
    }
    int k =0;
    cout<<"...............Code length "<<codelength<<endl;
    for(int i =0;i< codelength;i++)
    {
      /*Change the endianness of output code*/
  char data1 = (out_tmp[i] & 0x0ff0) >> 4;
      char data2 = (out_tmp[i] & 0x000f) << 4;
  out_tmp[i] = (data2<<8)|(data1);
  if(!(i & 0x01)) // Even
      {           /*lower 8-bits of 1st code*/
  output_code[k++] =(out_tmp[i] & 0xFF);
  output_code[k] = ((out_tmp[i] & 0xFF00)>>8);
      }
      else // Odd
      {
            output_code[k++] |= ((out_tmp[i] & 0x00F0)>>4);
output_code[k] = (out_tmp[i]<<4);
output_code[k++] |= ((out_tmp[i] >> 12) & 0x000F);
      }

    }
    *output_code_len = (codelength % 2 != 0) ? ((codelength/2)*3) + 2 : ((codelength/2)*3);
    // *output_code_len = k;
    // if(codelength % 2 != 0)
    // {
    //     output_code[++k] = 0;
    // }
    cout<<"output code length"<<*output_code_len<<" entries in lzw code "<<k<<endl;
    std::cout << std::endl << "assoc mem entry count: " << my_assoc_mem.fill << std::endl;
}




#endif

void handle_input(int argc, char* argv[], char** filename,int* blocksize) {
	int x;
	extern char *optarg;

	while ((x = getopt(argc, argv, ":b:f:")) != -1) {
		switch (x) {
		case 'b':
			*blocksize = atoi(optarg);
			printf("blocksize is set to %d optarg\n", *blocksize);
			break;
		case 'f':
		    *filename = optarg;
			printf("filename is %s\n", *filename);
			break;
		case ':':
			printf("-%c without parameter\n", optopt);
			break;
		}
	}
}