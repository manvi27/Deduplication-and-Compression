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
#include <unordered_map>
#define NUM_PACKETS 8
#define pipe_depth 4
#define DONE_BIT_L (1 << 7)
#define DONE_BIT_H (1 << 15)
#define usr_code
using namespace std;
int offset = 0;
unsigned char* file;

#ifdef usr_code

#define WIN_SIZE 16
#define PRIME 3
#define MODULUS 1024
#define TARGET 0

std::unordered_map <string, int> dedupTable;
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
    uint64_t i;
	// printf("buff length passed = %d\n",buff_size);
    uint32_t prevBoundary = ChunkBoundary.size();

	for(i = WIN_SIZE; i < buff_size - WIN_SIZE; i++)
	{
		if((hash_func(buff,i) % MODULUS) == 0 || (i - prevBoundary == 4096))
        {
			// printf("chunk boundary at%ld\n",i);
			ChunkBoundary.push_back(i);
            prevBoundary = i;
		}
	} 
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

#define CAPACITY 32768 // hash output is 15 bits, and we have 1 entry per bucket, so capacity is 2^15
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

void hash_lookup(unsigned long* hash_table, unsigned int key, bool* hit, unsigned int* result)
{
    //std::cout << "hash_lookup():" << std::endl;
    key &= 0xFFFFF; // make sure key is only 20 bits 

    unsigned long lookup = hash_table[my_hash(key)];

    // [valid][value][key]
    unsigned int stored_key = lookup&0xFFFFF;       // stored key is 20 bits
    unsigned int value = (lookup >> 20)&0xFFF;      // value is 12 bits
    unsigned int valid = (lookup >> (20 + 12))&0x1; // valid is 1 bit
    
    if(valid && (key == stored_key))
    {
        *hit = 1;
        *result = value;
        //std::cout << "\thit the hash" << std::endl;
        //std::cout << "\t(k,v,h) = " << key << " " << value << " " << my_hash(key) << std::endl;
    }
    else
    {
        *hit = 0;
        *result = 0;
        //std::cout << "\tmissed the hash" << std::endl;
    }
}

void hash_insert(unsigned long* hash_table, unsigned int key, unsigned int value, bool* collision)
{
    //std::cout << "hash_insert():" << std::endl;
    key &= 0xFFFFF;   // make sure key is only 20 bits
    value &= 0xFFF;   // value is only 12 bits

    unsigned long lookup = hash_table[my_hash(key)];
    unsigned int valid = (lookup >> (20 + 12))&0x1;

    if(valid)
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
    }
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
        *collision = 0;
        //std::cout << "\tinserted into the assoc mem" << std::endl;
        //std::cout << "\t(k,v) = " << key << " " << value << std::endl;
    }
    else
    {
        *collision = 1;
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
void insert(unsigned long* hash_table, assoc_mem* mem, unsigned int key, unsigned int value, bool* collision)
{
    hash_insert(hash_table, key, value, collision);
    if(*collision)
    {
        assoc_insert(mem, key, value, collision);
    }
}

void lookup(unsigned long* hash_table, assoc_mem* mem, unsigned int key, bool* hit, unsigned int* result)
{
    hash_lookup(hash_table, key, hit, result);
    if(!*hit)
    {
        assoc_lookup(mem, key, hit, result);
    }
}

#if KERNEL_TEST
unsigned int output_code_len;
unsigned int GetOutputCodeLen(void)
{
    return output_code_len;
}

void SetOutputCodeLen(unsigned int *val)
{
    output_code_len = val;
    return;
}

unsigned int input_code_len;
unsigned int GetInputCodeLen(void)
{
    return input_code_len;
}

void SetInputCodeLen(unsigned int val)
{
    input_code_len = val;
    return;
}
#endif

//****************************************************************************************************************
#if KERNEL_TEST
void encoding(const char* s1,char *output_code)
{
#else
void encoding(const char* s1, int length, char *output_code,unsigned int *output_code_len)
{
#endif
    // create hash table and assoc mem
    unsigned long hash_table[CAPACITY];
    assoc_mem my_assoc_mem;
    unsigned int out_tmp[4096];
    cout<<"input length"<<input_code_len<<endl;
    // cout<<"input length"<<length<<endl;
    // make sure the memories are clear
    for(int i = 0; i < CAPACITY; i++)
    {
        hash_table[i] = 0;
    }
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


    unsigned int prefix_code = s1[0];
    unsigned int code = 0;
    unsigned char next_char = 0;
    int codelength = 0; /*length of lzw code*/
    int i = 0;
    prefix_code = (s1[0]);

#if KERNEL_TEST
	while(i < GetInputCodeLen())
    {
        if(i + 1 == GetInputCodeLen())
#else
    while(i < length)
    {
        if(i + 1 == length)
#endif
        {
            // std::cout << prefix_code;
            // std::cout << " ";
			out_tmp[codelength++] = (prefix_code);
            break;
        }
        next_char = s1[i + 1];

        bool hit = 0;
        //std::cout << "prefix_code " << prefix_code << " next_char " << next_char << std::endl;
        lookup(hash_table, &my_assoc_mem, (prefix_code << 8) + next_char, &hit, &code);
        if(!hit)
        {
            out_tmp[codelength++] = prefix_code;
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
#if KERNEL_TEST
    output_code_len = (codelength % 2 != 0) ? ((codelength/2)*3) + 2 : ((codelength/2)*3);
    cout<<"output code length"<<output_code_len<<" entries in lzw code "<<k<<endl;
    std::cout << std::endl << "assoc mem entry count: " << my_assoc_mem.fill << std::endl;
#else
    *output_code_len = (codelength % 2 != 0) ? ((codelength/2)*3) + 2 : ((codelength/2)*3);
    cout<<"output code length"<<*output_code_len<<" entries in lzw code "<<k<<endl;
    std::cout << std::endl << "assoc mem entry count: " << my_assoc_mem.fill << std::endl;
#endif
    // *output_code_len = k;
    // if(codelength % 2 != 0) 
    // {
    //     output_code[++k] = 0; 
    // }
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
    unsigned int out_tmp[4096];
#if KERNEL_TEST
    cout<<"input length"<<input_code_len<<endl;
#else
     cout<<"input length"<<length<<endl;
#endif
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


    unsigned int prefix_code = s1[0];
    unsigned int code = 0;
    unsigned char next_char = 0;
    int codelength = 0; /*length of lzw code*/
    int i = 0;
    prefix_code = (s1[0]);
while(i < length)
    {
        if(i + 1 == length)
        {
             std::cout << prefix_code;
             std::cout << " ";
out_tmp[codelength++] = (prefix_code);
            break;
        }
        next_char = s1[i + 1];

        bool hit = 0;
        //std::cout << "prefix_code " << prefix_code << " next_char " << next_char << std::endl;
        lookup(hash_table, &my_assoc_mem, (prefix_code << 8) + next_char, &hit, &code);
        if(!hit)
        {
            out_tmp[codelength++] = prefix_code;
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

// int main1(int argc, char* argv[]) {
// 	stopwatch ethernet_timer;
// 	unsigned char* input[NUM_PACKETS];
// 	int writer = 0;
// 	int done = 0;
// 	int length = 0;
// 	int count = 0;
// 	ESE532_Server server;

// 	// default is 2k
// 	int blocksize = BLOCKSIZE;
// #ifdef usr_code
//    char* filename = strdup("vmlinuz.tar");
   
// #endif
// 	// set blocksize if decalred through command line
// 	handle_input(argc, argv, &filename,&blocksize);
//     FILE *outfd = fopen(filename, "wb");
//     if(outfd == NULL)
//    {
// 	    perror("invalid output file");
// 		exit(EXIT_FAILURE);
//    }
// 	file = (unsigned char*) malloc(sizeof(unsigned char) * 70000000);
// 	if (file == NULL) {
// 		printf("help\n");
// 	}
// 	printf("help cleared \n");

// 	for (int i = 0; i < NUM_PACKETS; i++) {
// 		input[i] = (unsigned char*) malloc(
// 				sizeof(unsigned char) * (NUM_ELEMENTS + HEADER));
// 		if (input[i] == NULL) {
// 			std::cout << "aborting " << std::endl;
// 			return 1;
// 		}
// 	}

// 	server.setup_server(blocksize);

// 	writer = pipe_depth;
// 	server.get_packet(input[writer]);
// 	count++;
// 	// get packet
// 	unsigned char* buffer = input[writer];

// 	// decode
// 	done = buffer[1] & DONE_BIT_L;
// 	length = buffer[0] | (buffer[1] << 8);
// 	length &= ~DONE_BIT_H;

// #ifdef usr_code
//     int TotalChunksrcvd = 0;
//     std::string input_buffer;
// 	int pos = 0;
// 	// printf("copying string");
// 	input_buffer.insert(0,(const char*)(buffer+2));
// 	pos += length;
// 	// cout << input_buffer<< endl;
// #endif

// 	// printing takes time so be weary of transfer rate
// 	//printf("length: %d offset %d\n",length,offset);

// 	// we are just memcpy'ing here, but you should call your
// 	// top function here.
// 	// memcpy(&file[offset], &buffer[HEADER], length);

// 	//offset += length;
// 	writer++;
// 	//last message
// 	while (!done) {
// 		// reset ring buffer
// 		if (writer == NUM_PACKETS) {
// 			writer = 0;
// 		}

// 		ethernet_timer.start();
// 		server.get_packet(input[writer]);
// 		ethernet_timer.stop();

// 		count++;

// 		// get packet
// 		unsigned char* buffer = input[writer];

// 		// decode
// 		done = buffer[1] & DONE_BIT_L;
// 		length = buffer[0] | (buffer[1] << 8);
// 		length &= ~DONE_BIT_H;

// #ifdef usr_code
//         vector<unsigned int> ChunkBoundary;
//         input_buffer.insert(pos,(const char*)(buffer + 2));
// 		vector <char> payload;
// 		pos += length;
// 		int UniqueChunkId; 
		
// 		int header;//header for writing back to file 
// 		if((pos >= 8192)| (done))
// 		{
// 		    // cout << "----------done value "<<done <<endl;
// 			ChunkBoundary.push_back(0);
// 			cdc(ChunkBoundary, input_buffer ,pos);
// 			if(128 == done)
// 			{
// 				ChunkBoundary.push_back(pos);
// 			}
// 			TotalChunksrcvd += ChunkBoundary.size() ;
// 			for(int i = 0; i < ChunkBoundary.size() - 1; i++)
//             {
//                 // printf("Point5\n");
//                 /*reference for using chunks */
//                 // cout <<ChunkBoundary[i + 1] - ChunkBoundary[i];
// 				UniqueChunkId = runSHA(dedupTable, input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),
//                         ChunkBoundary[i + 1] - ChunkBoundary[i]);
// 				// cout<<input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]);
// 				if(-1 == UniqueChunkId)
// 				{
// 					hardware_encoding(input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),payload);
// 					// encoding(input_buffer.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),payload);
// 					header = ((payload.size())<<1);
// 					// cout<<"Unique chunk\n";
// 				}
// 				else
// 				{
// 					// cout<<"Duplicate chunk\n";
// 					header = (((UniqueChunkId)<<1) | 1);

// 				}
// 				memcpy(&file[offset], &header, sizeof(header));
// 				// cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
// 				offset +=  sizeof(header);
// 				// cout<<"chunk size = "<<ChunkBoundary[i + 1] - ChunkBoundary[i]<<"LZW size " <<payload.size()<<endl;
// 				memcpy(&file[offset], &payload[0], payload.size());
// 				offset +=  payload.size();
// 				payload.clear();

//             }

// 			pos = pos - ChunkBoundary[ChunkBoundary.size() - 1];
// 			input_buffer = input_buffer.substr(ChunkBoundary[ChunkBoundary.size() - 1],pos);

// 		}
// #endif
// 		//printf("length: %d offset %d\n",length,offset);
// 		// memcpy(&file[offset], &buffer[HEADER], length);

// 		// offset += length;
// 		writer++;
// 	}
// 	cout<<"Total chunks rcvd = "<< (TotalChunksrcvd - 1)<< "unique chunks rcvd = "<<dedupTable.size()<<endl;
//     cout << "-------------file---------------"<<endl<<file[0];
// 	// write file to root and you can use diff tool on board
// 	// FILE *outfd = fopen("output_cpu.bin", "wb");
// 	int bytes_written = fwrite(&file[0], 1, offset, outfd);
// 	printf("write file with %d\n", bytes_written);
// 	fclose(outfd);

// 	for (int i = 0; i < NUM_PACKETS; i++) {
// 		free(input[i]);
// 	}
  
// 	free(file);
// 	std::cout << "--------------- Key Throughputs ---------------" << std::endl;
// 	float ethernet_latency = ethernet_timer.latency() / 1000.0;
// 	float input_throughput = (bytes_written * 8 / 1000000.0) / ethernet_latency; // Mb/s
// 	std::cout << "Input Throughput to Encoder: " << input_throughput << " Mb/s."
// 			<< " (Latency: " << ethernet_latency << "s)." << std::endl;

// 	return 0;
// }
