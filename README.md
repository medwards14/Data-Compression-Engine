# Data-Compression-Engine

My SIM.c program represents a data compression engine, which reads 32-bit instructions / binaries from a text file that may include up to 1024 rows.
It then parses and stores each binary, and creates a dictionary of the (up to 16) most frequent instructions. Then, depending on user input, it performs 
either code compression or code decompression. 

For code compression, the program takes each instruction read from original.txt and performs cost analysis to compress each 32-bit instruction in the most 
efficient method possible. This means it picks the compression method resulting in the lowest compressed size in bits. The compression methods themselves 
include: run-length encoding, bitmask-based compression, 1-bit mismatch, 2-bit consecutive mismatch, 4-bit consecutive mismatch, 2-bit mismatches anywhere
in the instruction, and direct match (in dictionary) compression. If none of these compression strategies are viable for a given instruction, it remains 
uncompressed and gets three bits added to the beginning to denote no compression strategy was used. The compressed binaries are stored in a bit stream,
and written to a text file called cout.txt. 

The decompression process is essentially the inverse, and uses the dictionary to match the compression strategy used for each compressed instruction. It 
then decompresses accordingly, and writes the output to dout.txt. For the test case provided (original.txt, compressed.txt), my compression functions 
accurately compress original.txt into the exact bits seen in compressed.txt, and my decompression functions decompress the contents of compressed.txt to
match the bits in original.txt exactly as well.

Instructions for use:

First, compile SIM.c using 'gcc SIM.c -o SIM'
To compress original.txt into cout.txt, run the command ./SIM 1
To decompress compressed.txt into dout.txt, run ./SIM 2
Finally, test if the bits match by running: 'diff -w -B cout.txt compression.txt' or 'diff -w B dout.txt original.txt'
