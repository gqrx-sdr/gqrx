#!/bin/bash

if [[ -f "/usr/local/lib/SoapySDR/modules*/libPlutoSDRSupport.so" ]]
then

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

fi

GQRX_VERSION=$(<version.txt)

mkdir -p Gqrx.app/Contents/MacOS
mkdir -p Gqrx.app/Contents/Resources

/bin/cat <<EOM >Gqrx.app/Contents/Info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>NSPrincipalClass</key>
  <string>NSApplication</string>
  <key>CFBundleGetInfoString</key>
  <string>Gqrx</string>
  <key>CFBundleExecutable</key>
  <string>gqrx</string>
  <key>CFBundleIdentifier</key>
  <string>dk.gqrx.www</string>
  <key>CFBundleName</key>
  <string>Gqrx</string>
  <key>CFBundleIconFile</key>
  <string>gqrx.icns</string>
  <key>CFBundleShortVersionString</key>
  <string>$GQRX_VERSION</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>IFMajorVersion</key>
  <integer>1</integer>
  <key>IFMinorVersion</key>
  <integer>0</integer>
</dict>
</plist>
EOM

cp build/src/gqrx Gqrx.app/Contents/MacOS
cp resources/icons/gqrx.icns Gqrx.app/Contents/Resources
cp -r /usr/local/lib/SoapySDR/modules* Gqrx.app/Contents/soapy-modules
chmod 644 Gqrx.app/Contents/soapy-modules/*

dylibbundler -s /usr/local/opt/icu4c/lib/ -od -b -x Gqrx.app/Contents/MacOS/gqrx -x Gqrx.app/Contents/soapy-modules/libPlutoSDRSupport.so -x Gqrx.app/Contents/soapy-modules/libremoteSupport.so -d Gqrx.app/Contents/Libs/
/usr/local/opt/qt/bin/macdeployqt Gqrx.app -dmg -no-strip -always-overwrite
mv Gqrx.dmg Gqrx-$GQRX_VERSION.dmg
