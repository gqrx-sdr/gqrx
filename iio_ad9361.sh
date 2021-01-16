#!/bin/bash

cp ./resources/ad9361.h /usr/local/include
cp ./resources/iio.h /usr/local/include
cp ./resources/libiio.pc  /usr/local/lib/pkgconfig
cp ./resources/libad9361.pc  /usr/local/lib/pkgconfig
cp ./resources/libad9361.dylib /usr/local/lib
cp ./resources/libiio.dylib /usr/local/lib

git clone https://github.com/pothosware/SoapyPlutoSDR.git
cd SoapyPlutoSDR
mkdir build
cd build
cmake ..
make install

cd ../..

rm -rf ./SoapyPlutoSDR
