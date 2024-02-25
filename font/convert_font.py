#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2023-2024 Martin J. Fiedler <keyj@emphy.de>
# SPDX-License-Identifier: MIT
import io
import json
from PIL import Image


def load_fonts():
    return MultiFontAtlas(
        MSDFFont("Inconsolata")
    )


class MultiFontAtlas:
    def __init__(self, *fonts):
        self.fonts = sorted(fonts, key=lambda f: f.img.size, reverse=True)
        assert len(fonts) == 1
        self.img = self.fonts[0].img

    def get_png(self):
        png = io.BytesIO()
        self.img.save(png, format='png', optimize=True)
        return png.getvalue()


class Font:
    def __init__(self, name, imgfile):
        self.name = name
        self.img = Image.open(imgfile).convert('RGB')
        self.atlas_pos = (0,0)


class MSDFFont(Font):
    def __init__(self, name, imgfile=None, jsonfile=None):
        Font.__init__(self, name, imgfile or (name + ".png"))
        with open(jsonfile or (name + ".json")) as f:
            data = json.load(f)
        atlas = data.get('atlas') or data['variants'][0]['metrics']
        w, h = atlas['width'], atlas['height']
        assert self.img.size == (w, h)
        metrics = data.get('metrics') or data['variants'][0]['metrics']
        mscale = 1 / metrics['lineHeight']
        mbase  = metrics['ascender'] * mscale
        if 'variants' in data:
            glyphs = {}
            for var in data['variants']:
                glyphs.update({ g['unicode']: g for g in var['glyphs'] })
        else:
            glyphs = { g['unicode']: g for g in data['glyphs'] }

        self.bitmap_height = 0
        self.baseline = mbase

        order = sorted(glyphs)
        for cp in (0xFFFD, ord('?')):
            if cp in order:
                self.fallback_index = order.index(cp)
                break

        self.glyphs = []
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
            self.glyphs.append((cp, adv, not(t and p), (px0,py0,px1,py1), (tx0,ty0,tx1,ty1)))


if __name__ == "__main__":
    atlas = load_fonts()
    w, h = atlas.img.size
    png = atlas.get_png()

    with open("font_data.cpp", 'w') as f:
        f.write('// This file has been generated automatically, DO NOT EDIT!\n\n')
        f.write('#include "font_data.h"\n\n')
        f.write('namespace FontData {\n\n')

        for font in atlas.fonts:
            f.write(f'extern const Glyph GlyphData_{font.name}[];\n')
        f.write('\n')

        f.write('const Font Fonts[] = {\n')
        for font in atlas.fonts:
            nstr = '"' + font.name + '",'
            f.write(f'    {{ {nstr:<14} {font.bitmap_height:2d}, {font.baseline:.6f}f, GlyphData_{font.name+",":<12} {len(font.glyphs):3d}, {font.fallback_index:3d} }},\n')
        f.write(f'    {{ {"nullptr,":<14}  0, 0.000000f, {"nullptr,":<22}   0,   0 }}\n')
        f.write('};\n\n')

        for font in atlas.fonts:
            ox, oy = font.atlas_pos
            f.write(f'const Glyph GlyphData_{font.name}[] = {{\n')
            for cp, adv, space, (px0,py0,px1,py1), (tx0,ty0,tx1,ty1) in font.glyphs:
                space = "true, " if space else "false,"
                f.write(f"    {{ 0x{cp:08X}, {adv:8.6f}f, {space} {{{px0:9.6f}f,{py0:9.6f}f,{px1:9.6f}f,{py1:9.6f}f }}, {{{tx0+ox:6.1f}f/{w},{ty0+oy:6.1f}f/{h},{tx1+ox:6.1f}f/{w},{ty1+oy:6.1f}f/{h} }} }},\n")
            f.write('};\n\n')

        f.write(f'const int TexDataSize = {len(png):6};\n')
        f.write('const uint8_t TexData[] = {')
        comma = ""
        BPL = (254 - 4) // 5
        for pos in range(0, len(png), BPL):
            f.write(comma + '\n    ' + ','.join(f"0x{b:02X}" for b in png[pos : pos + BPL]))
            comma = ","
        f.write('\n};\n\n} // namespace FontData\n')
