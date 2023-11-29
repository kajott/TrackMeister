#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
# SPDX-License-Identifier: MIT
import json

if __name__ == "__main__":
    with open("font.json") as f:
        data = json.load(f)
    atlas = data.get('atlas') or data['variants'][0]['metrics']
    w, h = atlas['width'], atlas['height']
    metrics = data.get('metrics') or data['variants'][0]['metrics']
    mscale = 1 / metrics['lineHeight']
    mbase  = metrics['ascender'] * mscale
    if 'variants' in data:
        glyphs = {}
        for var in data['variants']:
            glyphs.update({ g['unicode']: g for g in var['glyphs'] })
    else:
        glyphs = { g['unicode']: g for g in data['glyphs'] }

    with open("font.png", 'rb') as f:
        png = f.read()

    order = sorted(glyphs)
    for cp in (0xFFFD, ord('?')):
        if cp in order:
            fgi = order.index(cp)
            break

    with open("font_data.cpp", 'w') as f:
        f.write("// This file has been generated automatically, DO NOT EDIT!\n\n")
        f.write('#include "font_data.h"\n\n')
        f.write("namespace FontData {\n\n")
        f.write(f"const int TexDataSize        = {len(png):6};\n")
        f.write(f"const int NumGlyphs          = {len(glyphs):6};\n")
        f.write(f"const int FallbackGlyphIndex = {fgi:6};\n")
        f.write(f"const float Baseline         = {mbase:.6f}f;\n\n")

        f.write("const Glyph GlyphData[] = {\n")
        for cp in order:
            g = glyphs[cp]
            adv = g['advance'] * mscale
            p = g.get('planeBounds')
            if p:
                px0 =         p['left']   * mscale
                py0 = mbase - p['top']    * mscale
                px1 =         p['right']  * mscale
                py1 = mbase - p['bottom'] * mscale
            else:
                px0, py0, px1, py1 = 4*[0]
            t = g.get('atlasBounds')
            if t:
                tx0 =     t['left']
                ty0 = h - t['top']
                tx1 =     t['right']
                ty1 = h - t['bottom']
            else:
                tx0, ty0, tx1, ty1 = 4*[0]
            space = "false," if (t and p) else "true, "
            f.write(f"    {{ 0x{cp:08X}, {adv:8.6f}f, {space} {{{px0:9.6f}f,{py0:9.6f}f,{px1:9.6f}f,{py1:9.6f}f }}, {{{tx0:6.1f}f/{w},{ty0:6.1f}f/{h},{tx1:6.1f}f/{w},{ty1:6.1f}f/{h} }} }},\n")
        f.write("};\n\n")

        f.write("const uint8_t TexData[] = {")
        comma = ""
        BPL = (254 - 4) // 5
        for pos in range(0, len(png), BPL):
            f.write(comma + "\n    " + ','.join(f"0x{b:02X}" for b in png[pos : pos + BPL]))
            comma = ","
        f.write("\n};\n\n} // namespace FontData\n")
