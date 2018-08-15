#include <iostream>
#include <string>
#include <vector>
#include "common.h"
#include "filer.hpp"

ZZStats gStats;

static void show_usage(std::string name)
{
	std::cerr << "Usage: " << name << " <option(s)>\n"
		<< "Options:\n"
		<< "\t-h,--help\t\t\tShow this help message\n"
		<< "\t-t,--test\t\t\tTest case [sbc(single block compression), mbc(multiple block compression), rac(random access compression)]\n"
		<< "\t-b,--block-size\t\tSpecify the block size\n"
		<< "\t-n,--number-of-block\tSpecify how many blocks for each compressing step\n"
		<< "\t-d,--max-dict\t\tMaximum dictionary size for Random Access Compression(RAC)\n"
		<< "\t-k,--kmer-size\t\tK-mer size in Rolling K-mer algorithm to generate dictionary\n"
		<< "\t-s,--segment-size\tSegment size in Rolling K-mer algorithm to generate dicitonary\n"
		<< "\t-w,--workload\t\tWorkload type to test[random-read, sequential-read, sequential-write]\n"
		<< "\t-i,--input-file\t\tInput file name\n"
		<< "\t-o,--output-file\tOutput file name\n"
		<< std::endl;
}

static void print_csv(std::string test, int block_size, int number_of_blocks, int max_dict, int kmer_size, int segment_size, std::string wl, std::string file_in, std::string file_out, std::string dictionary_algorithm)
{
	std::cout << test << "," << block_size << "," << number_of_blocks << "," << max_dict << "," << kmer_size << "," << segment_size << "," << wl << "," << file_in << "," << file_out << ","
		<< gStats.dictionary_timer.count() << "," << gStats.compression_timer.count() << "," << gStats.decompression_timer.count() << ","
		<< gStats.total_dictionary_size << "," << gStats.total_raw_size << "," << gStats.total_compressed_size << "," << gStats.total_decompressed_size << "," << dictionary_algorithm << std::endl;
}

int main(int argc, char* argv[])
{
	if (argc < 10) {
		show_usage(argv[0]);
		return 1;
	}
	std::string test;
	int block_size = DEFAULT_BlockSize;
	int number_of_blocks = DEFAULT_NumberOfBlocks;
	int max_dict = DEFAULT_MaxDict;
	int kmer_size = DEFAULT_KmerSize;
	int segment_size = DEFAULT_SegmentSize;
	std::string wl;
	Workload workload = SequentialWrite;
	std::string file_in;
	std::string file_out;
	std::string dictionary_algorithm;
	GlobalParams params;
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if ((arg == "-h") || (arg == "--help")) {
			show_usage(argv[0]);
			return 0;
		} else if ((arg == "-t") || (arg == "--test-case")) {
			if (i + 1 < argc) {
				test = std::string(argv[++i]);
				if(test == "sbc") {
					params.algorithm = SBC;
				} else if(test == "mbc") {
					params.algorithm = MBC;
				} else if(test == "rac") {
					params.algorithm = RAC;
				}
			} else {
				std::cerr << "--test-case option require one argument." << std::endl;
			}
		} else if ((arg == "-b") || (arg == "--block-size")) {
			if (i + 1 < argc) { // Make sure we aren't at the end of argv!
				block_size = std::atoi(argv[++i]); // Increment 'i' so we don't get the argument as the next argv[i].
				params.block_size = block_size;
			} else { // Uh-oh, there was no argument to the destination option.
				std::cerr << "--block-size option requires one argument." << std::endl;
				return 1;
			}
		} else if ((arg == "-n") || (arg == "--number-of-blocks")) {
			if (i + 1 < argc) {
				number_of_blocks = std::atoi(argv[++i]);
				params.number_of_blocks = number_of_blocks;
			} else {
				std::cerr << "--number-of-blocks option requires one argument." << std::endl;
			}
		} else if ((arg == "-d") || (arg == "--max-dict")) {
			if (i + 1 < argc) {
				max_dict = std::atoi(argv[++i]);
				params.max_dict = max_dict;
			} else {
				std::cerr << "--max_dict option requires one argument." << std::endl;
			}
		} else if ((arg == "-k") || (arg == "--kmer-size")) {
			if (i + 1 < argc) {
				kmer_size = std::atoi(argv[++i]);
				params.kmer_size = kmer_size;
			} else {
				std::cerr << "--kmer-size option requires one argument." << std::endl;
			}
		} else if ((arg == "-s") || (arg == "--segment-size")) {
			if (i + 1 < argc) {
				segment_size = std::atoi(argv[++i]);
				params.segment_size = segment_size;
			} else {
				std::cerr << "--segment-size option requires one argument." << std::endl;
			}
		} else if ((arg == "-w") || (arg == "--workload")) {
			if (i + 1 < argc) {
				wl = std::string(argv[++i]);
				if(wl == "random-read") {
					workload = RandomRead;
				} else if(wl == "sequential-read") {
					workload = SequentialRead;
				} else if(wl == "sequential-write") {
					workload = SequentialWrite;
				} else {
					std::cerr << "Invalid workload" << std::endl;
					show_usage(argv[0]);
					return 1;
				}
				params.workload = workload;
			}
		} else if ((arg == "-i") || (arg == "--input-file")) {
			if (i + 1 < argc) {
				file_in = std::string(argv[++i]);
			} else {
				std::cerr << "--input-file option requires one argument." << std::endl;
			}
		} else if ((arg == "-o") || (arg == "--output-file")) {
			if (i + 1 < argc) {
				file_out = std::string(argv[++i]);
			} else {
				std::cerr << "--output-file option requires one argument." << std::endl;
			}
		} else if ((arg == "-a") || (arg == "--dictionary-algorithm")) {
			if (i + 1 < argc) {
				dictionary_algorithm = std::string(argv[++i]);
				params.dictionary_algorithm = dictionary_algorithm;
			} else {
				std::cerr << "--dictionary-algorithm option requires one argument." << std::endl;
			}
		}
	}
	Filer filer;
	filer.init(params);
	if(workload == SequentialWrite) {
		filer.compressFile(file_in, file_out);
	} else if(workload == SequentialRead) {
		filer.decompressFile(file_in, file_out);
	} else if(workload == RandomRead) {
		filer.decompressBlock(file_in, file_out);
	}
	print_csv(test, block_size, number_of_blocks, max_dict, kmer_size, segment_size, wl, file_in, file_out, dictionary_algorithm);
	return 0;
}
