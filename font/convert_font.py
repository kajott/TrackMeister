#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2023-2024 Martin J. Fiedler <keyj@emphy.de>
# SPDX-License-Identifier: MIT
import json
import os
import subprocess
import tempfile
from PIL import Image, ImageOps


def load_fonts():
    return MultiFontAtlas(
        MSDFFont("Inconsolata", number_height=0.595),
        MSDFFont("Iosevka",     number_height=0.588),
        BitmapFont("topaz1200", cell_size=(8,8),  display_size=(8,16), baseline=7,  number_height= 7, mapping='amiga'),
        BitmapFont("topaz500",  cell_size=(8,8),  display_size=(8,16), baseline=7,  number_height= 7, mapping='amiga'),
        BitmapFont("pc16",      cell_size=(8,16), display_size=(9,16), baseline=12, number_height=10, mapping='cp437'),
        BitmapFont("pc8",       cell_size=(8,8),  display_size=(8,8),  baseline=7,  number_height= 7, mapping='cp437'),
    )

ShowAtlas = 1  # debug option: show atlas texture at end

Mappings = {
    'ascii': list(range(32,127)),
    'amiga': list(range(32,127)) + [0x2592] + 32 * [0x25FB] + list(range(160, 256)),  # Latin1 with minor changes
    'cp437': # based on https://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/PC/CP437.TXT
             # and      https://www.unicode.org/Public/MAPPINGS/VENDORS/MISC/IBMGRAPH.TXT
       [0x0020, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022, 0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,  #   0.. 16
        0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8, 0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC   #  16.. 31
       ] + list(range(32, 127)) + [0x2302,                                                                                               #  32..127
        0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,  # 128..143
        0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,  # 144..159
        0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, 0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,  # 160..175
        0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,  # 176..191
        0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,  # 192..207
        0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,  # 208..223
        0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,  # 224..239
        0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25FC, 0x00A0], # 240..255
}


def npot(x):
    shift = 1
    while x & (x - 1):
        x = ((x - 1) | ((x - 1) >> shift)) + 1
        shift <<= 1
    return x

def isspace(cp):
    return cp in (0x20, 0xA0)


class MultiFontAtlas:
    def __init__(self, *fonts):
        self.fonts = list(fonts)
        assert len(fonts) >= 1

        # compute packed atlas geometry
        atlas_order = sorted(self.fonts, key=lambda font: font.img.size, reverse=True)
        self.width = npot(atlas_order[0].img.size[0])
        heights = {0:0}
        for font in atlas_order:
            w, h = font.img.size
            newh, x,y = min((max(cy0 for cx0,cy0 in heights.items() if (x0 <= cx0 < (x0 + w))) + h, x0,y0) for x0,y0 in heights.items() if (x0 <= (self.width - w)))
            font.atlas_pos = (x,y)
            heights[x + w] = max((x0,y0) for x0,y0 in heights.items() if (x0 <= (x + w)))[1]
            for x0 in heights:
                if x0 <= x < (x0 + w):
                    heights[x0] = newh
        self.height = npot(max(heights.values()))

    def get_png(self):
        img = Image.new('RGB', (self.width, self.height))
        for font in self.fonts:
            img.paste(font.img, font.atlas_pos)
        if ShowAtlas: img.show()
        pngfile = tempfile.mktemp(".png", "convert_font-")
        img.save(pngfile, format='png', optimize=True)
        return pngfile


class Font:
    invert = False
    def __init__(self, name, imgfile):
        self.name = name
        self.img = Image.open(imgfile).convert('RGB')
        if self.invert: self.img = ImageOps.invert(self.img)
        self.atlas_pos = (0,0)

    def sort_glyphs(self):
        "sort glyphs and remove duplicates"
        def _uniq(glyphs):
            prev = 0
            for g in sorted(glyphs):
                if g[0] != prev: yield g
                prev = g[0]
        self.glyphs = list(_uniq(sorted(self.glyphs)))


class MSDFFont(Font):
    def __init__(self, name, imgfile=None, jsonfile=None, number_height=None):
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
        self.number_height = number_height or (mscale * abs(glyphs[0x34]['planeBounds']['bottom'] - glyphs[0x34]['planeBounds']['top']))
        self.baseline = mbase

        cps = list(glyphs)
        self.glyphs = []
        for cp in cps:
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
        self.sort_glyphs()

        # crop ununsed black lines off the bottom
        h = ((len(self.img.tobytes().rstrip(b'\0')) + 3 * w - 1) // (3 * w) + 3) & (~3)
        self.img = self.img.crop((0,0,w,h))


class BitmapFont(Font):
    invert = True
    def __init__(self, name, imgfile=None, cell_size=(8,8), display_size=None, baseline=0.75, number_height=0.75, mapping='ascii'):
        Font.__init__(self, name, imgfile or (name + ".pbm"))
        if not display_size: display_size = cell_size
        assert not(display_size[1] % cell_size[1])
        self.bitmap_height = display_size[1]
        self.baseline      = baseline      if isinstance(baseline,      float) else (baseline      / cell_size[1])
        self.number_height = number_height if isinstance(number_height, float) else (number_height / cell_size[1])
        cw = cell_size[0] / display_size[1]
        adv = display_size[0] / display_size[1]
        cols, rows = (sz//cs for sz,cs in zip(self.img.size, cell_size))
        coords = [(x * cell_size[0], y * cell_size[1]) for y in range(rows) for x in range(cols)]
        self.glyphs = [(cp, adv, isspace(cp), (0.0, 0.0, cw, 1.0), (x, y, x + cell_size[0], y + cell_size[1]))
            for cp, (x,y) in zip(Mappings[mapping], coords)]
        self.sort_glyphs()


if __name__ == "__main__":
    atlas = load_fonts()
    pngfile = atlas.get_png()

    try:
        subprocess.run(["optipng", "-nx", "-o7", "-strip", "all", pngfile], check=True)
    except (EnvironmentError, subprocess.CalledProcessError) as e:
        print("WARNING: failed to run optipng:", e)
    try:
        subprocess.run(["advpng", "-z4", pngfile], check=True)
    except (EnvironmentError, subprocess.CalledProcessError) as e:
        print("WARNING: failed to run advpng:", e)

    with open(pngfile, 'rb') as f:
        png = f.read()
    try: os.unlink(pngfile)
    except EnvironmentError: pass

    print("generating font_data.cpp ...")
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
            cp2idx = { g[0]:i for i,g in enumerate(font.glyphs) }
            for cp in (0xFFFD, 0x25FB, 0x25FC, 0x25FD, 0x25FE, 0x25A1, 0x25A0, ord('?'), 32):
                try:
                    fallback_index = cp2idx[cp]
                    break
                except KeyError:
                    pass
            f.write(f'    {{ {nstr:<14} {font.bitmap_height:2d}, {font.baseline:.6f}f, {font.number_height:.6f}f, GlyphData_{font.name+",":<12} {len(font.glyphs):3d}, {fallback_index:3d} }},\n')
        f.write(f'    {{ {"nullptr,":<14}  0, 0.000000f, 0.000000f, {"nullptr,":<22}   0,   0 }}\n')
        f.write('};\n\n')

        for font in atlas.fonts:
            ox, oy = font.atlas_pos
            f.write(f'const Glyph GlyphData_{font.name}[] = {{\n')
            for cp, adv, space, (px0,py0,px1,py1), (tx0,ty0,tx1,ty1) in font.glyphs:
                space = "true, " if space else "false,"
                f.write(f"    {{ 0x{cp:08X}, {adv:8.6f}f, {space} {{{px0:9.6f}f,{py0:9.6f}f,{px1:9.6f}f,{py1:9.6f}f }}, {{{tx0+ox:6.1f}f/{atlas.width},{ty0+oy:6.1f}f/{atlas.height},{tx1+ox:6.1f}f/{atlas.width},{ty1+oy:6.1f}f/{atlas.height} }} }},\n")
            f.write('};\n\n')

        f.write(f'const int TexDataSize = {len(png):6};\n')
        f.write('const uint8_t TexData[] = {')
        comma = ""
        BPL = (254 - 4) // 5
        for pos in range(0, len(png), BPL):
            f.write(comma + '\n    ' + ','.join(f"0x{b:02X}" for b in png[pos : pos + BPL]))
            comma = ","
        f.write('\n};\n\n} // namespace FontData\n')
    print("done.")
