#!/bin/bash

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

dylibbundler -s /usr/local/opt/icu4c/lib/ -od -b -x Gqrx.app/Contents/MacOS/gqrx -x Gqrx.app/Contents/soapy-modules/libremoteSupport.so -d Gqrx.app/Contents/Libs/
cp /usr/local/opt/python@3.9/Frameworks/Python.framework/Versions/3.9/Python Gqrx.app/Contents/Libs
install_name_tool -change "/usr/local/opt/python@3.9/Frameworks/Python.framework/Versions/3.9/Python" "@executable_path/../libs/Python" "Gqrx.app/Contents/Libs/libgnuradio-runtime.3.9.0.0.dylib"
/usr/local/opt/qt@5/bin/macdeployqt Gqrx.app -dmg -no-strip -always-overwrite
mv Gqrx.dmg Gqrx-$GQRX_VERSION.dmg
