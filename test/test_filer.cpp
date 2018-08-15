#include "common.h"
#include "filer.hpp"
#include "compressor.hpp"
#include "decompressor.hpp"
#include "gtest/gtest.h"

ZZStats gStats;

class FilerTest : public ::testing::Test {
protected:
	Filer* _sbc_filer;
	Filer* _mbc_filer;
	Filer* _rac_filer;

	virtual void SetUp() {
		_sbc_filer = new Filer(SBC);
		_mbc_filer = new Filer(MBC);
		_rac_filer = new Filer(RAC);
	}

	virtual void TearDown() {
		delete _sbc_filer;
		delete _mbc_filer;
		delete _rac_filer;
		_sbc_filer = NULL;
		_mbc_filer = NULL;
		_rac_filer = NULL;
	}
};

TEST_F(FilerTest, TestMemcmp) {
	long long int fileSize = 1024*1024*5;
	char* buffer1 = new char[fileSize];
	char* buffer2 = new char[fileSize];
	for(long long int i = 0; i < fileSize; ++i) {
		char ch = 'A'+rand()%26;
		buffer1[i] = ch;
		buffer2[i] = ch;
	}
	EXPECT_TRUE(0 == memcmp(buffer1, buffer2, fileSize));
	delete [] buffer1;
	delete [] buffer2;
};

TEST_F(FilerTest, SBCTestRegularSize) {
	FILE* fp = fopen("test.in", "wb");
	long long int fileSize = 1024*1024*5;
	char* buffer = new char[fileSize];
	for(int i = 0; i < fileSize; ++i) {
		if(i < fileSize/3) {
			buffer[i] = 'A';
		} else if(i < 2*fileSize/3) {
			buffer[i] = 'A'+rand()%4;
		} else {
			buffer[i] = 'A'+rand()%26;
		}
	}
	fwrite(buffer, 1, fileSize, fp);
	fclose(fp);
	delete [] buffer;
	buffer = NULL;
	std::string fi_name = "test.in";
	std::string fo_name = "test.out";
	std::string fd_name = "test.dec";
	long long int compressedSize = _sbc_filer->compressFile(fi_name, fo_name);
	long long int decompressedSize = _sbc_filer->decompressFile(fo_name, fd_name);
	EXPECT_EQ(decompressedSize, fileSize);
	long long int srcSize = 0;
	long long int dstSize = 0;
	fp = fopen("test.in", "rb");
	fseek(fp, 0, SEEK_END);
	srcSize = ftell(fp);
	char* srcBuffer = new char[srcSize];
	rewind(fp);
	fread(srcBuffer, 1, srcSize, fp);
	fclose(fp);
	fp = NULL;
	fp = fopen("test.dec", "rb");
	fseek(fp, 0, SEEK_END);
	dstSize = ftell(fp);
	char* dstBuffer = new char[dstSize];
	rewind(fp);
	fread(dstBuffer, 1, dstSize, fp);
	fclose(fp);
	fp = NULL;
	EXPECT_EQ(fileSize, srcSize);
	EXPECT_EQ(fileSize, dstSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, dstBuffer, fileSize ));
	delete [] srcBuffer;
	srcBuffer = NULL;
	delete [] dstBuffer;
	dstBuffer = NULL;
}

TEST_F(FilerTest, SBCTestUnRegularSize) {
	FILE* fp = fopen("test.in", "wb");
	long long int fileSize = 1024*1024*5+100;
	char* buffer = new char[fileSize];
	for(int i = 0; i < fileSize; ++i) {
		if(i < fileSize/3) {
			buffer[i] = 'A';
		} else if(i < 2*fileSize/3) {
			buffer[i] = 'A'+rand()%4;
		} else {
			buffer[i] = 'A'+rand()%26;
		}
	}
	fwrite(buffer, 1, fileSize, fp);
	fclose(fp);
	fp = NULL;
	delete [] buffer;
	buffer = NULL;
	std::string fi_name = "test.in";
	std::string fo_name = "test.out";
	std::string fd_name = "test.dec";
	long long int compressedSize = _sbc_filer->compressFile(fi_name, fo_name);
	long long int decompressedSize = _sbc_filer->decompressFile(fo_name, fd_name);
	EXPECT_EQ(decompressedSize, fileSize);
	long long int srcSize = 0;
	long long int dstSize = 0;
	fp = fopen("test.in", "rb");
	fseek(fp, 0, SEEK_END);
	srcSize = ftell(fp);
	char* srcBuffer = new char[srcSize];
	rewind(fp);
	fread(srcBuffer, 1, srcSize, fp);
	fclose(fp);
	fp = NULL;
	fp = fopen("test.dec", "rb");
	fseek(fp, 0, SEEK_END);
	dstSize = ftell(fp);
	char* dstBuffer = new char[dstSize];
	rewind(fp);
	fread(dstBuffer, 1, dstSize, fp);
	fclose(fp);
	EXPECT_EQ(fileSize, srcSize);
	EXPECT_EQ(fileSize, dstSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, dstBuffer, fileSize ));
	delete [] srcBuffer;
	srcBuffer = NULL;
	delete [] dstBuffer;
	dstBuffer = NULL;
}

TEST_F(FilerTest, SBCTestDecompressBlock) {
	FILE* fi = fopen("test.in", "wb");
	long long int fileSize = 1024*1024*5;
	char* buffer = new char[fileSize];
	for(int i = 0; i < fileSize; ++i) {
		if(i < fileSize/3) {
			buffer[i] = 'A';
		} else if(i < 2*fileSize/3) {
			buffer[i] = 'A'+rand()%4;
		} else {
			buffer[i] = 'A'+rand()%26;
		}
	}
	fwrite(buffer, 1, fileSize, fi);
	fclose(fi);
	delete [] buffer;
	buffer = NULL;
	std::string fi_name = "test.in";
	std::string fo_name = "test.out";
	std::string fd_name = "test.dec";
	std::string fc_name = "test.cmp";
	long long int compressedSize = _sbc_filer->compressFile(fi_name, fo_name);
	int numberOfBlocks = 1;
	int blockSize = 4096;

	std::vector<int> blockSizeVec;
	long long int cur = 0;
	long long int end = fileSize;
	while(cur < end) {
		int len = end - cur < blockSize ? end - cur : blockSize;
		blockSizeVec.push_back(len);
		cur += len;
	}
	std::vector<int> blockIdxVec;
	blockIdxVec = _sbc_filer->decompressBlock(fo_name, fd_name);
	FILE* fd = fopen("test.dec", "rb");
	FILE* fc = fopen("test.cmp", "wb");
	fi = fopen("test.in", "rb");
	buffer = new char[blockSize];
	for(int i = 0; i < blockIdxVec.size(); ++i) {
		int offset = blockIdxVec[i] * blockSize;
		fseek(fi, offset, SEEK_SET);
		int sz = blockSizeVec[blockIdxVec[i]];
		fread(buffer, 1, sz, fi);
		fwrite(buffer, 1, sz, fc);
	}
	delete [] buffer;
	buffer = NULL;
	fclose(fi);
	fclose(fd);
	fclose(fc);

	fd = fopen("test.dec", "rb");
	fseek(fd, 0, SEEK_END);
	long long int fdSize = ftell(fd);
	rewind(fd);
	char* fdBuffer = new char[fdSize];
	fread(fdBuffer, 1, fdSize, fd);
	fc = fopen("test.cmp", "rb");
	fseek(fc, 0, SEEK_END);
	long long int fcSize = ftell(fc);
	rewind(fc);
	char* fcBuffer = new char[fcSize];
	fread(fcBuffer, 1, fcSize, fc);
	EXPECT_EQ(fdSize, fcSize);
	EXPECT_TRUE(0 == std::memcmp( fdBuffer, fcBuffer, fdSize ));

	fclose(fd);
	fclose(fc);
	fd = NULL;
	fc = NULL;
	delete [] fdBuffer;
	delete [] fcBuffer;
	fdBuffer = NULL;
	fcBuffer = NULL;
}


TEST_F(FilerTest, MBCTestRegularSize) {
	FILE* fp = fopen("test.in", "wb");
	long long int fileSize = 1024*1024*5;
	char* buffer = new char[fileSize];
	for(int i = 0; i < fileSize; ++i) {
		if(i < fileSize/3) {
			buffer[i] = 'A';
		} else if(i < 2*fileSize/3) {
			buffer[i] = 'A'+rand()%4;
		} else {
			buffer[i] = 'A'+rand()%26;
		}
	}
	fwrite(buffer, 1, fileSize, fp);
	fclose(fp);
	delete [] buffer;
	buffer = NULL;
	std::string fi_name = "test.in";
	std::string fo_name = "test.out";
	std::string fd_name = "test.dec";
	long long int compressedSize = _mbc_filer->compressFile(fi_name, fo_name);
	long long int decompressedSize = _mbc_filer->decompressFile(fo_name, fd_name);
	EXPECT_EQ(decompressedSize, fileSize);
	long long int srcSize = 0;
	long long int dstSize = 0;
	fp = fopen("test.in", "rb");
	fseek(fp, 0, SEEK_END);
	srcSize = ftell(fp);
	char* srcBuffer = new char[srcSize];
	rewind(fp);
	fread(srcBuffer, 1, srcSize, fp);
	fclose(fp);
	fp = NULL;
	fp = fopen("test.dec", "rb");
	fseek(fp, 0, SEEK_END);
	dstSize = ftell(fp);
	char* dstBuffer = new char[dstSize];
	rewind(fp);
	fread(dstBuffer, 1, dstSize, fp);
	fclose(fp);
	fp = NULL;
	EXPECT_EQ(fileSize, srcSize);
	EXPECT_EQ(fileSize, dstSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, dstBuffer, fileSize ));
	delete [] srcBuffer;
	srcBuffer = NULL;
	delete [] dstBuffer;
	dstBuffer = NULL;
}

TEST_F(FilerTest, MBCTestUnRegularSize) {
	FILE* fp = fopen("test.in", "wb");
	long long int fileSize = 1024*1024*5+100;
	char* buffer = new char[fileSize];
	for(int i = 0; i < fileSize; ++i) {
		if(i < fileSize/3) {
			buffer[i] = 'A';
		} else if(i < 2*fileSize/3) {
			buffer[i] = 'A'+rand()%4;
		} else {
			buffer[i] = 'A'+rand()%26;
		}
	}
	fwrite(buffer, 1, fileSize, fp);
	fclose(fp);
	fp = NULL;
	delete [] buffer;
	buffer = NULL;
	std::string fi_name = "test.in";
	std::string fo_name = "test.out";
	std::string fd_name = "test.dec";
	long long int compressedSize = _mbc_filer->compressFile(fi_name, fo_name);
	long long int decompressedSize = _mbc_filer->decompressFile(fo_name, fd_name);
	EXPECT_EQ(decompressedSize, fileSize);
	long long int srcSize = 0;
	long long int dstSize = 0;
	fp = fopen("test.in", "rb");
	fseek(fp, 0, SEEK_END);
	srcSize = ftell(fp);
	char* srcBuffer = new char[srcSize];
	rewind(fp);
	fread(srcBuffer, 1, srcSize, fp);
	fclose(fp);
	fp = NULL;
	fp = fopen("test.dec", "rb");
	fseek(fp, 0, SEEK_END);
	dstSize = ftell(fp);
	char* dstBuffer = new char[dstSize];
	rewind(fp);
	fread(dstBuffer, 1, dstSize, fp);
	fclose(fp);
	EXPECT_EQ(fileSize, srcSize);
	EXPECT_EQ(fileSize, dstSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, dstBuffer, fileSize ));
	delete [] srcBuffer;
	srcBuffer = NULL;
	delete [] dstBuffer;
	dstBuffer = NULL;
}

TEST_F(FilerTest, MBCTestDecompressBlock) {
	FILE* fi = fopen("test.in", "wb");
	long long int fileSize = 1024*1024*5;
	char* buffer = new char[fileSize];
	for(int i = 0; i < fileSize; ++i) {
		if(i < fileSize/3) {
			buffer[i] = 'A';
		} else if(i < 2*fileSize/3) {
			buffer[i] = 'A'+rand()%4;
		} else {
			buffer[i] = 'A'+rand()%26;
		}
	}
	fwrite(buffer, 1, fileSize, fi);
	fclose(fi);
	delete [] buffer;
	buffer = NULL;
	std::string fi_name = "test.in";
	std::string fo_name = "test.out";
	std::string fd_name = "test.dec";
	std::string fc_name = "test.cmp";
	long long int compressedSize = _mbc_filer->compressFile(fi_name, fo_name);
	int numberOfBlocks = 4;
	int blockSize = 4096;

	std::vector<int> blockSizeVec;
	long long int cur = 0;
	long long int end = fileSize;
	while(cur < end) {
		int len = end - cur < blockSize ? end - cur : blockSize;
		blockSizeVec.push_back(len);
		cur += len;
	}
	std::vector<int> blockIdxVec;
	blockIdxVec = _mbc_filer->decompressBlock(fo_name, fd_name);
	FILE* fd = fopen("test.dec", "rb");
	FILE* fc = fopen("test.cmp", "wb");
	fi = fopen("test.in", "rb");
	buffer = new char[blockSize];
	for(int i = 0; i < blockIdxVec.size(); ++i) {
		int offset = blockIdxVec[i] * blockSize;
		fseek(fi, offset, SEEK_SET);
		int sz = blockSizeVec[blockIdxVec[i]];
		fread(buffer, 1, sz, fi);
		fwrite(buffer, 1, sz, fc);
	}
	delete [] buffer;
	buffer = NULL;
	fclose(fi);
	fclose(fd);
	fclose(fc);

	fd = fopen("test.dec", "rb");
	fseek(fd, 0, SEEK_END);
	long long int fdSize = ftell(fd);
	rewind(fd);
	char* fdBuffer = new char[fdSize];
	fread(fdBuffer, 1, fdSize, fd);
	fc = fopen("test.cmp", "rb");
	fseek(fc, 0, SEEK_END);
	long long int fcSize = ftell(fc);
	rewind(fc);
	char* fcBuffer = new char[fcSize];
	fread(fcBuffer, 1, fcSize, fc);
	EXPECT_EQ(fdSize, fcSize);
	EXPECT_TRUE(0 == std::memcmp( fdBuffer, fcBuffer, fdSize ));

	fclose(fd);
	fclose(fc);
	fd = NULL;
	fc = NULL;
	delete [] fdBuffer;
	delete [] fcBuffer;
	fdBuffer = NULL;
	fcBuffer = NULL;
}



TEST_F(FilerTest, RACTestRegularSize) {
	FILE* fp = fopen("test.in", "wb");
	long long int fileSize = 1024*1024*5;
	char* buffer = new char[fileSize];
	for(int i = 0; i < fileSize; ++i) {
		if(i < fileSize/3) {
			buffer[i] = 'A';
		} else if(i < 2*fileSize/3) {
			buffer[i] = 'A'+rand()%4;
		} else {
			buffer[i] = 'A'+rand()%26;
		}
	}
	fwrite(buffer, 1, fileSize, fp);
	fclose(fp);
	delete [] buffer;
	buffer = NULL;
	std::string fi_name = "test.in";
	std::string fo_name = "test.out";
	std::string fd_name = "test.dec";
	long long int compressedSize = _rac_filer->compressFile(fi_name, fo_name);
	long long int decompressedSize = _rac_filer->decompressFile(fo_name, fd_name);
	EXPECT_EQ(decompressedSize, fileSize);
	long long int srcSize = 0;
	long long int dstSize = 0;
	fp = fopen("test.in", "rb");
	fseek(fp, 0, SEEK_END);
	srcSize = ftell(fp);
	char* srcBuffer = new char[srcSize];
	rewind(fp);
	fread(srcBuffer, 1, srcSize, fp);
	fclose(fp);
	fp = NULL;
	fp = fopen("test.dec", "rb");
	fseek(fp, 0, SEEK_END);
	dstSize = ftell(fp);
	char* dstBuffer = new char[dstSize];
	rewind(fp);
	fread(dstBuffer, 1, dstSize, fp);
	fclose(fp);
	fp = NULL;
	EXPECT_EQ(fileSize, srcSize);
	EXPECT_EQ(fileSize, dstSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, dstBuffer, fileSize ));
	delete [] srcBuffer;
	srcBuffer = NULL;
	delete [] dstBuffer;
	dstBuffer = NULL;
}

TEST_F(FilerTest, RACTestUnregularSize) {
	FILE* fp = fopen("test.in", "wb");
	long long int fileSize = 1024*1024*5+100;
	char* buffer = new char[fileSize];
	for(int i = 0; i < fileSize; ++i) {
		if(i < fileSize/3) {
			buffer[i] = 'A';
		} else if(i < 2*fileSize/3) {
			buffer[i] = 'A'+rand()%4;
		} else {
			buffer[i] = 'A'+rand()%26;
		}
	}
	fwrite(buffer, 1, fileSize, fp);
	fclose(fp);
	delete [] buffer;
	buffer = NULL;
	std::string fi_name = "test.in";
	std::string fo_name = "test.out";
	std::string fd_name = "test.dec";
	long long int compressedSize = _rac_filer->compressFile(fi_name, fo_name);
	long long int decompressedSize = _rac_filer->decompressFile(fo_name, fd_name);
	EXPECT_EQ(decompressedSize, fileSize);
	long long int srcSize = 0;
	long long int dstSize = 0;
	fp = fopen("test.in", "rb");
	fseek(fp, 0, SEEK_END);
	srcSize = ftell(fp);
	char* srcBuffer = new char[srcSize];
	rewind(fp);
	fread(srcBuffer, 1, srcSize, fp);
	fclose(fp);
	fp = NULL;
	fp = fopen("test.dec", "rb");
	fseek(fp, 0, SEEK_END);
	dstSize = ftell(fp);
	char* dstBuffer = new char[dstSize];
	rewind(fp);
	fread(dstBuffer, 1, dstSize, fp);
	fclose(fp);
	fp = NULL;
	EXPECT_EQ(fileSize, srcSize);
	EXPECT_EQ(fileSize, dstSize);
	EXPECT_TRUE(0 == std::memcmp( srcBuffer, dstBuffer, fileSize ));
	delete [] srcBuffer;
	srcBuffer = NULL;
	delete [] dstBuffer;
	dstBuffer = NULL;
}

TEST_F(FilerTest, RACTestDecompressBlock) {
	FILE* fi = fopen("test.in", "wb");
	long long int fileSize = 1024*1024*5;
	char* buffer = new char[fileSize];
	for(int i = 0; i < fileSize; ++i) {
		if(i < fileSize/3) {
			buffer[i] = 'A';
		} else if(i < 2*fileSize/3) {
			buffer[i] = 'A'+rand()%4;
		} else {
			buffer[i] = 'A'+rand()%26;
		}
	}
	fwrite(buffer, 1, fileSize, fi);
	fclose(fi);
	delete [] buffer;
	buffer = NULL;
	std::string fi_name = "test.in";
	std::string fo_name = "test.out";
	std::string fd_name = "test.dec";
	std::string fc_name = "test.cmp";
	long long int compressedSize = _rac_filer->compressFile(fi_name, fo_name);
	int numberOfBlocks = 256;
	int blockSize = 4096;

	std::vector<int> blockSizeVec;
	long long int cur = 0;
	long long int end = fileSize;
	while(cur < end) {
		int len = end - cur < blockSize ? end - cur : blockSize;
		blockSizeVec.push_back(len);
		cur += len;
	}
	std::vector<int> blockIdxVec;
	blockIdxVec = _rac_filer->decompressBlock(fo_name, fd_name);
	FILE* fd = fopen("test.dec", "rb");
	FILE* fc = fopen("test.cmp", "wb");
	fi = fopen("test.in", "rb");
	buffer = new char[blockSize];
	for(int i = 0; i < blockIdxVec.size(); ++i) {
		int offset = blockIdxVec[i] * blockSize;
		fseek(fi, offset, SEEK_SET);
		int sz = blockSizeVec[blockIdxVec[i]];
		fread(buffer, 1, sz, fi);
		fwrite(buffer, 1, sz, fc);
	}
	delete [] buffer;
	buffer = NULL;
	fclose(fi);
	fclose(fd);
	fclose(fc);

	fd = fopen("test.dec", "rb");
	fseek(fd, 0, SEEK_END);
	long long int fdSize = ftell(fd);
	rewind(fd);
	char* fdBuffer = new char[fdSize];
	fread(fdBuffer, 1, fdSize, fd);
	fc = fopen("test.cmp", "rb");
	fseek(fc, 0, SEEK_END);
	long long int fcSize = ftell(fc);
	rewind(fc);
	char* fcBuffer = new char[fcSize];
	fread(fcBuffer, 1, fcSize, fc);
	EXPECT_EQ(fdSize, fcSize);
	EXPECT_TRUE(0 == std::memcmp( fdBuffer, fcBuffer, fdSize ));

	fclose(fd);
	fclose(fc);
	fd = NULL;
	fc = NULL;
	delete [] fdBuffer;
	delete [] fcBuffer;
	fdBuffer = NULL;
	fcBuffer = NULL;
}

TEST_F(FilerTest, RACTestDecompressBlockSilesia) {
	FILE* fr = fopen("./silesia/ooffice", "rb");
	fseek(fr, 0, SEEK_END);
	long long int frSize = ftell(fr);
	rewind(fr);
	char* buffer = new char[frSize];
	fread(buffer, 1, frSize, fr);
	FILE* fi = fopen("./silesia/ooffice.in", "wb");
	fwrite(buffer, 1, frSize, fi);
	fclose(fr);
	fclose(fi);
	fr = NULL;
	fi = NULL;
	delete [] buffer;
	buffer = NULL;

	std::string fi_name = "./silesia/ooffice.in";
	std::string fo_name = "./silesia/ooffice.out";
	std::string fd_name = "./silesia/ooffice.dec";
	std::string fc_name = "./silesia/ooffice.cmp";
	long long int compressedSize = _rac_filer->compressFile(fi_name, fo_name);
	int numberOfBlocks = 256;
	int blockSize = 4096;

	std::vector<int> blockSizeVec;
	long long int cur = 0;
	long long int end = frSize;
	while(cur < end) {
		int len = end - cur < blockSize ? end - cur : blockSize;
		blockSizeVec.push_back(len);
		cur += len;
	}
	std::vector<int> blockIdxVec;
	blockIdxVec = _rac_filer->decompressBlock(fo_name, fd_name);
	FILE* fd = fopen("./silesia/ooffice.dec", "rb");
	FILE* fc = fopen("./silesia/ooffice.cmp", "wb");
	fi = fopen("./silesia/ooffice.in", "rb");
	buffer = new char[blockSize];
	for(int i = 0; i < blockIdxVec.size(); ++i) {
		int offset = blockIdxVec[i] * blockSize;
		fseek(fi, offset, SEEK_SET);
		int sz = blockSizeVec[blockIdxVec[i]];
		fread(buffer, 1, sz, fi);
		fwrite(buffer, 1, sz, fc);
	}
	delete [] buffer;
	buffer = NULL;
	fclose(fi);
	fclose(fd);
	fclose(fc);

	fd = fopen("./silesia/ooffice.dec", "rb");
	fseek(fd, 0, SEEK_END);
	long long int fdSize = ftell(fd);
	rewind(fd);
	char* fdBuffer = new char[fdSize];
	fread(fdBuffer, 1, fdSize, fd);
	fc = fopen("./silesia/ooffice.cmp", "rb");
	fseek(fc, 0, SEEK_END);
	long long int fcSize = ftell(fc);
	rewind(fc);
	char* fcBuffer = new char[fcSize];
	fread(fcBuffer, 1, fcSize, fc);
	EXPECT_EQ(fdSize, fcSize);
	EXPECT_TRUE(0 == std::memcmp( fdBuffer, fcBuffer, fdSize ));

	fclose(fd);
	fclose(fc);
	fd = NULL;
	fc = NULL;
	delete [] fdBuffer;
	delete [] fcBuffer;
	fdBuffer = NULL;
	fcBuffer = NULL;
}

int main(int argc, char* argv[]) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
