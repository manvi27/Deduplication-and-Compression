#include <iostream>
#include <vector>
using namespace std;

int main()
{
	vector<unsigned int> ChunkBoundary;
	vector <char> payload;
	string s = "The Little Prince Chapter IOnce when I was six years old I saw a magnificent picture in a book, called True Stories from Nature, about the primeval forest. It was a picture of a boa constrictor in the act of swallowing an animal. Here is a copy of the drawing.BoaIn the book it said: \"Boa constrictors swallow their prey whole, without chewing it. After that they are not able to move, and they sleep through the six months that they need for digestion.\"I pondered deeply, then, over the adventures of the jungle. And after some work with a colored pencil I succeeded in making my first drawing. My Drawing Number One. It looked something like this:Hat";
    int UniqueChunkId;
    int header;
	cdc(ChunkBoundary, s ,s.length());
	//TotalChunksrcvd += ChunkBoundary.size() ;
	for(int i = 0; i < ChunkBoundary.size() - 1; i++)
	{
	    /*reference for using chunks */
		UniqueChunkId = runSHA(dedupTable, s.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),
	    ChunkBoundary[i + 1] - ChunkBoundary[i]);
		if(-1 == UniqueChunkId)
		{
			encoding(s.substr(ChunkBoundary[i],ChunkBoundary[i + 1] - ChunkBoundary[i]),payload);
			header = ((payload.size())<<1);
			// cout<<"Unique chunk\n";
		}
		else
		{
			// cout<<"Duplicate chunk\n";
			header = (((UniqueChunkId)<<1) | 1);

		}
		//memcpy(&file[offset], &header, sizeof(header));
		// cout << "-------header----------"<< header<<"=="<<(int)(*((int*)&file[offset]))<<endl;
		//offset +=  sizeof(header);
		// cout<<"chunk size = "<<ChunkBoundary[i + 1] - ChunkBoundary[i]<<"LZW size " <<payload.size()<<endl;
		//memcpy(&file[offset], &payload[0], payload.size());
		//offset +=  payload.size();
		payload.clear();
	}
	return 0;
}
