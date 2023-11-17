#include <iostream>
#include <vector>
#include <fstream>
#include "../Server/encoder.h"
#include "../Decoder/Decoder.h"
using namespace std;

std::unordered_map <string, int> dedupTable1;
static std::ifstream Input;

extern uint32_t TableSize;

void decoding(vector<int> op)
{
    cout << "\nDecoding\n";
    unordered_map<int, string> table;
    for (int i = 0; i <= 255; i++) {
        string ch = "";
        ch += char(i);
        table[i] = ch;
    }
    int old = op[0], n;
    string s = table[old];
    string c = "";
    c += s[0];
    cout << s;
    int count = 256;
    for (int i = 0; i < op.size() - 1; i++) {
        n = op[i + 1];
        if (table.find(n) == table.end()) {
            s = table[old];
            s = s + c;
        }
        else {
            s = table[n];
        }
        cout << s;
        c = "";
        c += s[0];
        table[count] = table[old] + c;
        count++;
        old = n;
    }
}




int main(void)
{
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
			//encoding(s.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),payload);
			//const char *str = s.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]).c_str();
#if KERNEL_TEST
			SetInputCodeLen(ChunkBoundary[i + 1] - ChunkBoundary[i]);
			encoding(str + ChunkBoundary[i], payload);
			payloadlen = GetOutputCodeLen();
			cout<<"Chuck position : "<<ChunkBoundary[i]<<" chunk size = "<<ChunkBoundary[i + 1] - ChunkBoundary[i]<<" LZW size " <<payloadlen<<" Table Size : "<<TableSize<<endl;
//			payloadlen = (TableSize > 1) ? payloadlen + 1 : payloadlen;
			header = ((payloadlen)<<1);
			cout<<"Unique chunk ... "<<UniqueChunkId<<endl;
#else
			encoding(str + ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i],payload,&payloadlen);
			cout<<"Chuck position : "<<ChunkBoundary[i]<<" chunk size = "<<ChunkBoundary[i + 1] - ChunkBoundary[i]<<" LZW size " <<payloadlen<<" Table Size : "<<TableSize<<endl;
//			payloadlen = (TableSize > 1) ? payloadlen + 1 : payloadlen;
			header = ((payloadlen)<<1);
			cout<<"Unique chunk ... "<<UniqueChunkId<<endl;
#endif
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
#if KERNEL_TEST
		payloadlen = 0;
		SetOutputCodeLen(0);
#else
		payloadlen = 0;
#endif
	}
	myfile.close();

	/*write compressed file*/
	FILE *outfd = fopen("output_cpu.bin", "wb");
	int bytes_written = fwrite(&file[0], 1, offset, outfd);
	printf("write file with %d\n", bytes_written);
	fclose(outfd);

	char* OP_comp = "output_cpu.bin";
	char* OP_uncomp = "output_fpga.txt";
	main2(OP_comp, OP_uncomp);
	return 0;
}
