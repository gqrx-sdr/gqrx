#!/bin/bash
cp /Library/Frameworks/iio.framework/iio /usr/local/lib/libiio.dylib
install_name_tool -id "/usr/local/lib/libiio.dylib" /usr/local/lib/libiio.dylib
cp /Library/Frameworks/ad9361.framework/ad9361 /usr/local/lib/libad9361.dylib
install_name_tool -id "/usr/local/lib/libad9361.dylib" /usr/local/lib/libad9361.dylib
install_name_tool -delete_rpath /Library/Frameworks /usr/local/lib/libad9361.dylib
install_name_tool -change @rpath/iio.framework/Versions/0.21/iio /usr/local/lib/libiio.dylib /usr/local/lib/libad9361.dylib
install_name_tool -change @rpath/iio.framework/Versions/0.21/iio /usr/local/lib/libiio.dylib /usr/local/lib/SoapySDR/modules0.*/libPlutoSDRSupport.so
install_name_tool -change @rpath/ad9361.framework/Versions/0.2/ad9361 /usr/local/lib/libad9361.dylib /usr/local/lib/SoapySDR/modules0.*/libPlutoSDRSupport.so
