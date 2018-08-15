#ifndef COMMON_H
#define COMMON_H

#include <chrono>
#include <iostream>
#include <unordered_map>
#include <vector>

#define SBC_BLOCK_SIZE 4096
#define SBC_NUMBER_OF_BLOCKS 1
#define MBC_BLOCK_SIZE 4096
#define MBC_NUMBER_OF_BLOCKS 4
#define RAC_BLOCK_SIZE 4096
#define RAC_NUMBER_OF_BLOCKS 256
#define RAC_MAX_DICT 4096
#define RAC_K 64
#define RAC_D 8
#define BUFFER_SIZE 1048576

#define DEFAULT_BlockSize SBC_BLOCK_SIZE
#define DEFAULT_NumberOfBlocks SBC_NUMBER_OF_BLOCKS
#define DEFAULT_MaxDict RAC_MAX_DICT
#define DEFAULT_KmerSize RAC_D
#define DEFAULT_SegmentSize RAC_K

struct ZZStats {
	std::chrono::duration<double> compression_timer;
	std::chrono::duration<double> decompression_timer;
	std::chrono::duration<double> dictionary_timer;
	long long int total_dictionary_size;
	long long int total_raw_size;
	long long int total_compressed_size;
	long long int total_decompressed_size;
	int n_stripes;
	// hashmap to map stripe index to dictionary size
	std::unordered_map<int, std::vector<int> > dictionary_map;
	// hashmap to map stripe index to raw block size
	std::unordered_map<int, std::vector<int> > block_raw_map;
	// hashmap to map stripe index to compressed block size
	std::unordered_map<int, std::vector<int> > block_compressed_map;

	void print() {
		std::cout << "Time for generating dictionary (CPU):" << dictionary_timer.count() << std::endl;
		std::cout << "Time for compression (CPU):" << compression_timer.count() << std::endl;
		std::cout << "Time for decompression (CPU):" << decompression_timer.count() << std::endl;
		std::cout << "Dictionary Size: " << total_dictionary_size << std::endl;
		std::cout << "Raw Data Size: " << total_raw_size << std::endl;
		std::cout << "Compressed Data Size: " << total_compressed_size << std::endl;
		std::cout << "Decompressed Data Size: " << total_decompressed_size << std::endl;
	}
};

extern ZZStats gStats;

enum Workload {
	RandomRead,
	SequentialRead,
	SequentialWrite
};

enum CompressionAlgorithm {
	SBC,
	MBC,
	RAC
};

struct GlobalParams {
	CompressionAlgorithm algorithm;
	int block_size;
	int number_of_blocks;
	int max_dict;
	int kmer_size;
	int segment_size;
	Workload workload;
	std::string dictionary_algorithm;
};

enum StreamState {
	SS_null,
	SS_init,
	SS_fail,
	SS_continue,
	SS_done
};

struct CompressionParameter {
	int block_size;
	int number_of_blocks;
	int max_dict;
	int k; // segment size for rolling kmers
	int d; // k-mer size for rolling kmers
};

struct StripeHeader {
	long long int offsetOfCompressedData;
	int rawStripeSize;
	int compressedStripeSize;
};

struct StripeEntry {
	int offsetOfCompressedData;
	int rawBlockSize;
	int compressedBlockSize;
};

#endif
