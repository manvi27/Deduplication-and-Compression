#ifndef _ENCODER_H_
#define _ENCODER_H_

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

// max number of elements we can get from ethernet
#define NUM_ELEMENTS 16384
#define HEADER 2

void cdc(vector<unsigned int> &ChunkBoundary, string buff, unsigned int buff_size);
void encoding(const char* s1,int length,char *output_code,int *output_code_len);

#endif
