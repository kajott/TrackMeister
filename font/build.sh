#!/bin/bash
cd "$(dirname "$0")"

ATLAS_GEN_REPO=https://github.com/Chlumsky/msdf-atlas-gen
ATLAS_GEN_BUILDDIR=msdf-atlas-gen/build
ATLAS_GEN_BINARY=$ATLAS_GEN_BUILDDIR/bin/msdf-atlas-gen
ATLAS_GEN_OPTS="-charset charset.txt -type msdf -format png -potr -size 32 -pxrange 8 -outerpxpadding 2"

FONT1_NAME=Inconsolata
FONT1_FILE=${FONT1_NAME}-Regular.ttf
FONT1_URL=https://raw.githubusercontent.com/google/fonts/main/ofl/inconsolata/static/$FONT1_FILE

FONT2_NAME=Iosevka
FONT2_FILE=${FONT2_NAME}Fixed-Regular.ttf
FONT2_ZIP=${FONT2_NAME}-TTF.zip
FONT2_URL=https://github.com/be5invis/Iosevka/releases/download/v29.2.1/PkgTTF-IosevkaFixed-29.2.1.zip

set -ex

if [ ! -x $ATLAS_GEN_BINARY ] ; then
    [ -d msdf-atlas-gen ] || git clone --recursive $ATLAS_GEN_REPO
    cmake -S msdf-atlas-gen -B $ATLAS_GEN_BUILDDIR -G Ninja \
          -DCMAKE_BUILD_TYPE=Release \
          -DMSDFGEN_DISABLE_SVG=1 \
          -DMSDF_ATLAS_BUILD_STANDALONE=1 \
          -DMSDF_ATLAS_NO_ARTERY_FONT=1 \
          -DMSDF_ATLAS_USE_SKIA=0 \
          -DMSDF_ATLAS_USE_VCPKG=0
    cmake --build $ATLAS_GEN_BUILDDIR
fi

[ ! -r $FONT1_FILE ] && wget -O$FONT1_FILE -nv $FONT1_URL
$ATLAS_GEN_BINARY $ATLAS_GEN_OPTS -font $FONT1_FILE -imageout $FONT1_NAME.png -json $FONT1_NAME.json

[ ! -r $FONT2_FILE -a ! -r $FONT2_ZIP ] && wget -O$FONT2_ZIP -nv $FONT2_URL
[ ! -r $FONT2_FILE ] && unzip $FONT2_ZIP $FONT2_FILE
$ATLAS_GEN_BINARY $ATLAS_GEN_OPTS -font $FONT2_FILE -imageout $FONT2_NAME.png -json $FONT2_NAME.json

python3 convert_font.py
