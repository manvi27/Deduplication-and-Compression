#ifndef DEDUP_H
#define DEDUP_H

#include <stdlib.h>
#include <string.h>
#include <unordered_map>

using namespace std;

int32_t checkDedup(string hash, std::unordered_map <std::string, int> &dedupTable, uint32_t tableSize);

#endif //DEDUP_H