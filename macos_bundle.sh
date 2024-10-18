#!/bin/bash -e

GQRX_VERSION=$(<build/version.txt)
IDENTITY=Y3GC27WZ4S

mkdir -p Gqrx.app/Contents/MacOS
mkdir -p Gqrx.app/Contents/Resources
mkdir -p Gqrx.app/Contents/soapy-modules

/bin/cat <<EOM >Gqrx.app/Contents/Info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>NSPrincipalClass</key>
  <string>NSApplication</string>
  <key>CFBundleExecutable</key>
  <string>gqrx</string>
  <key>CFBundleIdentifier</key>
  <string>dk.gqrx.gqrx</string>
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

/bin/cat <<EOM >/tmp/Entitlements.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>com.apple.security.cs.allow-unsigned-executable-memory</key>
  <true/>
</dict>
</plist>
EOM

cp build/src/gqrx Gqrx.app/Contents/MacOS
cp resources/icons/gqrx.icns Gqrx.app/Contents/Resources
# see https://apple.stackexchange.com/questions/437618/why-is-homebrew-installed-in-opt-homebrew-on-apple-silicon-macs
MACDEPLOYQT6=/usr/local/opt/qt@6/bin/macdeployqt
if [ -f /opt/homebrew/opt/qt@6/bin/macdeployqt ]; then
    MACDEPLOYQT6=/opt/homebrew/opt/qt@6/bin/macdeployqt
fi
cp /*/*/lib/SoapySDR/modules*/libPlutoSDRSupport.so Gqrx.app/Contents/soapy-modules
cp /*/*/lib/SoapySDR/modules*/libremoteSupport.so Gqrx.app/Contents/soapy-modules
chmod 644 Gqrx.app/Contents/soapy-modules/*

dylibbundler -s /usr/local/opt/icu4c/lib/ -od -b -x Gqrx.app/Contents/MacOS/gqrx -x Gqrx.app/Contents/soapy-modules/libPlutoSDRSupport.so -x Gqrx.app/Contents/soapy-modules/libremoteSupport.so -d Gqrx.app/Contents/libs/
${MACDEPLOYQT6} Gqrx.app -no-strip -always-overwrite # TODO: Remove macdeployqt workaround
if [ "$1" = "true" ]; then
    ${MACDEPLOYQT6} Gqrx.app -no-strip -always-overwrite -sign-for-notarization=$IDENTITY
else
    ${MACDEPLOYQT6} Gqrx.app -no-strip -always-overwrite
fi

for f in Gqrx.app/Contents/libs/*.dylib Gqrx.app/Contents/soapy-modules/*.so Gqrx.app/Contents/Frameworks/*.framework Gqrx.app/Contents/Frameworks/*.dylib Gqrx.app/Contents/MacOS/gqrx
do
    if [ "$1" = "true" ]; then
        codesign --force --verify --verbose --timestamp --options runtime --entitlements /tmp/Entitlements.plist --sign $IDENTITY $f
    else
        codesign --remove-signature $f
    fi
done
