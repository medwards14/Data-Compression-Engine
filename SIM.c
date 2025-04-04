// On my honor, I have neither given nor received unauthorized aid on this assignment - Mason Edwards
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#define maxBinaries 1024
// defining maxBits to read the 32-bit binary in each row plus the additional two characters: \n
#define maxBits 34
#define dictionaryLength 16

typedef struct binary {
	char* bits;
	int index;
	int frequency;
} Binary;

Binary createBinary(char* line, int index) {
	Binary binary;
	binary.bits = line;
	binary.index = index;
	binary.frequency = 0;
	return binary;
}

typedef struct dataBuffer {
	unsigned char* data;
	int allocationSize;
	int bitsWritten;
	int bitsRead;
} DataBuffer;

typedef struct compressionOption {
	bool valid;
	int bits;
	int order;
	int dictionaryIndex;
	int parameterOne;
	int parameterTwo;
} CompressionOption;

// function to make initial data allocation for the buffer
DataBuffer initDataBuffer(int initialAllocation) {
	DataBuffer buffer;
	buffer.data = (unsigned char*)calloc(initialAllocation, sizeof(unsigned char));

	if (buffer.data == NULL) {
		exit(EXIT_FAILURE);
	}

	buffer.allocationSize = initialAllocation;
	buffer.bitsWritten = 0;
	buffer.bitsRead = 0;
	return buffer;
}

// function to release memory allocation used for the data buffer
void releaseDataBuffer(DataBuffer buffer) {
	if (buffer.data != NULL) {
		free(buffer.data);
	}
}

// function to check if a given amount of bits can be added to a buffer
static DataBuffer validateAllocation(DataBuffer buffer, int bitsToAdd) {
	int totalBitsNeeded = buffer.bitsWritten + bitsToAdd;
	int bytesNeeded = (totalBitsNeeded + 7) / 8;

	if (bytesNeeded > buffer.allocationSize) {
		int updatedSize;
		if (buffer.allocationSize * 2 > bytesNeeded) {
			updatedSize = buffer.allocationSize * 2;
		}
		else {
			updatedSize = bytesNeeded;
		}

		unsigned char* newData = (unsigned char*)realloc(buffer.data, updatedSize);
		if (newData == NULL) {
			exit(EXIT_FAILURE);
		}

		// releasing allocated region
		memset((newData + buffer.allocationSize), 0, updatedSize - buffer.allocationSize);
		buffer.data = newData;
		buffer.allocationSize = updatedSize;
	}

	return buffer;
}

// function for pushing a single bit to a buffer
DataBuffer pushBit(DataBuffer buffer, int bit) {
	// checking if theres space
	buffer = validateAllocation(buffer, 1);
	int byteIndex = (buffer.bitsWritten / 8);
	int bitOffset = 7 - (buffer.bitsWritten % 8);

	if (bit == 1) {
		// shifting and setting the bit to 1
		buffer.data[byteIndex] |= (1 << bitOffset);
	}

	buffer.bitsWritten += 1;
	return buffer;
}

// pushing a specified amount (count) of bits from a given buffer
DataBuffer pushBits(DataBuffer buffer, unsigned int value, int count) {
	for (int i = count - 1; i >= 0; i--) {
		// using bitwise AND and shifting to push each bit
		int bit = (value >> i) & 1;
		buffer = pushBit(buffer, bit);
	}

	return buffer;
}

// pushing 32 bits from a given instruction
DataBuffer push32Bits(DataBuffer buffer, const char* instruction) {
	for (int i = 0; i < 32; i++) {
		int bit;
		if (instruction[i] == '1') {
			bit = 1;
		}
		else {
			bit = 0;
		}
		buffer = pushBit(buffer, bit);
	}

	return buffer;
}

// writing bitsWritten in lines of separate 32 bit binaries / chunks
DataBuffer writeBuffer(DataBuffer buffer, FILE* file) {
	int totalBits = buffer.bitsWritten;
	int currentBit = 0;

	while (currentBit < totalBits) {
		// if less than 32 bits remain I'll only write the bits that do remain
		int bitsToWrite = 32;
		if (totalBits - currentBit < 32) {
			bitsToWrite = (totalBits - currentBit);
		}

		unsigned int lineValue = 0;

		// for each of the next 32 bits, i = 0 represents the leftmost bit (MSB)
		for (int i = 0; i < 32; i++) {
			if (i < bitsToWrite) {
				int byteIndex = (currentBit + i) / 8;
				int bitInByte = (currentBit + i) % 8;
				int bitOffset = 7 - bitInByte;

				int bitValue = (buffer.data[byteIndex] >> bitOffset) & 1;
				if (bitValue == 1) {
					// using bitwise OR to make the bit at  31 - i equal to 1
					lineValue |= (1u << (31 - i));
				}
			}
			else {
				// no left over bits if there are less than 32 bits in this chunk
			}
		}

		// lineValue has MSB and rest of chunk so now converting it to a 32 character binary
		char str[33];
		for (int b = 31; b >= 0; b--) {
			// shifting the bits to the right by b bits, then masking out all but teh LSB
			int bit = (lineValue >> b) & 1;
			// setting this as the value
			if (bit == 1) {
				str[31 - b] = '1';
			}else {
				str[31 - b] = '0';
			}
		}
		str[32] = '\0';
		fprintf(file, "%s\n", str);

		// moving to the next chunk of bits
		currentBit += 32;
	}

	// resetting bitsWritten in case I need to reuse this
	buffer.bitsWritten = 0;
	memset(buffer.data, 0, buffer.allocationSize);

	return buffer;
}

// creating my dictionary and setting its size at address of dictionaryCounter
void createDictionary(Binary set[], int setCounter, Binary dictionary[dictionaryLength], int* dictionaryCounter) {
	// using insertion sort
	for (int i = 1; i < setCounter; i++) {
		Binary key = set[i];
		int j = i - 1;

		while (j >= 0) {
			bool needShift = false;

			if (set[j].frequency < key.frequency) {
				needShift = true;
			}
			else if (set[j].frequency == key.frequency) {
				if (set[j].index > key.index) {
					needShift = true;
				}
			}

			if (needShift == false) {
				break;
			}

			set[j + 1] = set[j];
			j--;
		}
		set[j + 1] = key;
	}

	// setting the size of the dictionary, whether its the max of 16 entries or less
	if (setCounter < dictionaryLength) {
		*dictionaryCounter = setCounter;
	}
	else {
		*dictionaryCounter = dictionaryLength;
	}

	// storing the binaries in the dictionary
	for (int i = 0; i < *dictionaryCounter; i++) {
		dictionary[i] = set[i];
	}
}

// dictionary lookup function
int findInDictionary(Binary dictionary[], int dictionaryCount, const char* instruction) {
	for (int i = 0; i < dictionaryCount; i++) {
		if (strcmp(dictionary[i].bits, instruction) == 0) {
			return i;
		}
	}

	return -1;
}

// bitmask compression function
bool checkBitMask(const char* instruction, const char* dictionaryEntry, int* startPosition, unsigned int* mask) {
	int firstMismatch = -1;

	// locating the first mismatch
	for (int i = 0; i < 32; i++) {
		if (instruction[i] != dictionaryEntry[i]) {
			firstMismatch = i;
			break;
		}
	}

	if (firstMismatch == -1) {
		// cant use bitmask becuase theres no mismatch
		return false;
	}

	// not using bitmask if the first mismatch is after index 28 either
	if (firstMismatch > 28) {
		return false;
	}

	// making sure everything up to the first mismatch does actually match
	for (int i = 0; i < firstMismatch; i++) {
		if (instruction[i] != dictionaryEntry[i]) {
			return false;
		}
	}

	// building the 4 bit mask
	unsigned int maskValue = 0;
	for (int offset = 0; offset < 4; offset++) {
		maskValue <<= 1;

		int position = firstMismatch + offset;
		if (instruction[position] != dictionaryEntry[position]) {
			maskValue |= 1;
		}
	}

	// making sure the top bit is set
	if ((maskValue & 0x8) == 0) {
		return false;
	}

	// checking that there are no mismatches after firstMismatch + 3
	for (int i = firstMismatch + 4; i < 32; i++) {
		if (instruction[i] != dictionaryEntry[i]) {
			return false;
		}
	}

	// can do botmask
	*startPosition = firstMismatch;
	*mask = maskValue;

	return true;
}

bool checkOneBitMismatch(const char* instruction, const char* dictionaryBits, int* mismatchPosition) {
	int mismatchCounter = 0;
	int position = -1;

	// looping through the 32 bit instruction to find the mismatch position
	for (int i = 0; i < 32; i++) {
		if (instruction[i] != dictionaryBits[i]) {
			mismatchCounter += 1;
			position = i;

			if (mismatchCounter > 1) {
				return false;
			}
		}
	}

	// setting the mismatch position
	if (mismatchCounter == 1) {
		*mismatchPosition = position;
		return true;
	}

	return false;
}

// if there are two consecutive bits that dont match a dictionary entry but everything else does
bool checkConsecutiveTwoBitMismatch(const char* instruction, const char* dictionaryBits, int* startPosition) {
	int mismatchCounter = 0;
	int mismatchPosition = -1;

	// looping through the 32 bit instruction to find the mismatch position
	for (int i = 0; i < 32; i++) {
		if (instruction[i] != dictionaryBits[i]) {
			mismatchCounter += 1;

			// if this is the first mismatch, I'll store it in mismatch position
			if (mismatchCounter == 1) {
				mismatchPosition = i;
			}
			else if (mismatchCounter == 2) {
				if (i != mismatchPosition + 1) {
					return false;
				}
			}
			else {
				return false;
			}
		}
	}

	// setting the mismatch position
	if (mismatchCounter == 2) {
		*startPosition = mismatchPosition;
		return true;
	}

	return false;
}

// if there is a four-bit region that differs from a dictionary entry, but everything else is the same
bool checkConsecutiveFourBitMismatch(const char* instruction, const char* dictionaryBits, int* startPosition) {
	int mismatchCounter = 0;
	int mismatchPosition = -1;

	// looping through the 32 bit instruction to find the mismatch position
	for (int i = 0; i < 32; i++) {
		if (instruction[i] != dictionaryBits[i]) {
			mismatchCounter += 1;

			if (mismatchCounter == 1) {
				mismatchPosition = i;
			}
			else {
				if (i != mismatchPosition + (mismatchCounter - 1)) {
					return false;
				}
			}
			if (mismatchCounter > 4) {
				return false;
			}
		}
	}

	// setting the mismatch position
	if (mismatchCounter == 4) {
		*startPosition = mismatchPosition;
		return true;
	}

	return false;
}

// if the instruction is different by any two bits and only those two bits
bool checkAnyTwoBitMismatch(const char* instruction, const char* dictionaryBits, int* mismatchPositionOne, int* mismatchPositionTwo) {
	int mismatchCounter = 0;
	int positionOne = -1;
	int positionTwo = -1;

	// looping through the 32 bit instruction to find the mismatch position
	for (int i = 0; i < 32; i++) {
		if (instruction[i] != dictionaryBits[i]) {
			mismatchCounter += 1;

			if (mismatchCounter == 1) {
				positionOne = i;
			}
			else if (mismatchCounter == 2) {
				positionTwo = i;
			}
			else {
				return false;
			}
		}
	}

	// setting the mismatch position
	if (mismatchCounter == 2) {
		*mismatchPositionOne = positionOne;
		*mismatchPositionTwo = positionTwo;
		return true;
	}

	return false;
}

// compressing an instruction using bitmask based-compression
DataBuffer compressBitmask(DataBuffer buffer, int startPosition, unsigned int mask, int dictionaryIndex) {
	// first 3 bits are 010
	buffer = pushBits(buffer, 0b010, 3);

	// next 5 bits are the start position
	buffer = pushBits(buffer, (unsigned int)startPosition, 5);

	// next 4 bits are the bit mask
	buffer = pushBits(buffer, mask, 4);

	// last 4 bits are the dictionary index
	buffer = pushBits(buffer, dictionaryIndex, 4);

	return buffer;
}

// this is used in my main compression function to check which of two compression strategies are more profitable
static CompressionOption selectOption(CompressionOption one, CompressionOption two) {
	if (one.valid == false) { return two; }
	if (two.valid == false) { return one; }

	if (one.bits < two.bits) {
		return one;
	}
	else if (two.bits < one.bits) {
		return two;
	}
	else {
		// if both options would have the same compressed size, I choose based on the order from the specification
		if (one.order < two.order) {
			return one;
		}
		else if (two.order < one.order) {
			return two;
		}
		else {
			// if both options are of the same format, I'll choose the one that appeared first in the original binary
			if (one.dictionaryIndex <= two.dictionaryIndex) {
				return one;
			}
			else {
				return two;
			}
		}
	}
}

// implementing run-length encoding compression
DataBuffer compressRLE(DataBuffer buffer, int dictionaryIndex, int counter) {
	// from the specification, the RLE format code is 3 bits (001)
	buffer = pushBits(buffer, 0b001, 3);

	// calling pushBits to store 001 and run length encoding
	buffer = pushBits(buffer, (counter - 1), 3);

	return buffer;
}

//
DataBuffer compressSingleInstruction(DataBuffer buffer, Binary dictionary[], int dictionaryCounter, const char* instruction, int lineNum) {
	// this is used to store the best compression strategy
	CompressionOption best;
	best.valid = false;

	CompressionOption option;

	// assuming the lowest priority - uncompressed format (000, 35 bits total). I then check the strategies in order of priority
	option.valid = true;
	option.bits = 35;
	option.order = 8;
	option.dictionaryIndex = -1;
	best = selectOption(best, option);

	// checking if direct match (111) is viable
	int directMatchIndex = findInDictionary(dictionary, dictionaryCounter, instruction);
	if (directMatchIndex >= 0) {
		// 3 bits for '111' + 4 bits for dictionary index
		option.valid = true;
		option.bits = 7;
		option.order = 7;
		option.dictionaryIndex = directMatchIndex;
		best = selectOption(best, option);
	}

	// checking if bitmask based compression (010) is viable
	bool foundBitmask = false;
	int bitmaskDictionaryIndex = -1;
	int bitmaskStartPosition = -1;
	unsigned int bitmaskValue = 0;

	for (int i = 0; i < dictionaryCounter; i++) {
		int position;
		unsigned int maskValue;
		if (checkBitMask(instruction, dictionary[i].bits, &position, &maskValue)) {
			foundBitmask = true;
			bitmaskDictionaryIndex = i;
			bitmaskStartPosition = position;
			bitmaskValue = maskValue;
			// breaking on the smallest-indexed dictionary match
			break;
		}
	}

	if (foundBitmask == true) {
		// 3 bits (010) + 5 bits (startPos) + 4 bits (mask) + 4 bits (dictionary index)
		option.valid = true;
		option.bits = 16;
		option.order = 2;  // bitmask is "2nd priority" in tie-breaking
		option.dictionaryIndex = bitmaskDictionaryIndex;
		option.parameterOne = bitmaskStartPosition;
		option.parameterTwo = bitmaskValue;
		best = selectOption(best, option);
	}

	// checking if 1 bit mismatch (011) is viable
	bool foundOneBit = false;
	int oneBitDictionaryIndex = -1;
	int mismatchPosition = -1;

	for (int i = 0; i < dictionaryCounter; i++) {
		int position;
		if (checkOneBitMismatch(instruction, dictionary[i].bits, &position)) {
			foundOneBit = true;
			oneBitDictionaryIndex = i;
			mismatchPosition = position;
			break;
		}
	}

	if (foundOneBit == true) {
		// 3 bits (011) + 5 bits (mismatch position) + 4 bits (dictionary index)
		option.valid = true;
		option.bits = 12;
		option.order = 3;
		option.dictionaryIndex = oneBitDictionaryIndex;
		option.parameterOne = mismatchPosition;
		best = selectOption(best, option);
	}

	// checking if 2 bit consecutive mismatch (100) is viable
	bool foundTwoBitC = false;
	int twoBitCDictionaryIndex = -1;
	int twoBitCStartPosition = -1;

	for (int i = 0; i < dictionaryCounter; i++) {
		int pos;
		if (checkConsecutiveTwoBitMismatch(instruction, dictionary[i].bits, &pos)) {
			foundTwoBitC = true;
			twoBitCDictionaryIndex = i;
			twoBitCStartPosition = pos;
			break;
		}
	}

	if (foundTwoBitC == true) {
		// 3 bits (100) + 5 bits (start position) + 4 bits (dictionary index)
		option.valid = true;
		option.bits = 12;
		option.order = 4;
		option.dictionaryIndex = twoBitCDictionaryIndex;
		option.parameterOne = twoBitCStartPosition;
		best = selectOption(best, option);
	}

	// checking if 4 bit consecutive mismatch (101) is viable
	bool foundFourBitC = false;
	int fourBitCDictionaryIndex = -1;
	int fourBitCStartPosition = -1;

	for (int i = 0; i < dictionaryCounter; i++) {
		int position;
		if (checkConsecutiveFourBitMismatch(instruction, dictionary[i].bits, &position)) {
			foundFourBitC = true;
			fourBitCDictionaryIndex = i;
			fourBitCStartPosition = position;
			break;
		}
	}

	if (foundFourBitC == true) {
		// 3 bits (101) + 5 bits (start position) + 4 bits (dict index)
		option.valid = true;
		option.bits = 12;
		option.order = 5;
		option.dictionaryIndex = fourBitCDictionaryIndex;
		option.parameterOne = fourBitCStartPosition;
		best = selectOption(best, option);
	}

	// checking if 2 bit mismatch anywhere (110) is viable
	bool foundAnyTwoBit = false;
	int anyTwoBitDictionaryIndex = -1;
	int position1 = -1;
	int position2 = -1;

	for (int i = 0; i < dictionaryCounter; i++) {
		int pos1, pos2;
		if (checkAnyTwoBitMismatch(instruction, dictionary[i].bits, &pos1, &pos2)) {
			foundAnyTwoBit = true;
			anyTwoBitDictionaryIndex = i;
			position1 = pos1;
			position2 = pos2;
			break;
		}
	}

	if (foundAnyTwoBit == true) {
		// 3 bits (110) + 5 (mismatch 1 position) + 5 (mismatch 2 position) + 4 bits (dictionary index)
		option.valid = true;
		option.bits = 17;
		option.order = 6;
		option.dictionaryIndex = anyTwoBitDictionaryIndex;
		option.parameterOne = position1;
		option.parameterTwo = position2;
		best = selectOption(best, option);
	}

	if (best.valid == false) {
		// using 000 + uncompressed binary
		buffer = pushBits(buffer, 0b000, 3);
		buffer = push32Bits(buffer, instruction);
		return buffer;
	}

	// writing bits to buffer after deciding which compression strategy is most profitable
	switch (best.order) {
		case 2:
			buffer = pushBits(buffer, 0b010, 3);
			buffer = pushBits(buffer, (unsigned int)best.parameterOne, 5);
			buffer = pushBits(buffer, (unsigned int)best.parameterTwo, 4);
			buffer = pushBits(buffer, best.dictionaryIndex, 4);
			break;
		case 3:
			buffer = pushBits(buffer, 0b011, 3);
			buffer = pushBits(buffer, (unsigned int)best.parameterOne, 5);
			buffer = pushBits(buffer, best.dictionaryIndex, 4);
			break;
		case 4:
			buffer = pushBits(buffer, 0b100, 3);
			buffer = pushBits(buffer, (unsigned int)best.parameterOne, 5);
			buffer = pushBits(buffer, best.dictionaryIndex, 4);
			break;
		case 5:
			buffer = pushBits(buffer, 0b101, 3);
			buffer = pushBits(buffer, (unsigned int)best.parameterOne, 5);
			buffer = pushBits(buffer, best.dictionaryIndex, 4);
			break;
		case 6:
			buffer = pushBits(buffer, 0b110, 3);
			buffer = pushBits(buffer, (unsigned int)best.parameterOne, 5);
			buffer = pushBits(buffer, (unsigned int)best.parameterTwo, 5);
			buffer = pushBits(buffer, best.dictionaryIndex, 4);
			break;
		case 7:
			buffer = pushBits(buffer, 0b111, 3);
			buffer = pushBits(buffer, best.dictionaryIndex, 4);
			break;
		case 8:
			// uncompressed
		default:
			buffer = pushBits(buffer, 0b000, 3);
			buffer = push32Bits(buffer, instruction);
			break;
	}

	return buffer;
}

// helper function to easily get the minimum bit cost to compress a given instruction
static int getCompressionBits(Binary dictionary[], int dictionaryCount, const char* instruction, int (*findinDictionary)(Binary[], int, const char*)) {
	// uncompressed
	int minimumBits = 35;

	// using the passed in function pointer so I can use my findInDictionary function to get the instructions dictionary index
	int directIndex = findinDictionary(dictionary, dictionaryCount, instruction);
	if (directIndex >= 0) {
		if (7 < minimumBits) {
			minimumBits = 7;
		}
	}

	// bitmask based compression would be 16 bits
	for (int i = 0; i < dictionaryCount; i++) {
		int position;
		unsigned int mask;

		if (checkBitMask(instruction, dictionary[i].bits, &position, &mask)) {
			if (16 < minimumBits) {
				minimumBits = 16;
			}
			// breaking after finding the first valid dictionary entry
			break;
		}
	}

	// 1 bit mismatch would be 12 bits
	for (int i = 0; i < dictionaryCount; i++) {
		int position;

		if (checkOneBitMismatch(instruction, dictionary[i].bits, &position)) {
			if (12 < minimumBits) {
				minimumBits = 12;
			}
			break;
		}
	}

	// 2 bit consecutive mismatch would be 12 bits
	for (int i = 0; i < dictionaryCount; i++) {
		int position;

		if (checkConsecutiveTwoBitMismatch(instruction, dictionary[i].bits, &position)) {
			if (12 < minimumBits) {
				minimumBits = 12;
			}
			break;
		}
	}

	// 4 bit consecutive mismatch would be 12 bits
	for (int i = 0; i < dictionaryCount; i++) {
		int position;

		if (checkConsecutiveFourBitMismatch(instruction, dictionary[i].bits, &position)) {
			if (12 < minimumBits) {
				minimumBits = 12;
			}
			break;
		}
	}

	// 2 bit anywhere mismatch would be 17 bits
	for (int i = 0; i < dictionaryCount; i++) {
		int position1, position2;

		if (checkAnyTwoBitMismatch(instruction, dictionary[i].bits, &position1, &position2)) {
			if (17 < minimumBits) {
				minimumBits = 17;
			}
			break;
		}
	}

	return minimumBits;
}

// compression function for handling all instructions
DataBuffer compressAll(DataBuffer buffer, Binary dictionary[], int dictionaryCounter, Binary binaries[], int binaryCounter) {
	int i = 0;

	// looping through every instruction read from original.txt
	while (i < binaryCounter)
	{
		// storing the current instruction to compress
		const char* currentBits = binaries[i].bits;

		int j = i + 1;
		int runLength = 1;

		// checking if there are repeated instructions following the current one and counting them
		while (j < binaryCounter && strcmp(binaries[j].bits, currentBits) == 0) {
			runLength++;
			j++;
		}

		// checking if the current instruction is stored in the dictionary 
		int dictionaryIndex = findInDictionary(dictionary, dictionaryCounter, currentBits);

		// if the instruction is not repeated in the next line(s) of original.txt or if it is not found in the dictionary, ill compress it individually
		if (runLength == 1 || dictionaryIndex < 0) {
			buffer = compressSingleInstruction(buffer, dictionary, dictionaryCounter, currentBits, i);
			i = j;
			continue;
		}

		int remainingRun = runLength;

		// since RLE should only be done for the current instruction and AT MOST the following 8 matching instructions, I will compress up to the next 9 instructions
		// only then start over with the 10th instruction to match the specification
		while (remainingRun > 9)
		{
			// compressing the first instructino that repeats using direct match
			buffer = compressSingleInstruction(buffer, dictionary, dictionaryCounter, currentBits, i);

			// then using run length encoding for the following (up to) 8 instructions
			buffer = compressRLE(buffer, dictionaryIndex, 8);

			remainingRun -= 9;
			i += 9;

			if (i >= binaryCounter) {
				return buffer;
			}

			// continuing if the next instruction is the same
			if (i < binaryCounter) {
				if (strcmp(binaries[i].bits, currentBits) != 0) {
					remainingRun = 0;
					break;
				}
			}
		}

		// continuing through original.txt instructions if current RLE compression has finished
		if (remainingRun == 0) {
			continue;
		}

		// performing compression cost comparisons for the instructions left over
		int leftover = remainingRun - 1;
		int costFirst = getCompressionBits(dictionary, dictionaryCounter, currentBits, findInDictionary);
		int chunks = (leftover + 7) / 8;
		int costRLE = costFirst + 6 * chunks;
		int costIndividual = remainingRun * costFirst;

		// if RLE is most cost effective
		if (costRLE <= costIndividual) {
			// compress the first instruction normally
			buffer = compressSingleInstruction(buffer, dictionary, dictionaryCounter, currentBits, i);

			// handling the remaining RLE instructions
			int leftOverTemp = leftover;
			while (leftOverTemp > 0) {
				int chunk;
				if (leftOverTemp > 8) {
					chunk = 8;
				} else {
					chunk = leftOverTemp;
				}

				// testing different amounts of bits left over
				if (chunk <= 2) {
					// usign direct matche
					for (int c = 0; c < chunk; c++) {
						buffer = pushBits(buffer, 0b111, 3);
						buffer = pushBits(buffer, dictionaryIndex, 4);
					}
					leftOverTemp -= chunk;
				}
				else {
					// using RLE
					buffer = compressRLE(buffer, dictionaryIndex, chunk);
					leftOverTemp -= chunk;
				}
			}
		}
		else {
			// compressing individually
			for (int c = 0; c < remainingRun; c++) {
				buffer = compressSingleInstruction(buffer, dictionary, dictionaryCounter,
					binaries[i + c].bits, (i + c));
			}
		}

		i += remainingRun;
	}

	return buffer;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// decompression functions

// checking if buffer has enough bits to read
bool canReadBits(DataBuffer buffer, int count) {
	return (buffer.bitsRead + count) <= buffer.bitsWritten;
}

// function for actually reading bits from a given buffer
DataBuffer readBits(DataBuffer buffer, int count, unsigned int* returnValue) {
	if (canReadBits(buffer, count) == false) {
		exit(EXIT_FAILURE);
	}

	unsigned int value = 0;
	for (int i = 0; i < count; i++) {
		value <<= 1;

		int byteIndex = buffer.bitsRead / 8;
		int bitOffset = 7 - (buffer.bitsRead % 8);
		int bit = (buffer.data[byteIndex] >> bitOffset) & 1;
		value |= bit;

		buffer.bitsRead++;
	}

	*returnValue = value;

	return buffer;
}

// helper function to map each compression strategy to its priority order
int mapOrder(unsigned int formatBits) {
	switch (formatBits) {
		// RLE
		case 0b001: return 1;
		// bitmask
		case 0b010: return 2;
		// 1 bit mismatch
		case 0b011: return 3;
		// 2 bit consecutive mismatch
		case 0b100: return 4;
		// 4 bit consecutive mismatch
		case 0b101: return 5;
		// 2 bit anywhere mismatch
		case 0b110: return 6;
		// direct match
		case 0b111: return 7;
		// uncompressed
		case 0b000: return 8;
		default: return -1;
	}
}

// flips a bit at its given position
void bitFlip(char* instruction, int position) {
	if (instruction[position] == '0') {
		instruction[position] = '1';
	}
	else if (instruction[position] == '1') {
		instruction[position] = '0';
	}
}

// flips up to 4 bits as needed for decompressing bitmask-based compressed instructions
void maskBits(char* base, int startPosition, unsigned int mask) {
	for (int i = 0; i < 4; i++) {
		int shift = 3 - i;
		// extracting the LSB
		int bitValue = (mask >> shift) & 1;

		if (bitValue) {
			int position = startPosition + i;
			bitFlip(base, position);
		}
	}
}

// decompression
DataBuffer decompressSingleInstruction(DataBuffer buffer, Binary dictionary[], char* lastDecoded, char* returnInstruction, CompressionOption* returnOption) {
	CompressionOption best;
	best.valid = false;
	best.bits = 0;
	best.order = -1;
	best.dictionaryIndex = -1;
	best.parameterOne = 0;
	best.parameterTwo = 0;

	// initializing the returned instruction to an empty binary
	returnInstruction[0] = '\0';

	// reading the 3 bit format code
	unsigned int formatBits;
	buffer = readBits(buffer, 3, &formatBits);

	// using my helper function to determine the priority of the passed in compressed instruction
	int order = mapOrder(formatBits);

	// validity check
	if (order < 1 || order > 8) {
		returnOption->valid = false;
		return buffer;
	}

	best.valid = true;
	best.order = order;

	// if not RLE, decoding the 32 bit line into a new decode buffer
	char decodeBuffer[33];
	memset(decodeBuffer, 0, sizeof(decodeBuffer));

	// using a switch to instruction decompression based on priority
	switch (order) {
		case 1: {
			unsigned int runLen;
			buffer = readBits(buffer, 3, &runLen);
			best.parameterOne = (int)runLen;
			break;
		} 
		case 2: {
			unsigned int startPosition, mask, dictionaryIndex;
			buffer = readBits(buffer, 5, &startPosition);
			buffer = readBits(buffer, 4, &mask);
			buffer = readBits(buffer, 4, &dictionaryIndex);

			best.dictionaryIndex = (int)dictionaryIndex;
			best.parameterOne = (int)startPosition;
			best.parameterTwo = (int)mask;

			strcpy(decodeBuffer, dictionary[dictionaryIndex].bits);
			maskBits(decodeBuffer, startPosition, mask);
			strcpy(lastDecoded, decodeBuffer);
			strcpy(returnInstruction, decodeBuffer);
			break;
		}
		case 3: {
			unsigned int mismatchPosition, dictionaryIndex3;
			buffer = readBits(buffer, 5, &mismatchPosition);
			buffer = readBits(buffer, 4, &dictionaryIndex3);

			best.dictionaryIndex = (int)dictionaryIndex3;
			best.parameterOne = (int)mismatchPosition;

			strcpy(decodeBuffer, dictionary[dictionaryIndex3].bits);
			bitFlip(decodeBuffer, mismatchPosition);
			strcpy(lastDecoded, decodeBuffer);
			strcpy(returnInstruction, decodeBuffer);
			break;
		}
		case 4: {
			unsigned int startPosition4, dictionaryIndex4;
			buffer = readBits(buffer, 5, &startPosition4);
			buffer = readBits(buffer, 4, &dictionaryIndex4);

			best.dictionaryIndex = (int)dictionaryIndex4;
			best.parameterOne = (int)startPosition4;

			strcpy(decodeBuffer, dictionary[dictionaryIndex4].bits);
			bitFlip(decodeBuffer, startPosition4);
			bitFlip(decodeBuffer, startPosition4 + 1);
			strcpy(lastDecoded, decodeBuffer);
			strcpy(returnInstruction, decodeBuffer);
			break;
		}
		case 5: {
			unsigned int startPosition5, dictionaryIndex5;
			buffer = readBits(buffer, 5, &startPosition5);
			buffer = readBits(buffer, 4, &dictionaryIndex5);

			best.dictionaryIndex = (int)dictionaryIndex5;
			best.parameterOne = (int)startPosition5;

			strcpy(decodeBuffer, dictionary[dictionaryIndex5].bits);

			for (int m = 0; m < 4; m++) {
				bitFlip(decodeBuffer, startPosition5 + m);
			}
			strcpy(lastDecoded, decodeBuffer);
			strcpy(returnInstruction, decodeBuffer);
			break;
		}
		case 6: {
			unsigned int location1, location2, dictionaryIndex6;
			buffer = readBits(buffer, 5, &location1);
			buffer = readBits(buffer, 5, &location2);
			buffer = readBits(buffer, 4, &dictionaryIndex6);

			best.dictionaryIndex = (int)dictionaryIndex6;
			best.parameterOne = (int)location1;
			best.parameterTwo = (int)location2;

			strcpy(decodeBuffer, dictionary[dictionaryIndex6].bits);
			bitFlip(decodeBuffer, location1);
			bitFlip(decodeBuffer, location2);
			strcpy(lastDecoded, decodeBuffer);
			strcpy(returnInstruction, decodeBuffer);
			break;
		}
		case 7: {
			unsigned int index7;
			buffer = readBits(buffer, 4, &index7);

			best.dictionaryIndex = (int)index7;

			strcpy(decodeBuffer, dictionary[index7].bits);
			strcpy(lastDecoded, decodeBuffer);
			strcpy(returnInstruction, decodeBuffer);
			break;
		}
		case 8: {
			char raw[33];
			for (int i = 0; i < 32; i++) {
				unsigned int bitValue;
				buffer = readBits(buffer, 1, &bitValue);

				raw[i];
				if (bitValue == 1) {
					raw[i] = '1';
				} else {
					raw[i] = '0';
				}
			}
			raw[32] = '\0';

			strcpy(decodeBuffer, raw);
			strcpy(lastDecoded, decodeBuffer);
			strcpy(returnInstruction, decodeBuffer);
			break;
		}
		default: {
			// error
			best.valid = false;
			break;
		}
	}

	// returning updated buffer with the most optimally decompressed instruction
	*returnOption = best;

	return buffer;
}

// main decompression function
DataBuffer decompressAll(DataBuffer buffer, Binary dictionary[], int dictCount, FILE* returnFile) {
	// keeping track of the last decoded instruction, and clearing it
	char lastDecoded[33];
	memset(lastDecoded, 0, sizeof(lastDecoded));

	// reading format code
	while (canReadBits(buffer, 3)) {
		// declaring option to store decompression information for current instruction
		CompressionOption option;
		option.valid = false;
		char decodedLine[33];
		decodedLine[0] = '\0';

		// decompressing a single instruction
		buffer = decompressSingleInstruction(buffer, dictionary, lastDecoded, decodedLine, &option);

		// validity check
		if (option.valid == false) {
			break;
		}

		// determining if RLE should be used
		if (option.order == 1) {
			int runLength = option.parameterOne;
			fprintf(returnFile, "%s\n", lastDecoded);

			for (int r = 0; r < runLength; r++) {
				fprintf(returnFile, "%s\n", lastDecoded);
			}
		}
		else {
			// decompressing normally
			if (decodedLine[0] != '\0') {
				fprintf(returnFile, "%s\n", decodedLine);
			}
		}
	}

	return buffer;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		fprintf(stderr, "please enter either './SIM 1' for compression, or './SIM 2' for decompression.\n");
		return -1;
	}

	int operation = atoi(argv[1]);

	//compression section --------------------------------------------------------------------------------------------------------------------------------------------
	if (operation == 1) {
		Binary binaries[maxBinaries];
		int binaryCounter = 0;

		// reading uncompressed binaries from input file (original.txt) and storing in binaries array
		FILE* binaries_ptr = fopen("original.txt", "r");
		if (binaries_ptr == NULL) { return -1; }

		char str[maxBits];

		while (fgets(str, sizeof(str), binaries_ptr) != NULL) {
			char* bits_string = strtok(str, "\n");

			if (bits_string) {
				char* bits_copy = strdup(bits_string);

				if (bits_copy == NULL) {
					fclose(binaries_ptr);
					return -1;
				}

				Binary data = createBinary(bits_copy, binaryCounter);
				binaries[binaryCounter] = data;
				binaryCounter++;
			}
		}

		//printf("number of binaries: %d\n", binaryCounter);
		fclose(binaries_ptr);

		Binary set[maxBinaries];
		int setCounter = 0;

		// creating set of unique binaries
		for (int i = 0; i < binaryCounter; i++) {
			if (i == 0) {
				set[setCounter] = binaries[i];
				setCounter++;
				continue;
			}

			// checking if current binary exists in set
			bool found = false;
			for (int j = 0; j < setCounter; j++) {
				if (strcmp(binaries[i].bits, set[j].bits) == 0) {
					found = true;
					break;
				}
			}

			// if not, push it to set and continue with next binary
			if (found == false) {
				//printf("unique binary found\n");
				set[setCounter] = binaries[i];
				setCounter++;
			}
		}

		// now calculating the frequency that each binary appears in the original file
		for (int i = 0; i < setCounter; i++) {
			int counter = 0;
			for (int j = 0; j < binaryCounter; j++) {
				if (strcmp(set[i].bits, binaries[j].bits) == 0) {
					counter += 1;
				}
			}

			set[i].frequency = counter;
		}

		//for (int i = 0; i < setCounter; i++) {
			//printf("set index %d has binary %s with frequency %d and original index %d\n", i, set[i].bits, set[i].frequency, set[i].index);
		//}

		//printf("\n");

		// creating dictionary by sorting set of unique binaries in descending order based on their frequencies and storing (up to) the first 16 in dictionary array
		Binary dictionary[dictionaryLength];
		int dictionaryCounter = 0;
		createDictionary(set, setCounter, dictionary, &dictionaryCounter);

		//for (int i = 0; i < dictionaryCounter; i++) {
			//printf("dictionary index %d has binary %s, with frequency: %d and original index %d\n", i, dictionary[i].bits, dictionary[i].frequency, dictionary[i].index);
		//}

		// creating my DataBuffer
		DataBuffer buffer = initDataBuffer(128);

		// compressing all binaries
		buffer = compressAll(buffer, dictionary, dictionaryCounter, binaries, binaryCounter);

		// writing the compressed binaries
		FILE* output = fopen("cout.txt", "w");

		if (output == NULL) {
			return -1;
		}

		buffer = writeBuffer(buffer, output);

		// writing the separation marker
		fprintf(output, "xxxx\n");

		// writing the dictionary
		for (int i = 0; i < dictionaryCounter; i++) {
			fprintf(output, "%s\n", dictionary[i].bits);
		}

		// releasing allocated memory
		releaseDataBuffer(buffer);

		fclose(output);
	}
	// decompression section --------------------------------------------------------------------------------------------------------------------------------------------
	else if (operation == 2) {
		FILE* instructions_ptr = fopen("compressed.txt", "r");
		if (instructions_ptr == NULL) {
			fprintf(stderr, "cannot open compressed.txt\n");
			return -1;
		}

		// initializing a DataBuffer this time for reading bits
		DataBuffer buffer = initDataBuffer(maxBinaries);

		bool foundMarker = false;
		char str[maxBits];

		// reading lines until xxxx
		while (fgets(str, sizeof(str), instructions_ptr)) {
			// using strtok to remove newline characters
			char* bits_string = strtok(str, "\n");
			if (bits_string == false) {
				continue;
			}

			if (strcmp(bits_string, "xxxx") == 0) {
				foundMarker = true;
				break;
			}

			// parsing decompressed instruction line
			for (int i = 0; i < 32; i++) {
				int bit = (bits_string[i] == '1') ? 1 : 0;
				buffer = pushBit(buffer, bit);
			}
		}

		if (foundMarker == false) {
			fclose(instructions_ptr);
			return -1;
		}

		// 4) Now read up to 16 dictionary lines (each is a 32-bit pattern)
		Binary dictionary[16];
		int dictionaryCount = 0;

		while (dictionaryCount < 16 && fgets(str, sizeof(str), instructions_ptr)) {
			char* bits_string = strtok(str, "\n");
			
			if (bits_string == false || strlen(bits_string) == 0) {
				break;
			}

			// storing this as a dictionary entry
			dictionary[dictionaryCount].bits = strdup(bits_string);
			dictionary[dictionaryCount].index = dictionaryCount;
			dictionary[dictionaryCount].frequency = 0;
			dictionaryCount++;
		}

		fclose(instructions_ptr);

		// writing the decompressed data to dout.txt
		FILE* original_ptr = fopen("dout.txt", "w");
		if (original_ptr == NULL) {
			releaseDataBuffer(buffer);
			return -1;
		}

		// calling my decompression function
		buffer = decompressAll(buffer, dictionary, dictionaryCount, original_ptr);

		fclose(original_ptr);

		// release allocated buffer memory
		releaseDataBuffer(buffer);
	}

	return 0;
}

