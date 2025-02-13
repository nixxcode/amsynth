#!/bin/bash

set -euxo pipefail

mkdir -p "/Library/Application Support/amsynth/banks"
cp -c ../data/banks/*.bank "/Library/Application Support/amsynth/banks/"

mkdir -p "/Library/Application Support/amsynth/skins/default"
cp -c ../data/skins/default/layout.ini ../data/skins/default/*.png "/Library/Application Support/amsynth/skins/default/"
