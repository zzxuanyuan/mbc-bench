#ifndef DECOMPRESSOR_H
#define DECOMPRESSOR_H

#include <iostream>
#include "common.h"
#include "lz4.h"
#include "zdict.h"
#include "zstd.h"

class Decompressor {
protected:
	bool _mbc_enable;
	bool _rac_enable;
	CompressionAlgorithm _algorithm;
	CompressionParameter _params;
	LZ4_streamDecode_t* _stream;
public:
	Decompressor(CompressionParameter params);
	void resetStream();
	virtual ~Decompressor();
	virtual char* getDictBuffer();
	virtual int getDictBufferSize();
	// return decompressed size; we use LZ4_decompress_safe_continue in this function but we know dstCapacity is the same as return value;
	virtual int decompressStripe(const char* stripeBuffer, const int stripeSize, char* &dstBuffer, int dstCapacity);
	virtual int decompressBlock(const char* stripeBuffer, const int stripeSize, char* &dstBuffer, int dstCapacity, int blockIdx);
};

class SBCDecompressor : public Decompressor {
public:
	SBCDecompressor(CompressionParameter params);
	virtual ~SBCDecompressor();
	int decompressBlock(const char* stripeBuffer, const int stripeSize, char* &dstBuffer, int dstCapacity, int blockIdx);
};

class MBCDecompressor : public Decompressor {
public:
	MBCDecompressor(CompressionParameter params);
	virtual ~MBCDecompressor();
	int decompressBlock(const char* stripeBuffer, const int stripeSize, char* &dstBuffer, int dstCapacity, int blockIdx);
};

class RACDecompressor : public Decompressor {
private:
	char* _dict_buffer;
	int _dict_buffer_size;
public:
	RACDecompressor(CompressionParameter params);
	virtual ~RACDecompressor();
	char* getDictBuffer();
	int getDictBufferSize();
	// return decompressed size; we use LZ4_decompress_safe_continue in this function but we know dstCapacity is the same as return value;
	int decompressStripe(const char* stripeBuffer, const int stripeSize, char* &dstBuffer, int dstCapacity);
	int decompressBlock(const char* stripeBuffer, const int stripeSize, char* &dstBuffer, int dstCapacity, int blockIdx);
};

Decompressor::Decompressor(CompressionParameter params) {
	_params = params;
	_stream = LZ4_createStreamDecode();
}

void Decompressor::resetStream() {
	if(_stream) {
		delete _stream;
		_stream = NULL;
	}
	_stream = LZ4_createStreamDecode();
}

Decompressor::~Decompressor() {}

char* Decompressor::getDictBuffer() { return NULL; }
int Decompressor::getDictBufferSize() { return -1; }

int Decompressor::decompressStripe(const char* srcBuffer, const int srcSize, char* &dstBuffer, int dstCapacity) {
//	int decSize = LZ4_decompress_safe_continue (_stream, srcBuffer, dstBuffer, srcSize, dstCapacity);
	std::chrono::time_point<std::chrono::system_clock> t_start = std::chrono::system_clock::now();
	int decSize = LZ4_decompress_safe (srcBuffer, dstBuffer, srcSize, dstCapacity); // We should use this LZ4's API instead of the above one because SBC/MBC should not be dependent with previous block/multiple-block
	std::chrono::time_point<std::chrono::system_clock> t_end = std::chrono::system_clock::now();
	gStats.decompression_timer += t_end - t_start;
	return decSize;
}

int Decompressor::decompressBlock(const char* srcBuffer, const int srcSize, char* &dstBuffer, int dstCapacity, int blockIdx) {
	std::cout << "WARNING: Decompressor::decompressBlock should not be called" << std::endl;
	int decSize = LZ4_decompress_safe_continue (_stream, srcBuffer, dstBuffer, srcSize, dstCapacity);
	return decSize;
}

SBCDecompressor::SBCDecompressor(CompressionParameter params) : Decompressor(params) {
	_mbc_enable = false;
	_rac_enable = false;
	_algorithm = SBC;
	if(_params.number_of_blocks != 1) {
		std::cout << "ERROR: SBCDecompressor number of block is not 1" << std::endl;
	}
}

SBCDecompressor::~SBCDecompressor() {}

int SBCDecompressor::decompressBlock(const char* srcBuffer, const int srcSize, char* &dstBuffer, int dstCapacity, int blockIdx) {
	if(blockIdx != 0) {
		std::cout << "ERROR: SBCDecompressor::decompressBlock, blockIdx != 0" << std::endl;
	}
//	int decSize = LZ4_decompress_safe_continue (_stream, srcBuffer, dstBuffer, srcSize, dstCapacity);
	std::chrono::time_point<std::chrono::system_clock> t_start = std::chrono::system_clock::now();
	int decSize = LZ4_decompress_safe (srcBuffer, dstBuffer, srcSize, dstCapacity);
	std::chrono::time_point<std::chrono::system_clock> t_end = std::chrono::system_clock::now();
	gStats.decompression_timer += t_end - t_start;
	return decSize;
}

MBCDecompressor::MBCDecompressor(CompressionParameter params) : Decompressor(params) {
	_mbc_enable = true;
	_rac_enable = false;
	_algorithm = MBC;
	if(_params.number_of_blocks <= 1) {
		std::cout << "ERROR: MBCDecompressor number of block is less or equal to 1" << std::endl;
	}
}

MBCDecompressor::~MBCDecompressor() {}

int MBCDecompressor::decompressBlock(const char* srcBuffer, const int srcSize, char* &dstBuffer, int dstCapacity, int blockIdx) {
	if(blockIdx < 0 || blockIdx > _params.number_of_blocks-1) {
		std::cout << "ERROR: MBCDecompressor::decompressBlock, blockIdx is not within range" << std::endl;
	}
	int stripeSize = _params.block_size * _params.number_of_blocks;
	int decSize = -1;
	if(dstCapacity < stripeSize) {
		std::cout << "WARNING: MBCDecompressor::decompressBlock, dstCapacity < stripeSize" << std::endl;
		char* buffer = NULL;
		buffer = new char[stripeSize];
//		decSize = LZ4_decompress_safe_continue (_stream, srcBuffer, buffer, srcSize, stripeSize);
		std::chrono::time_point<std::chrono::system_clock> t_start = std::chrono::system_clock::now();
		decSize = LZ4_decompress_safe (srcBuffer, buffer, srcSize, stripeSize);
		std::chrono::time_point<std::chrono::system_clock> t_end = std::chrono::system_clock::now();
		gStats.decompression_timer += t_end - t_start;
		/* last stripe does not satisfy this condition
		if(decSize != stripeSize) {
			std::cout << "WARNING: MBCDecompressor::decompressBlock, decSize != stripeSize" << std::endl;
		}
		*/
		int offset = blockIdx * _params.block_size;
		memcpy(dstBuffer, buffer+offset, _params.block_size);
		delete [] buffer;
	} else {
//		decSize = LZ4_decompress_safe_continue (_stream, srcBuffer, dstBuffer, srcSize, dstCapacity);
		std::chrono::time_point<std::chrono::system_clock> t_start = std::chrono::system_clock::now();
		decSize = LZ4_decompress_safe (srcBuffer, dstBuffer, srcSize, dstCapacity);
		std::chrono::time_point<std::chrono::system_clock> t_end = std::chrono::system_clock::now();
		gStats.decompression_timer += t_end - t_start;
		/*
		if(decSize != stripeSize) {
			std::cout << "WARNING: MBCDecompressor::decompressBlock, decSize != stripeSize" << std::endl;
		}
		*/
		int offset = blockIdx * _params.block_size;
		dstBuffer += offset;
	}
	decSize = _params.block_size;
	return decSize;
}

RACDecompressor::RACDecompressor(CompressionParameter params) : Decompressor(params) {
	_mbc_enable = false;
	_rac_enable = true;
	_algorithm = RAC;
	if(_params.number_of_blocks <= 1) {
		std::cout << "ERROR: RACDecompressor number of block is less or equal to 1" << std::endl;
	}
	_dict_buffer = new char[_params.max_dict];
	_dict_buffer_size = _params.max_dict;
}

RACDecompressor::~RACDecompressor() {}

char* RACDecompressor::getDictBuffer() {
	return _dict_buffer;
}

int RACDecompressor::getDictBufferSize() {
	return _dict_buffer_size;
}

int RACDecompressor::decompressStripe(const char* srcBuffer, const int srcSize, char* &dstBuffer, int dstCapacity) {
	int dstSize = 0;
	int blockSize = _params.block_size;
	const char* p = srcBuffer;
	int dictSize = -1;
	memcpy(&dictSize, p, sizeof(int));
	p += sizeof(int);
	if(!_dict_buffer || _dict_buffer_size < dictSize) {
		std::cout << "WARNING: RACDecompressor::decompressStripe, recreating dictionary buffer" << std::endl;
		if(_dict_buffer) {
			delete [] _dict_buffer;
		}
		_dict_buffer = new char[dictSize];
		_dict_buffer_size = dictSize;
	}
	memcpy(_dict_buffer, p, dictSize);
	p += dictSize;
	int nBlocks = -1;
	memcpy(&nBlocks, p, sizeof(int));
	p += sizeof(int);
	std::vector<StripeEntry> entries;
	StripeEntry e;
	for(int i = 0; i < nBlocks; ++i) {
		memcpy(&e, p, sizeof(StripeEntry));
		entries.push_back(e);
		p += sizeof(StripeEntry);
	}
	
	int decompressedSize = 0;
	char* dstPtr = dstBuffer;
	for(int i = 0; i < nBlocks; ++i) {
		if(entries[i].compressedBlockSize == entries[i].rawBlockSize) {
			memcpy(dstPtr, p, entries[i].rawBlockSize);
			decompressedSize = entries[i].rawBlockSize;
		} else {
			std::chrono::time_point<std::chrono::system_clock> t_start = std::chrono::system_clock::now();
			decompressedSize = LZ4_decompress_safe_usingDict((const char*) p, dstPtr, entries[i].compressedBlockSize, blockSize, _dict_buffer, dictSize);
			std::chrono::time_point<std::chrono::system_clock> t_end = std::chrono::system_clock::now();
			gStats.decompression_timer += t_end - t_start;
		}
		p += entries[i].compressedBlockSize;
		dstPtr += decompressedSize;
		dstSize += decompressedSize;
	}
//	if(dstSize != dstCapacity) {
//		std::cout << "ERROR: RACDecompressor::decompressStripe, dstSize != dstCapacity" << std::endl;
//	}
	return dstSize;
}

int RACDecompressor::decompressBlock(const char* srcBuffer, const int srcSize, char* &dstBuffer, int dstCapacity, int blockIdx) {
	int dstSize = 0;
	int blockSize = _params.block_size;
	const char* p = srcBuffer;
	int dictSize = 0;
	memcpy(&dictSize, p, sizeof(int));
	p += sizeof(int);
	if(!_dict_buffer || _dict_buffer_size < dictSize) {
		if(_dict_buffer) {
			delete [] _dict_buffer;
		}
		_dict_buffer = new char[dictSize];
		_dict_buffer_size = dictSize;
	}
	memcpy(_dict_buffer, p, dictSize);
	p += dictSize;
	int nBlocks = 0;
	memcpy(&nBlocks, p, sizeof(int));
	p += sizeof(int);
	StripeEntry entry;
	memcpy(&entry, p+blockIdx*sizeof(StripeEntry), sizeof(StripeEntry));
	p += nBlocks*sizeof(StripeEntry);
	
	int decompressedSize = 0;
	int offset = entry.offsetOfCompressedData;
	if(entry.compressedBlockSize == entry.rawBlockSize) {
		memcpy(dstBuffer, p+offset, entry.rawBlockSize);
		decompressedSize = entry.rawBlockSize;
	} else {
		std::chrono::time_point<std::chrono::system_clock> t_start = std::chrono::system_clock::now();
		decompressedSize = LZ4_decompress_safe_usingDict((const char*) p+offset, dstBuffer, entry.compressedBlockSize, blockSize, _dict_buffer, dictSize);
		std::chrono::time_point<std::chrono::system_clock> t_end = std::chrono::system_clock::now();
		gStats.decompression_timer += t_end - t_start;
	}
//	if(dstSize != blockSize) {
//		std::cout << "ERROR: RACDecompressor::decompressBlock, dstSize != block" << std::endl;
//	}
	dstSize = decompressedSize;
	return dstSize;
}

#endif
