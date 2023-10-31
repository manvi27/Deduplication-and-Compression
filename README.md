# Deduplication-and-Compression
Repository for a compressor that can receive data in real time at modern ethernet speeds and compress it into memory using deduplication and compression. Coarse grained aproach with stages, Content-Defined Chunking, SHA-256 (or SHA3-384) hashes to screen for duplicate chunks, and LZW compression to compress non-duplicate chunks. 
