#!/bin/bash
cd "$(dirname "$0")"

INPUT_SVG_FILE=tm_logo.svg

set -ex

inkscape $INPUT_SVG_FILE -o logo.png \
    --export-png-color-mode=Gray_8 \
    --export-background=white \
    --export-background-opacity=1.0

if which optipng &>/dev/null ; then
    optipng -nx -o7 -strip all logo.png
elif which pngcrush &>/dev/null ; then
    pngcrush -ow -brute -noreduce logo.png
fi

python3 convert_logo.py
