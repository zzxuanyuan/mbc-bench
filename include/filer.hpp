#ifndef FILER_H
#define FILER_H

#include <vector>
#include <iostream>
#include "common.h"
#include "compressor.hpp"
#include "decompressor.hpp"

class Filer {
private:
	FILE* _fi;
	FILE* _fo;
	long long int _file_in_size;
	char* _buffer_in;
	char* _buffer_out;
	int _buffer_in_size;
	int _buffer_out_size;
	CompressionAlgorithm _algorithm;
	std::string _dictionary_algorithm;
	CompressionParameter _params;
	Compressor* _compressor;
	Decompressor* _decompressor;
public:
	Filer();
	Filer(CompressionAlgorithm algorithm);
	virtual ~Filer();
	void init(GlobalParams params);
	int stripeCompressBound(int stripeSize);
	long long int compressFile(std::string fi_name, std::string fo_name);
	long long int decompressFile(std::string fi_name, std::string fo_name);
	std::vector<int> decompressBlock(std::string fi_name, std::string fo_name);
};

Filer::Filer() {
	_buffer_in_size = -1;
	_buffer_out_size = -1;
	_buffer_in = NULL;
	_buffer_out = NULL;
	_fi = NULL;
	_fo = NULL;
	_params.block_size = -1;
	_params.number_of_blocks = -1;
	_params.max_dict = -1;
	_params.k = -1;
	_params.d = -1;
	_compressor = NULL;
	_decompressor = NULL;
}

Filer::Filer(CompressionAlgorithm algorithm) {
	_algorithm = algorithm;
	_buffer_in_size = BUFFER_SIZE;
	_buffer_out_size = BUFFER_SIZE;
	_buffer_in = NULL;
	_buffer_out = NULL;
	_fi = NULL;
	_fo = NULL;
	if(algorithm == SBC) {
		_params.block_size = SBC_BLOCK_SIZE;
		_params.number_of_blocks = SBC_NUMBER_OF_BLOCKS;
		_params.max_dict = 0;
		_params.k = 0;
		_params.d = 0;
		_compressor = new SBCCompressor(_params);
		_decompressor = new SBCDecompressor(_params);
	} else if(algorithm == MBC) {
		_params.block_size = MBC_BLOCK_SIZE;
		_params.number_of_blocks = MBC_NUMBER_OF_BLOCKS;
		_params.max_dict = 0;
		_params.k = 0;
		_params.d = 0;
		_compressor = new MBCCompressor(_params);
		_decompressor = new MBCDecompressor(_params);
	} else if(algorithm == RAC) {
		_params.block_size = RAC_BLOCK_SIZE;
		_params.number_of_blocks = RAC_NUMBER_OF_BLOCKS;
		_params.max_dict = RAC_MAX_DICT;
		_params.k = RAC_K;
		_params.d = RAC_D;
		_compressor = new RACCompressor(_params);
		_decompressor = new RACDecompressor(_params);
	}
}

Filer::~Filer() {
	if(_fi) {
		fclose(_fi);
		_fi = NULL;
	}
	if(_fo) {
		fclose(_fo);
		_fo = NULL;
	}
	if(_buffer_in) {
		delete [] _buffer_in;
		_buffer_in = NULL;
	}
	if(_buffer_out) {
		delete [] _buffer_out;
		_buffer_out = NULL;
	}
	if(_compressor) {
		delete _compressor;
		_compressor = NULL;
	}
	if(_decompressor) {
		delete _decompressor;
		_decompressor = NULL;
	}
}

void Filer::init(GlobalParams params) {
	_algorithm = params.algorithm;
	_dictionary_algorithm = params.dictionary_algorithm;
	_buffer_in_size = BUFFER_SIZE;
	_buffer_out_size = BUFFER_SIZE;
	_buffer_in = NULL;
	_buffer_out = NULL;
	_fi = NULL;
	_fo = NULL;
	_params.block_size = params.block_size;
	_params.number_of_blocks = params.number_of_blocks;
	_params.max_dict = params.max_dict;
	_params.k = params.segment_size;
	_params.d = params.kmer_size;
	if(params.algorithm == SBC) {
		if(params.number_of_blocks != 1 || params.max_dict != 0 || params.segment_size != 0 || params.kmer_size != 0) {
			std::cout << "ERROR: invalid SBC params" << std::endl;
		}
		_compressor = new SBCCompressor(_params);
		_decompressor = new SBCDecompressor(_params);
	} else if(params.algorithm == MBC) {
		if(params.number_of_blocks <= 1 || params.max_dict != 0 || params.segment_size != 0 || params.kmer_size != 0) {
			std::cout << "ERROR: invalid MBC params" << std::endl;
		}
		_compressor = new MBCCompressor(_params);
		_decompressor = new MBCDecompressor(_params);
	} else if(params.algorithm == RAC) {
		if(params.max_dict == 0 || params.segment_size == 0 || params.kmer_size == 0) {
			std::cout << "ERROR: invalid RAC params" << std::endl;
		}
		_compressor = new RACCompressor(_params);
		_decompressor = new RACDecompressor(_params);
	}
}

int Filer::stripeCompressBound(int srcSize) {
	int stripeCapacity = 0;
	int stripeSize = _params.block_size * _params.number_of_blocks;
	if(_algorithm == SBC) {
		stripeCapacity = LZ4_compressBound(srcSize);
	} else if(_algorithm == MBC) {
		stripeCapacity = LZ4_compressBound(srcSize);
	} else if(_algorithm == RAC) {
		int numberOfBlocks = (srcSize-1)/_params.block_size+1;
		int hdrSize = sizeof(int)+_params.max_dict+sizeof(int)+numberOfBlocks*sizeof(StripeEntry);
		int bodySize = numberOfBlocks * _params.block_size;
		stripeCapacity = hdrSize + bodySize;
//		stripeCapacity = 2*srcSize;
	}
	return stripeCapacity;
}

long long int Filer::compressFile(std::string fi_name, std::string fo_name) {
	_fi = fopen(fi_name.c_str(), "rb");
	_fo = fopen(fo_name.c_str(), "wb");
	if(!_fi) {
		std::cout << "ERROR: Filer::Filer, _fi is invalid" << std::endl;
	}
	fseek(_fi, 0, SEEK_END);
	_file_in_size = ftell(_fi);
	gStats.total_raw_size = _file_in_size;
	rewind(_fi);
	int stripeSize = _params.block_size * _params.number_of_blocks;
	_buffer_in_size = ((BUFFER_SIZE-1)/stripeSize+1)*stripeSize;
	_buffer_out_size = _buffer_in_size;
	if(_buffer_in) {
		delete [] _buffer_in;
	}
	_buffer_in = new char[2*_buffer_in_size];
	if(_buffer_out) {
		delete [] _buffer_out;
	}
	_buffer_out = new char[2*_buffer_out_size];
	if(!_compressor) {
		std::cout << "ERROR: Filer::compress, _compressor is invalid" << std::endl;
	}

	long long int dstSize = 0;
	int nStripes = (_file_in_size-1)/stripeSize+1;
	int hdrSize = 0;
	hdrSize += 8; // 8 bytes for SBC/MBC/RAC
	hdrSize += sizeof(CompressionParameter); // 20 bytes for _params
	hdrSize += sizeof(int); // 4 bytes for the number of stripes
	hdrSize += nStripes * sizeof(StripeHeader); // create a StripeHeader for each stripe
	char* hdrBuffer = new char[hdrSize+sizeof(int)]; // sizeof(int) bytes to store the total size of header
	char* hdrPtr = hdrBuffer;
	memcpy(hdrPtr, &hdrSize, sizeof(int));
	hdrPtr += sizeof(int);
	if(_algorithm == SBC) {
		hdrPtr[0] = 'S';
		hdrPtr[1] = 'B';
		hdrPtr[2] = 'C';
	} else if(_algorithm == MBC) {
		hdrPtr[0] = 'M';
		hdrPtr[1] = 'B';
		hdrPtr[2] = 'C';
	} else if(_algorithm == RAC) {
		hdrPtr[0] = 'R';
		hdrPtr[1] = 'A';
		hdrPtr[2] = 'C';
	}
	hdrPtr += 8;
	memcpy(hdrPtr, &_params, sizeof(CompressionParameter));
	hdrPtr += sizeof(CompressionParameter);
	memcpy(hdrPtr, &nStripes, sizeof(int));
	hdrPtr += sizeof(int);
	dstSize += sizeof(int); // do not forget to add 4 bytes at the beginnning of the file to represent the header size
	dstSize += hdrSize;
	fwrite(hdrBuffer, 1, hdrSize+sizeof(int), _fo);
	size_t rsize = 0;
	StripeHeader h;
	long long int offset = 0;
	while(1) {
		rsize = fread(_buffer_in, 1, _buffer_in_size, _fi);
		if(rsize == 0) {
			break;
		}
		const char* cur = _buffer_in;
		const char* end = _buffer_in + rsize;
		char* oPtr = _buffer_out;
		size_t wsize = 0;
		while(cur < end) {
			int srcSize = end - cur < stripeSize ? end - cur : stripeSize;
			int bound = stripeCompressBound(srcSize);
			int cmpSize = -1;
			if(_dictionary_algorithm == "rolling-kmer") {
				cmpSize = _compressor->compressStripe(cur, srcSize, oPtr, bound);
			} else if(_dictionary_algorithm == "suffix-array") {
				cmpSize = _compressor->compressStripe(cur, srcSize, oPtr, bound, "suffix-array");
			}
			if(cmpSize >= srcSize) {
				memcpy(oPtr, cur, srcSize);
				cmpSize = srcSize;
				h.offsetOfCompressedData = offset;
				h.rawStripeSize = srcSize;
				h.compressedStripeSize = cmpSize;
				memcpy(hdrPtr, &h, sizeof(StripeHeader));
			} else {
				h.offsetOfCompressedData = offset;
				h.rawStripeSize = srcSize;
				h.compressedStripeSize = cmpSize;
				memcpy(hdrPtr, &h, sizeof(StripeHeader));
			}
			hdrPtr += sizeof(StripeHeader);
			cur += srcSize;
			oPtr += cmpSize;
			offset += cmpSize;
			wsize += cmpSize;
		}
		fwrite(_buffer_out, 1, wsize, _fo);
	}
	dstSize += offset;

	rewind(_fo);
	fwrite(hdrBuffer, 1, hdrSize+sizeof(int), _fo);
	fseek(_fo, 0, SEEK_END);
	gStats.total_compressed_size = ftell(_fo);
	fclose(_fi);
	_fi = NULL;
	fclose(_fo);
	_fo = NULL;
	delete [] _buffer_in;
	_buffer_in = NULL;
	delete [] _buffer_out;
	_buffer_out = NULL;
	return dstSize;
}

long long int Filer::decompressFile(std::string fi_name, std::string fo_name) {
	_fi = fopen(fi_name.c_str(), "rb");
	_fo = fopen(fo_name.c_str(), "wb");
	if(!_fi) {
		std::cout << "ERROR: Filer::Filer, _fi is invalid" << std::endl;
	}
	fseek(_fi, 0, SEEK_END);
	_file_in_size = ftell(_fi);
	gStats.total_compressed_size = _file_in_size;
	rewind(_fi);
	int dstSize = 0;
	int hdrSize = -1;
	fread(&hdrSize, sizeof(int), 1, _fi);
	if(hdrSize < 0) {
		std::cout << "ERROR: Filer::decompressFile, hdrSize is invalid" << std::endl;
	}
	char* hdrBuffer = new char[hdrSize];
	fread(hdrBuffer, 1, hdrSize, _fi);
	const char* hdrPtr = hdrBuffer;
	const char* p = hdrPtr + 8;
	memcpy(&_params, p, sizeof(CompressionParameter));
	if(hdrPtr[0] == 'S' && hdrPtr[1] == 'B' && hdrPtr[2] == 'C') {
		_algorithm = SBC;
		_decompressor = new SBCDecompressor(_params);
	} else if(hdrPtr[0] == 'M' && hdrPtr[1] == 'B' && hdrPtr[2] == 'C') {
		_algorithm = MBC;
		_decompressor = new MBCDecompressor(_params);
	} else if(hdrPtr[0] == 'R' && hdrPtr[1] == 'A' && hdrPtr[2] == 'C') {
		_algorithm = RAC;
		_decompressor = new RACDecompressor(_params);
	}
	p += sizeof(CompressionParameter);
	int nStripes = -1;
	memcpy(&nStripes, p, sizeof(int));
	p += sizeof(int);
	std::vector<StripeHeader> stripeVector;
	StripeHeader h;
	for(int i = 0; i < nStripes; ++i) {
		memcpy(&h, p, sizeof(StripeHeader));
		stripeVector.push_back(h);
		p += sizeof(StripeHeader);
	}
	int stripeSize = _params.block_size * _params.number_of_blocks;
	_buffer_out_size = ((BUFFER_SIZE-1)/stripeSize+1)*stripeSize;
	_buffer_in_size = _buffer_out_size;
	if(_buffer_in) {
		delete [] _buffer_in;
	}
	_buffer_in = new char[_buffer_in_size*2];
	if(_buffer_out) {
		delete [] _buffer_out;
	}
	_buffer_out = new char[_buffer_out_size*2];
	long long int accuInSize = 0;
	long long int accuOutSize = 0;
	long long int offset = 0;
	int idxStart = 0;
	int idxEnd = 0;
	for(int i = 0, idxStart = 0; i < stripeVector.size(); ++i) {
		accuInSize += stripeVector[i].compressedStripeSize;
		accuOutSize += stripeVector[i].rawStripeSize;
		if(accuOutSize>=_buffer_out_size || i == stripeVector.size()-1) {
			idxEnd = i;
			int rsize = fread(_buffer_in, 1, accuInSize, _fi);
			const char* iPtr = _buffer_in;
			char* oPtr = _buffer_out;
			int decSize = 0;
			for(int m = idxStart; m <= idxEnd; ++m) {
				int decompressedSize = 0;
				if(stripeVector[m].compressedStripeSize == stripeVector[m].rawStripeSize) {
					memcpy(oPtr, iPtr, stripeVector[m].rawStripeSize);
					decompressedSize = stripeVector[m].rawStripeSize;
				} else {
					decompressedSize = _decompressor->decompressStripe(iPtr, stripeVector[m].compressedStripeSize, oPtr, stripeVector[m].rawStripeSize);
				}
				decSize += decompressedSize;
				dstSize += decompressedSize;
				iPtr += stripeVector[m].compressedStripeSize;
				oPtr += stripeVector[m].rawStripeSize;
				if(decompressedSize != stripeVector[m].rawStripeSize) {
					std::cout << "ERROR: Filer::decompressFile, decompressedSize != rawStripeSize" << std::endl;
				}
			}
			int wsize = fwrite(_buffer_out, 1, decSize, _fo);
			idxStart = idxEnd+1;
			accuInSize = 0;
			accuOutSize = 0;
		} 
	}
	fclose(_fi);
	_fi = NULL;

	fseek(_fo, 0, SEEK_END);
	gStats.total_decompressed_size = ftell(_fo);

	fclose(_fo);
	_fo = NULL;
	delete [] _buffer_in;
	_buffer_in = NULL;
	delete [] _buffer_out;
	_buffer_out = NULL;
	return dstSize;
}

/* INPUT:  fi_name -- compressed data file,
 *         fc_name -- raw data file and compare content with decompressed block data,
 *         blockNumber -- the block index in the whole file, it is different from block index that blockIdx is the index within a stripe.
 * OUTPUT: 
 */
std::vector<int> Filer::decompressBlock(std::string fi_name, std::string fo_name) {
	_fi = fopen(fi_name.c_str(), "rb");
	if(!_fi) {
		std::cout << "ERROR: Filer::Filer, _fi is invalid" << std::endl;
	}
	fseek(_fi, 0, SEEK_END);
	_file_in_size = ftell(_fi);
	gStats.total_compressed_size = _file_in_size;
	rewind(_fi);

	int dstSize = 0;
	int hdrSize = -1;
	fread(&hdrSize, sizeof(int), 1, _fi);
	if(hdrSize < 0) {
		std::cout << "ERROR: Filer::decompressFile, hdrSize is invalid" << std::endl;
	}
	char* hdrBuffer = new char[hdrSize];
	fread(hdrBuffer, 1, hdrSize, _fi);
	const char* hdrPtr = hdrBuffer;
	const char* p = hdrPtr + 8;
	memcpy(&_params, p, sizeof(CompressionParameter));
	if(hdrPtr[0] == 'S' && hdrPtr[1] == 'B' && hdrPtr[2] == 'C') {
		_algorithm = SBC;
		_decompressor = new SBCDecompressor(_params);
	} else if(hdrPtr[0] == 'M' && hdrPtr[1] == 'B' && hdrPtr[2] == 'C') {
		_algorithm = MBC;
		_decompressor = new MBCDecompressor(_params);
	} else if(hdrPtr[0] == 'R' && hdrPtr[1] == 'A' && hdrPtr[2] == 'C') {
		_algorithm = RAC;
		_decompressor = new RACDecompressor(_params);
	}
	p += sizeof(CompressionParameter);
	int nStripes = -1;
	memcpy(&nStripes, p, sizeof(int));
	p += sizeof(int);
	// calculate the total number of blocks by the very last stripe since other stripes should be capacted by number_of_blocks
	int blockInLastStripe = 0;
	if(_algorithm == SBC) {
		blockInLastStripe = 1;
	} else if(_algorithm == MBC) {
		StripeHeader h;
		/* read stripe header */
		int offset = (nStripes - 1) * sizeof(StripeHeader);
		memcpy(&h, p+offset, sizeof(StripeHeader));
		blockInLastStripe = (h.rawStripeSize-1)/_params.block_size+1;
	} else if(_algorithm == RAC) {
		StripeHeader h;
		/* read stripe header */
		int offset = (nStripes - 1) * sizeof(StripeHeader);
		memcpy(&h, p+offset, sizeof(StripeHeader));
		/* read the last stripe */
		int stripeSize = _params.block_size * _params.number_of_blocks;
		if(!_buffer_in || _buffer_in_size < stripeSize) {
			if(_buffer_in) {
				delete [] _buffer_in;
				std::cout << "WARNING: Filer::decompressBlock, _buffer_in is not valid" << std::endl;
			}
			_buffer_in = new char[stripeSize];
			_buffer_in_size = stripeSize;
		}
		if(!_buffer_out || _buffer_out_size < stripeSize) {
			if(_buffer_out) {
				delete [] _buffer_out;
				std::cout << "WARNING: Filer::decompressBlock, _buffer_out is not valid" << std::endl;
			}
			_buffer_out = new char[stripeSize];
			_buffer_out_size = stripeSize;
		}
		fseek(_fi, h.offsetOfCompressedData+hdrSize+sizeof(int), SEEK_SET);
		int cmpSize = fread(_buffer_in, 1, h.compressedStripeSize, _fi);
		if(cmpSize != h.compressedStripeSize) {
			std::cout << "ERROR: Filer::decompressBlock, cmpSize != h.compressedStripeSize" << std::endl;
		}
		if(h.compressedStripeSize == h.rawStripeSize) {
			blockInLastStripe = (h.rawStripeSize - 1) / _params.block_size + 1;
		} else {
			const char* p = _buffer_in;
			int dictSize = 0;
			memcpy(&dictSize, p, sizeof(int));
			p += sizeof(int);
			p += dictSize;
			int nBlocks = 0;
			memcpy(&nBlocks, p, sizeof(int));
			blockInLastStripe = nBlocks;
		}
	}
	int totalBlockNumber = _params.number_of_blocks * (nStripes - 1) + blockInLastStripe;

	int blockNumber = -1;
	_fo = fopen(fo_name.c_str(), "wb");
	std::vector<int> randIdxVec;
	for(int i = 0; i < totalBlockNumber; ++i) {
		blockNumber = rand() % totalBlockNumber;
		randIdxVec.push_back(blockNumber);
		int stripeIdx = blockNumber / _params.number_of_blocks;
		int blockIdx = blockNumber % _params.number_of_blocks;
		/* read stripe header */
		StripeHeader h;
		int offset = stripeIdx * sizeof(StripeHeader);
		memcpy(&h, p+offset, sizeof(StripeHeader));
		/* read the stripe [stripeIdx] */
		int stripeSize = _params.block_size * _params.number_of_blocks;
		if(!_buffer_in || _buffer_in_size < stripeSize) {
			if(_buffer_in) {
				delete [] _buffer_in;
				std::cout << "WARNING: Filer::decompressBlock, _buffer_in is not valid" << std::endl;
			}
			_buffer_in = new char[stripeSize];
			_buffer_in_size = stripeSize;
		}
		if(!_buffer_out || _buffer_out_size < stripeSize) {
			if(_buffer_out) {
				delete [] _buffer_out;
				std::cout << "WARNING: Filer::decompressBlock, _buffer_out is not valid" << std::endl;
			}
			_buffer_out = new char[stripeSize];
			_buffer_out_size = stripeSize;
		}
		fseek(_fi, h.offsetOfCompressedData+hdrSize+sizeof(int), SEEK_SET);
		int cmpSize = fread(_buffer_in, 1, h.compressedStripeSize, _fi);
		if(cmpSize != h.compressedStripeSize) {
			std::cout << "ERROR: Filer::decompressBlock, cmpSize != h.compressedStripeSize" << std::endl;
		}

		/* decompress block [blockIdx] */
		char* oPtr = _buffer_out;
		int decSize = -1;
		if(h.compressedStripeSize == h.rawStripeSize) { // Do not forget to hanle the uncompressed stripe, especiall for the case of RAC.
			int stripeOffset = blockIdx * _params.block_size;
			memcpy(oPtr, _buffer_in+stripeOffset, _params.block_size);
			decSize = _params.block_size;
		} else {
			decSize = _decompressor->decompressBlock(_buffer_in, cmpSize, oPtr, _buffer_out_size, blockIdx);
		}
		fwrite(oPtr, 1, decSize, _fo);
		dstSize += decSize;
	}

	fseek(_fo, 0, SEEK_END);
	gStats.total_decompressed_size = ftell(_fo);

	/* clean up */
	fclose(_fi);
	fclose(_fo);
	_fi = NULL;
	_fo = NULL;
	delete [] _buffer_in;
	delete [] _buffer_out;
	_buffer_in = NULL;
	_buffer_out = NULL;
	return randIdxVec;
}

#endif
