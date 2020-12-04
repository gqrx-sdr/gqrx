#!/bin/bash

GQRX_VERSION=$(<version.txt)

mkdir -p Gqrx.app/Contents/MacOS
mkdir -p Gqrx.app/Contents/Resources

/bin/cat <<EOM >Gqrx.app/Contents/Info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleGetInfoString</key>
  <string>Gqrx</string>
  <key>LSEnvironment</key>
  <dict>
    <key>SOAPY_SDR_ROOT</key>
    <string>/usr/local</string>
  </dict>
  <key>CFBundleExecutable</key>
  <string>gqrx</string>
  <key>CFBundleIdentifier</key>
  <string>dk.gqrx.www</string>
  <key>CFBundleName</key>
  <string>gqrx</string>
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

dylibbundler -s /usr/local/opt/icu4c/lib/ -od -b -x Gqrx.app/Contents/MacOS/gqrx -d Gqrx.app/Contents/Libs/
/usr/local/opt/qt/bin/macdeployqt Gqrx.app -dmg -no-strip -always-overwrite
