#!/bin/bash
cd "$(dirname "$0")"

ATLAS_GEN_REPO=https://github.com/Chlumsky/msdf-atlas-gen
ATLAS_GEN_BUILDDIR=msdf-atlas-gen/build
ATLAS_GEN_BINARY=$ATLAS_GEN_BUILDDIR/bin/msdf-atlas-gen

FONT_FILE=Inconsolata-Regular.ttf
FONT_URL=https://raw.githubusercontent.com/google/fonts/main/ofl/inconsolata/static/$FONT_FILE

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

[ ! -r $FONT_FILE ] && wget -O$FONT_FILE -nv $FONT_URL

$ATLAS_GEN_BINARY \
    -font $FONT_FILE -charset charset.txt \
    -type msdf \
    -format png -imageout font.png \
    -json font.json \
    -size 32 -pxrange 2 \
    -potr

python3 convert_font.py
