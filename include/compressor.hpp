#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <vector>
#include <iostream>
#include "common.h"
#include "lz4.h"
#include "zdict.h"
#include "zstd.h"

class Compressor {
protected:
	bool _mbc_enable;
	bool _rac_enable;
	CompressionAlgorithm _algorithm;
	CompressionParameter _params;
	LZ4_stream_t* _stream;
public:
	Compressor(CompressionParameter params);
	virtual ~Compressor();
	// return 0 when it is compressed and dstSize is the size of compressed data, return 1 when compressed data is larger than uncompressed data.
	virtual int compressStripe(const char* stripeBuffer, const int stripeSize, char* &dstBuffer, int dstCapacity, std::string dictAlgm = "rolling-kmer");
};

class SBCCompressor : public Compressor {
public:
	SBCCompressor(CompressionParameter params);
	virtual ~SBCCompressor();
};

class MBCCompressor : public Compressor {
public:
	MBCCompressor(CompressionParameter params);
	virtual ~MBCCompressor();
};

class RACCompressor : public Compressor {
private:
	char* _dict_buffer;
	int _dict_buffer_size;
public:
	RACCompressor(CompressionParameter params);
	virtual ~RACCompressor();
	// return 0 when it is compressed and dstSize is the size of compressed data, return 1 when compressed data is larger than uncompressed data.
	int compressStripe(const char* stripeBuffer, const int stripeSize, char* &dstBuffer, int dstCapacity, std::string dictAlgm = "rolling-kmer");
	// return dictionary size
	int generateDict(const char* stripeBuffer, const int stripeSize, char* &dictBuffer, int dictCapacity, std::string dictAlgm);
};

Compressor::Compressor(CompressionParameter params) {
	_params = params;
	_stream = LZ4_createStream();
}

Compressor::~Compressor() {}

int Compressor::compressStripe(const char* srcBuffer, const int srcSize, char* &dstBuffer, int dstCapacity, std::string dictAlgm) {
	if(dstCapacity < LZ4_compressBound(srcSize)) {
		std::cout << "ERROR: Compressor::compressStripe, dstCapacity < LZ4_compressBound(srcSize), dstCapacity is " << dstCapacity << " , LZ4_compressBound(srcSize) is " << LZ4_compressBound(srcSize) << std::endl;
	}
//	int dstSize = LZ4_compress_fast_continue(_stream, srcBuffer, dstBuffer, srcSize, dstCapacity, 1);
	std::chrono::time_point<std::chrono::system_clock> t_start = std::chrono::system_clock::now();
	int dstSize = LZ4_compress_fast(srcBuffer, dstBuffer, srcSize, dstCapacity, 1); // We should use this function instead of the above LZ4's API because SBC/MBC should not be dependent with previous block/multiple-block
	std::chrono::time_point<std::chrono::system_clock> t_end = std::chrono::system_clock::now();
	gStats.compression_timer += t_end - t_start;
	return dstSize;
}

SBCCompressor::SBCCompressor(CompressionParameter params) : Compressor(params) {
	_mbc_enable = false;
	_rac_enable = false;
	_algorithm = SBC;
	
	if(_params.number_of_blocks != 1) {
		std::cout << "ERROR: SBCCompressor but number of blocks is not 1" << std::endl;
	}
	_params.max_dict = 0;
	_params.k = 0;
	_params.d = 0;
}

SBCCompressor::~SBCCompressor() {}

MBCCompressor::MBCCompressor(CompressionParameter params) : Compressor(params) {
	_mbc_enable = true;
	_rac_enable = false;
	_algorithm = MBC;

	if(_params.number_of_blocks <= 1) {
		std::cout << "ERROR: MBCCompressor but number of blocks is less or equal to 1" << std::endl;
	}
	_params.max_dict = 0;
	_params.k = 0;
	_params.d = 0;
}

MBCCompressor::~MBCCompressor() {}

RACCompressor::RACCompressor(CompressionParameter params) : Compressor(params) {
	_mbc_enable = false;
	_rac_enable = true;
	_algorithm = RAC;

	if(_params.number_of_blocks <= 1 || _params.max_dict <= 0 || _params.k <= 0 || _params.d <= 0) {
		std::cout << "ERROR: RACCompressor but parameters are invalid" << std::endl;
	}
	_dict_buffer = new char[_params.max_dict];
	_dict_buffer_size = _params.max_dict;
}

RACCompressor::~RACCompressor() {}

int RACCompressor::compressStripe(const char* srcBuffer, const int srcSize, char* &dstBuffer, int dstCapacity, std::string dictAlgm) {
	int dstSize = 0;
	int blockSize = _params.block_size;
	int blockCapacity = LZ4_compressBound(blockSize);
	int dictCapacity = _params.max_dict;
	if(!_dict_buffer || _dict_buffer_size < _params.max_dict) {
		std::cout << "WARNING: RACCompressor::compressStripe, recreating dictionary buffer" << std::endl;
		if(_dict_buffer) {
			delete [] _dict_buffer;
		}
		_dict_buffer = new char[_params.max_dict*2];
		_dict_buffer_size = _params.max_dict;
	}
	int dictSize = generateDict(srcBuffer, srcSize, _dict_buffer, dictCapacity, dictAlgm);
	char* p = dstBuffer;
	memcpy(p, &dictSize, sizeof(int));
	p += sizeof(int);
	dstSize += sizeof(int);
	memcpy(p, _dict_buffer, dictSize);
	p += dictSize;
	dstSize += dictSize;

	const char* cur = srcBuffer;
	const char* end = cur + srcSize;

	int numberOfEntries = srcSize%blockSize==0 ? srcSize/blockSize : srcSize/blockSize+1;
	memcpy(p, &numberOfEntries, sizeof(int));
	p += sizeof(int);
	dstSize += sizeof(int); // forgot to add these 4 bytes that cost more time than I expected
	char* p1 = p;
	char* p2 = p + numberOfEntries * sizeof(StripeEntry);
	int offset = 0;
	dstSize += numberOfEntries * sizeof(StripeEntry);
	while(cur < end) {
		int distToEnd = end - cur;
		int len = distToEnd < blockSize ? distToEnd : blockSize;
		std::chrono::time_point<std::chrono::system_clock> t_start = std::chrono::system_clock::now();
		LZ4_loadDict(_stream, (const char*) _dict_buffer, dictSize);
		int cmpSize = LZ4_compress_fast_continue(_stream, cur, p2, len, blockCapacity, 1);
		std::chrono::time_point<std::chrono::system_clock> t_end = std::chrono::system_clock::now();
		gStats.compression_timer += t_end - t_start;
		if(cmpSize >= blockSize) {
			memcpy(p2, cur, blockSize);
			cmpSize = blockSize;
		}
		dstSize += cmpSize;
		StripeEntry e = {offset, len, cmpSize};
		offset += cmpSize;
		memcpy(p1, &e, sizeof(StripeEntry));
		p1 += sizeof(StripeEntry);
		p2 += cmpSize;
		cur += len;
	}

	return dstSize + sizeof(int);
}

int RACCompressor::generateDict(const char* stripeBuffer, const int stripeSize, char* &dictBuffer, int dictCapacity, std::string dictAlgm) {
	/* generate dictionary */
	ZDICT_params_t zParams;
	zParams.compressionLevel = 1;
	zParams.notificationLevel = 1;
	zParams.dictID = 0;

	ZDICT_cover_params_t* coverParams = new ZDICT_cover_params_t;
	ZDICT_legacy_params_t* legacyParams = new ZDICT_legacy_params_t;
	if(dictAlgm == "rolling-kmer") {
		coverParams->k = _params.k;
		coverParams->d = _params.d;
		coverParams->steps = 1000; // should not matter
		coverParams->nbThreads = 1;
		coverParams->zParams = zParams;
	} else if(dictAlgm == "suffix-array") {
		legacyParams->zParams = zParams;
	}
	int blockSize = _params.block_size;

	const char* cur = stripeBuffer;
	const char* end = cur + stripeSize;
	unsigned nbSamples = (stripeSize % _params.block_size == 0) ? stripeSize / _params.block_size : stripeSize / _params.block_size + 1;
	std::vector<size_t> sizeVector;
	while(cur < end) {
		size_t distToEnd = end - cur;
		size_t len = distToEnd < blockSize ? distToEnd : blockSize;
		sizeVector.push_back(len);
		cur += len;
	}
	std::chrono::time_point<std::chrono::system_clock> t_start = std::chrono::system_clock::now();
	int dictSize = -1;
	if(dictAlgm == "rolling-kmer") {
		dictSize = ZDICT_optimizeTrainFromBuffer_cover(dictBuffer, dictCapacity, (const void*) stripeBuffer, sizeVector.data(), nbSamples, coverParams);
	} else if(dictAlgm == "suffix-array") {
		dictSize = ZDICT_trainFromBuffer_legacy(dictBuffer, dictCapacity, (const void*) stripeBuffer, sizeVector.data(), nbSamples, *legacyParams);
	}
	std::chrono::time_point<std::chrono::system_clock> t_end = std::chrono::system_clock::now();
	gStats.dictionary_timer += t_end - t_start;
	gStats.total_dictionary_size += dictSize;
	return dictSize;
}

#endif
