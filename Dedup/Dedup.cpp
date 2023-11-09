#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unordered_map>
#include "Dedup.h"

using namespace std;

int32_t checkDedup(string hash, std::unordered_map <std::string, int> &dedupTable, uint32_t tableSize)
{
    //cout<<dedupTable.find(hash)<<"......\n";
    //cout<<tableSize<<"-------\n";
    //cout<<dedupTable;
printf("SHAPoint3\n");
    if(dedupTable.find(hash) != dedupTable.end()) {
        int32_t pos = dedupTable.at(hash);
        cout<<"Hash found at "<< pos << "\n";
        return pos;
    } else {
        cout<<"Hash not found\n";
        dedupTable.insert(make_pair(hash, tableSize));
        cout<<"Hash added to position" <<tableSize <<"\n";
        return -1;
    }

    return -1;
}