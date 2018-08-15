#include "common.h"
#include "compressor.hpp"
#include "decompressor.hpp"
#include "gtest/gtest.h"

ZZStats gStats;

class CompressionTest : public ::testing::Test {
protected:
	SBCCompressor* _sbc_c;
	MBCCompressor* _mbc_c;
	RACCompressor* _rac_c;
	SBCDecompressor* _sbc_d;
	MBCDecompressor* _mbc_d;
	RACDecompressor* _rac_d;

	virtual void SetUp() {
		CompressionParameter params;
		params.block_size = 4096;
		params.number_of_blocks = 1;
		params.max_dict = 0;
		params.k = 0;
		params.d = 0;
		_sbc_c = new SBCCompressor(params);
		_sbc_d = new SBCDecompressor(params);
		params.number_of_blocks = 4;
		_mbc_c = new MBCCompressor(params);
		_mbc_d = new MBCDecompressor(params);
		params.number_of_blocks = 256;
		params.max_dict = 4096;
		params.k = 64;
		params.d = 8;
		_rac_c = new RACCompressor(params);
		_rac_d = new RACDecompressor(params);
	}

	virtual void TearDown() {
		delete _sbc_c;
		delete _mbc_c;
		delete _rac_c;
		delete _sbc_d;
		delete _mbc_d;
		delete _rac_d;
		_sbc_c = NULL;
		_mbc_c = NULL;
		_rac_c = NULL;
		_sbc_d = NULL;
		_mbc_d = NULL;
		_rac_d = NULL;
	}
};

TEST_F(CompressionTest, SBCTestRegularSize) {
	int blockSize = 4096;
	char* srcBuffer = new char[blockSize];
	int srcSize = blockSize;
	for(int i = 0; i < blockSize; ++i) {
		srcBuffer[i] = 'A';
	}
	int blockCapacity  = LZ4_compressBound(blockSize);
	char* dstBuffer = new char[blockCapacity];
	int dstSize = _sbc_c->compressStripe(srcBuffer, srcSize, dstBuffer, blockCapacity);
	char* decBuffer = new char[blockSize];
	int decSize = _sbc_d->decompressStripe(dstBuffer, dstSize, decBuffer, blockCapacity);
	EXPECT_EQ(decSize, srcSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, decBuffer, decSize ));
	delete [] srcBuffer;
	delete [] dstBuffer;
	delete [] decBuffer;
}

TEST_F(CompressionTest, SBCTestUnregularSize) {
	int blockSize = 4096 - 100;
	char* srcBuffer = new char[blockSize];
	int srcSize = blockSize;
	for(int i = 0; i < blockSize; ++i) {
		srcBuffer[i] = 'A';
	}
	int blockCapacity  = LZ4_compressBound(blockSize);
	char* dstBuffer = new char[blockCapacity];
	int dstSize = _sbc_c->compressStripe(srcBuffer, srcSize, dstBuffer, blockCapacity);
	char* decBuffer = new char[blockSize];
	int decSize = _sbc_d->decompressStripe(dstBuffer, dstSize, decBuffer, blockCapacity);
	EXPECT_EQ(decSize, srcSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, decBuffer, decSize ));
	delete [] srcBuffer;
	delete [] dstBuffer;
	delete [] decBuffer;
}

TEST_F(CompressionTest, SBCTestRandomRead) {
	int numberOfBlocks = 1;
	int blockSize = 4096;
	int stripeSize = numberOfBlocks * blockSize;
	int srcSize = stripeSize;
	int dstCapacity = 2 * stripeSize;
	char* srcBuffer = new char[srcSize];

	for(int i = 0; i < srcSize; ++i) {
		if(i < srcSize/4) {
			srcBuffer[i] = 'A';
		} else if(i < srcSize/2) {
			srcBuffer[i] = 'A'+rand()%4;
		} else if(i < srcSize*3/4) {
			srcBuffer[i] = 'A'+rand()%13;
		} else {
			srcBuffer[i] = 'A'+rand()%26;
		}
	}
	
	char* dstBuffer = new char[dstCapacity];
	int cmpSize = _sbc_c->compressStripe(srcBuffer, srcSize, dstBuffer, dstCapacity);

	char* blkBuffer = new char[blockSize];
	for(int i = 0; i < 10; ++i) {
		int blockIdx = rand() % numberOfBlocks;
		int decSize = _sbc_d->decompressBlock(dstBuffer, cmpSize, blkBuffer, blockSize, blockIdx);
		EXPECT_EQ(decSize, blockSize);
		int offset = blockIdx * blockSize;
		EXPECT_TRUE(0 == std::memcmp( srcBuffer + offset, blkBuffer, blockSize ));
	}
	delete [] srcBuffer;
	delete [] dstBuffer;
	delete [] blkBuffer;
}


TEST_F(CompressionTest, MBCTestRegularSize) {
	int numberOfBlocks = 4;
	int blockSize = 4096;
	int srcSize = numberOfBlocks * blockSize;
	char* srcBuffer = new char[srcSize];
	for(int i = 0; i < srcSize; ++i) {
		if(i/blockSize == 0) {
			srcBuffer[i] = 'A';
		} else if(i/blockSize == 1) {
			srcBuffer[i] = 'B';
		} else if(i/blockSize == 2) {
			srcBuffer[i] = 'C';
		} else if(i/blockSize == 3) {
			srcBuffer[i] = 'D';
		}
	}
	int stripeCapacity = LZ4_compressBound(srcSize);
	char* dstBuffer = new char[stripeCapacity];
	int cmpSize = _mbc_c->compressStripe(srcBuffer, srcSize, dstBuffer, stripeCapacity);
	char* decBuffer = new char[srcSize];
	int decSize = _mbc_d->decompressStripe(dstBuffer, cmpSize, decBuffer, stripeCapacity);
	EXPECT_EQ(decSize, srcSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, decBuffer, decSize ));
	delete [] srcBuffer;
	delete [] dstBuffer;
	delete [] decBuffer;
}

TEST_F(CompressionTest, MBCTestUnregularSize) {
	int numberOfBlocks = 4;
	int blockSize = 4096;
	int srcSize = numberOfBlocks * blockSize - 100;
	char* srcBuffer = new char[srcSize];
	for(int i = 0; i < srcSize; ++i) {
		if(i/blockSize == 0) {
			srcBuffer[i] = 'A';
		} else if(i/blockSize == 1) {
			srcBuffer[i] = 'B';
		} else if(i/blockSize == 2) {
			srcBuffer[i] = 'C';
		} else if(i/blockSize == 3) {
			srcBuffer[i] = 'D';
		}
	}
	int stripeCapacity = LZ4_compressBound(srcSize);
	char* dstBuffer = new char[stripeCapacity];
	int cmpSize = _mbc_c->compressStripe(srcBuffer, srcSize, dstBuffer, stripeCapacity);
	char* decBuffer = new char[srcSize];
	int decSize = _mbc_d->decompressStripe(dstBuffer, cmpSize, decBuffer, stripeCapacity);
	EXPECT_EQ(decSize, srcSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, decBuffer, decSize ));
	delete [] srcBuffer;
	delete [] dstBuffer;
	delete [] decBuffer;
}

TEST_F(CompressionTest, MBCTestRandomRead) {
	int numberOfBlocks = 4;
	int blockSize = 4096;
	int stripeSize = numberOfBlocks * blockSize;
	int srcSize = stripeSize;
	int dstCapacity = 2 * stripeSize;
	char* srcBuffer = new char[srcSize];

	for(int i = 0; i < srcSize; ++i) {
		if(i < srcSize/4) {
			srcBuffer[i] = 'A';
		} else if(i < srcSize/2) {
			srcBuffer[i] = 'A'+rand()%4;
		} else if(i < srcSize*3/4) {
			srcBuffer[i] = 'A'+rand()%13;
		} else {
			srcBuffer[i] = 'A'+rand()%26;
		}
	}
	
	char* dstBuffer = new char[dstCapacity];
	int cmpSize = _mbc_c->compressStripe(srcBuffer, srcSize, dstBuffer, dstCapacity);

	char* blkBuffer = new char[blockSize];
	for(int i = 0; i < 10; ++i) {
		int blockIdx = rand() % numberOfBlocks;
		int decSize = _mbc_d->decompressBlock(dstBuffer, cmpSize, blkBuffer, blockSize, blockIdx);
		EXPECT_EQ(decSize, blockSize);
		int offset = blockIdx * blockSize;
		EXPECT_TRUE(0 == std::memcmp( srcBuffer + offset, blkBuffer, blockSize ));
	}
	delete [] srcBuffer;
	delete [] dstBuffer;
	delete [] blkBuffer;
}


TEST_F(CompressionTest, RACTestRegularSize) {
	int numberOfBlocks = 256;
	int blockSize = 4096;
	int srcSize = numberOfBlocks * blockSize;
	char* srcBuffer = new char[srcSize];
	for(int i = 0; i < srcSize; ++i) {
		if(i/blockSize%4 == 0) {
			srcBuffer[i] = 'A';
		} else if(i/blockSize%4 == 1) {
			srcBuffer[i] = 'B';
		} else if(i/blockSize%4 == 2) {
			srcBuffer[i] = 'C';
		} else if(i/blockSize%4 == 3) {
			srcBuffer[i] = 'D';
		}
	}
	int stripeCapacity = srcSize * 2;
	char* dstBuffer = new char[stripeCapacity];
	int cmpSize = _rac_c->compressStripe(srcBuffer, srcSize, dstBuffer, stripeCapacity);
	char* decBuffer = new char[srcSize];
	int decSize = _rac_d->decompressStripe(dstBuffer, cmpSize, decBuffer, stripeCapacity);
	EXPECT_EQ(decSize, srcSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, decBuffer, decSize ));
	delete [] srcBuffer;
	delete [] dstBuffer;
	delete [] decBuffer;
}

TEST_F(CompressionTest, RACTestUnregularSize) {
	int numberOfBlocks = 256;
	int blockSize = 4096;
	int srcSize = numberOfBlocks * blockSize + 100;
	char* srcBuffer = new char[srcSize];
	for(int i = 0; i < srcSize; ++i) {
		if(i/blockSize%4 == 0) {
			srcBuffer[i] = 'A';
		} else if(i/blockSize%4 == 1) {
			srcBuffer[i] = 'B';
		} else if(i/blockSize%4 == 2) {
			srcBuffer[i] = 'C';
		} else if(i/blockSize%4 == 3) {
			srcBuffer[i] = 'D';
		}
	}
	int stripeCapacity = srcSize * 2;
	char* dstBuffer = new char[stripeCapacity];
	int cmpSize = _rac_c->compressStripe(srcBuffer, srcSize, dstBuffer, stripeCapacity);
	char* decBuffer = new char[srcSize];
	int decSize = _rac_d->decompressStripe(dstBuffer, cmpSize, decBuffer, stripeCapacity);
	EXPECT_EQ(decSize, srcSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, decBuffer, decSize ));
	delete [] srcBuffer;
	delete [] dstBuffer;
	delete [] decBuffer;
}

TEST_F(CompressionTest, RACTestRandom) {
	int numberOfBlocks = 256;
	int blockSize = 4096;
	int srcSize = numberOfBlocks * blockSize;
	char* srcBuffer = new char[srcSize];
	for(int i = 0; i < srcSize; ++i) {
		if(i < srcSize/4) {
			srcBuffer[i] = 'A';
		} else if(i < srcSize/2) {
			srcBuffer[i] = 'A'+rand()%4;
		} else if(i < srcSize*3/4) {
			srcBuffer[i] = 'A'+rand()%13;
		} else {
			srcBuffer[i] = 'A'+rand()%26;
		}
	}
	int stripeCapacity = srcSize * 2;
	char* dstBuffer = new char[stripeCapacity];
	int cmpSize = _rac_c->compressStripe(srcBuffer, srcSize, dstBuffer, stripeCapacity);
	char* decBuffer = new char[srcSize];
	int decSize = _rac_d->decompressStripe(dstBuffer, cmpSize, decBuffer, stripeCapacity);
	EXPECT_EQ(decSize, srcSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, decBuffer, decSize ));
	delete [] srcBuffer;
	delete [] dstBuffer;
	delete [] decBuffer;
}

TEST_F(CompressionTest, RACTestPureRandom) {
	int numberOfBlocks = 256;
	int blockSize = 4096;
	int srcSize = numberOfBlocks * blockSize;
	char* srcBuffer = new char[srcSize];
	for(int i = 0; i < srcSize; ++i) {
		srcBuffer[i] = 'A'+rand()%26;
	}
	int stripeCapacity = srcSize * 2;
	char* dstBuffer = new char[stripeCapacity];
	int cmpSize = _rac_c->compressStripe(srcBuffer, srcSize, dstBuffer, stripeCapacity);
	char* decBuffer = new char[srcSize];
	int decSize = _rac_d->decompressStripe(dstBuffer, cmpSize, decBuffer, stripeCapacity);
	EXPECT_EQ(decSize, srcSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, decBuffer, decSize ));
	delete [] srcBuffer;
	delete [] dstBuffer;
	delete [] decBuffer;
}

TEST_F(CompressionTest, RACTestFile) {
	int numberOfBlocks = 256;
	int blockSize = 4096;
	int srcSize = numberOfBlocks * blockSize;
	char* srcBuffer1 = new char[srcSize];
	FILE* fp = fopen("test.in", "rb");
	int rsize = fread(srcBuffer1, 1, srcSize, fp);
	EXPECT_EQ(rsize, srcSize);
	int stripeCapacity = srcSize * 2;
	char* dstBuffer = new char[stripeCapacity];
	int cmpSize = _rac_c->compressStripe(srcBuffer1, srcSize, dstBuffer, stripeCapacity);
	FILE* fw1 = fopen("test.cmp.stripe1", "wb");
	fwrite(dstBuffer, 1, cmpSize, fw1);
	fclose(fw1);
	char* decBuffer = new char[srcSize];
	int decSize = _rac_d->decompressStripe(dstBuffer, cmpSize, decBuffer, stripeCapacity);
	fw1 = fopen("test.dec.stripe1", "wb");
	fwrite(decBuffer, 1, decSize, fw1);
	fclose(fw1);
	EXPECT_EQ(decSize, rsize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer1, decBuffer, decSize ));

	char* srcBuffer2 = new char[srcSize];
	rsize = fread(srcBuffer2, 1, srcSize, fp);
	EXPECT_EQ(rsize, srcSize);
	EXPECT_FALSE(0 == std::memcmp( srcBuffer1, srcBuffer2, srcSize ));
	cmpSize = _rac_c->compressStripe(srcBuffer2, srcSize, dstBuffer, stripeCapacity);
	FILE* fw2 = fopen("test.cmp.stripe2", "wb");
	fwrite(dstBuffer, 1, cmpSize, fw2);
	fclose(fw2);
	decSize = _rac_d->decompressStripe(dstBuffer, cmpSize, decBuffer, stripeCapacity);
	fw2 = fopen("test.dec.stripe2", "wb");
	fwrite(decBuffer, 1, decSize, fw2);
	fclose(fw2);
	EXPECT_EQ(decSize, rsize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer2, decBuffer, decSize ));
	delete [] srcBuffer1;
	delete [] srcBuffer2;
	delete [] dstBuffer;
	delete [] decBuffer;
}

TEST_F(CompressionTest, RACTestRandomRead) {
	int numberOfBlocks = 256;
	int blockSize = 4096;
	int stripeSize = numberOfBlocks * blockSize;
	int srcSize = stripeSize;
	int dstCapacity = 2 * stripeSize;
	char* srcBuffer = new char[srcSize];
	/*
	for(int i = 0; i < srcSize; ++i) {
		srcBuffer[i] = 'A';
	}
	*/
	
	for(int i = 0; i < srcSize; ++i) {
		if(i < srcSize/4) {
			srcBuffer[i] = 'A';
		} else if(i < srcSize/2) {
			srcBuffer[i] = 'A'+rand()%4;
		} else if(i < srcSize*3/4) {
			srcBuffer[i] = 'A'+rand()%13;
		} else {
			srcBuffer[i] = 'A'+rand()%26;
		}
	}
	
	char* dstBuffer = new char[dstCapacity];
	int cmpSize = _rac_c->compressStripe(srcBuffer, srcSize, dstBuffer, dstCapacity);

	char* blkBuffer = new char[blockSize];
	for(int i = 0; i < 10; ++i) {
		int blockIdx = rand() % numberOfBlocks;
		int decSize = _rac_d->decompressBlock(dstBuffer, cmpSize, blkBuffer, blockSize, blockIdx);
		EXPECT_EQ(decSize, blockSize);
		int offset = blockIdx * blockSize;
		EXPECT_TRUE(0 == std::memcmp( srcBuffer + offset, blkBuffer, blockSize ));
	}
	delete [] srcBuffer;
	delete [] dstBuffer;
	delete [] blkBuffer;
}


int main(int argc, char* argv[]) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
