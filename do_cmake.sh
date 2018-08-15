#!/bin/sh

git clone git@github.com:google/googletest.git lib/googletest
cd lib/googletest
cd ../..
git clone git@github.com:lz4/lz4.git lib/lz4
cd lib/lz4
git checkout tags/v1.8.2 -b mbc-bench
make -j8
cd ../..
git clone git@github.com:facebook/zstd.git lib/zstd
cd lib/zstd
git checkout tags/v1.3.5 -b mbc-bench
git apply ../../patches/zdict.patch
make -j8
cd ../..
cp lib/lz4/lib/lz4.h include/
cp lib/zstd/zstd.h include/
cp lib/zstd/lib/dictBuilder/zdict.h include/
