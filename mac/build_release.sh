#!/bin/bash

set -euxo pipefail

xcodebuild -target amsynth_au -configuration Release install DSTROOT=$PWD/dstroot -quiet
xcodebuild -target amsynth_vst -configuration Release install DSTROOT=$PWD/dstroot -quiet

codesign --remove-signature dstroot/Library/Audio/Plug-Ins/Components/amsynth.component
codesign --remove-signature dstroot/Library/Audio/Plug-Ins/VST/amsynth.vst

VERSION=$(/usr/libexec/PlistBuddy -c 'Print CFBundleVersion' dstroot/Library/Audio/Plug-Ins/Components/amsynth.component/Contents/Info.plist)

mkdir -p "dstroot/Library/Application Support/amsynth/skins/default"
cp -cR ../data/banks "dstroot/Library/Application Support/amsynth/"
cp -c ../data/skins/default/layout.ini ../data/skins/default/*.png "dstroot/Library/Application Support/amsynth/skins/default/"

mkdir -p build/Packages

pkgbuild build/Packages/au.pkg \
	--component dstroot/Library/Audio/Plug-Ins/Components/amsynth.component \
	--install-location /Library/Audio/Plug-Ins/Components \
	--scripts "pkg/scripts/au"

pkgbuild build/Packages/vst.pkg \
	--component dstroot/Library/Audio/Plug-Ins/VST/amsynth.vst \
	--install-location /Library/Audio/Plug-Ins/VST \
	--scripts "pkg/scripts/vst"

pkgbuild build/Packages/data.pkg \
	--root "dstroot/Library/Application Support/amsynth" \
	--install-location "/Library/Application Support/amsynth" \
	--identifier com.nickdowell.amsynth.data \
	--version "${VERSION}" \

productbuild \
	--distribution pkg/distribution.xml \
	--package-path build/Packages \
	--resources pkg/resources \
	"build/Packages/amsynth-${VERSION}.pkg"

(cd build/Packages && zip "../../amsynth-${VERSION}-macos.zip" "amsynth-${VERSION}.pkg")
(cd pkg && zip "../amsynth-${VERSION}-macos.zip" Readme.rtf)

(cd build/Release && zip -r "../../amsynth-${VERSION}-macos.dSYMs.zip" *.dSYM)
