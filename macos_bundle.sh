#!/bin/bash -e

GQRX_VERSION="$(<build/version.txt)"
IDENTITY=Y3GC27WZ4S

echo "CONDA_PREFIX: " $CONDA_PREFIX

MACDEPLOYQT6=${CONDA_PREFIX}/bin/macdeployqt6
echo "macdeployqt6: " ${MACDEPLOYQT6}

# cleanup and setup
if [ -e Gqrx.app ] ;
then rm -r Gqrx.app
fi

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
cp "$CONDA_PREFIX"/lib/SoapySDR/modules*/* Gqrx.app/Contents/soapy-modules

if [ "$1" = "true" ]; then
    "${MACDEPLOYQT6}" Gqrx.app -verbose=1 -no-strip -always-overwrite -sign-for-notarization="${IDENTITY}" -libpath=Gqrx.app/Contents/Frameworks
else
    "${MACDEPLOYQT6}" Gqrx.app -verbose=1 -no-strip -always-overwrite -libpath=Gqrx.app/Contents/Frameworks
fi

for f in Gqrx.app/Contents/Frameworks/*.dylib Gqrx.app/Contents/soapy-modules/* Gqrx.app/Contents/MacOS/gqrx
do
    if [ "$1" = "true" ]; then
        codesign --force --verify --verbose --timestamp --options runtime --entitlements /tmp/Entitlements.plist --sign "${IDENTITY}" "$f"
    else
        codesign --remove-signature "$f"
    fi
done
