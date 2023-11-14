#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
# SPDX-License-Identifier: MIT
from PIL import Image
import json
import math

IMAGE_PREDICTOR = "bloom"
IMAGE_ENCODER = "zero-run"

###############################################################################

def predict_image(data, width):
    if IMAGE_PREDICTOR == "delta":  # simple delta predictor ##################
        prev = 0
        data = bytearray(data)
        for i in range(len(data)):
            next = data[i]
            data[i] = (next - prev) & 0xFF
            prev = next
        return bytes(data)

    elif IMAGE_PREDICTOR == "bloom":  # Charles Bloom / Fabian Giesen #########
        new = bytearray(data)
        data += (width + 4) * b'\x00'
        for i in range(len(new)):
            w = data[i-1]
            n = data[i-width]
            nw = data[i-width-1]
            p = n + w - nw
            p = max(p, min((w, n, nw)))
            p = min(p, max((w, n, nw)))
            new[i] = (data[i] - p) & 0xFF
        return new

    else:  # null predictor ###################################################
        return data

def unpredict_image(data, width):
    if IMAGE_PREDICTOR == "delta":  # simple delta predictor ##################
        prev = 0
        data = bytearray(data)
        for i in range(len(data)):
            prev = (prev + data[i]) & 0xFF
            data[i] = prev
        return bytes(data)

    elif IMAGE_PREDICTOR == "bloom":  # Charles Bloom / Fabian Giesen #########
        length = len(data)
        data = bytearray(data + b'\x00' * (width + 4))
        for i in range(len(data)):
            w = data[i-1]
            n = data[i-width]
            nw = data[i-width-1]
            p = n + w - nw
            p = max(p, min((w, n, nw)))
            p = min(p, max((w, n, nw)))
            data[i] = (data[i] + p) & 0xFF
        return bytes(data[:length])

    else:  # null predictor ###################################################
        return data

###############################################################################

def encode_data(data):
    if IMAGE_ENCODER == "zero-run":  # zero-run encoder #######################
        end = len(data)
        data += bytes(2 * [int(data.endswith(b'\x00'))])  # add a terminator
        pos = 0
        while pos < end:
            # detect and encode zero run
            length = 0
            while not(data[pos + length]) and (length < 255): length += 1
            yield length
            pos += length
            if pos >= end: break

            # detect and encode literal run (but accept single zero bytes in it)
            length = 0
            while (data[pos + length]     and (length < 255)) \
               or (data[pos + length + 1] and (length < 254)): length += 1
            yield length
            yield from data[pos : pos+length]
            pos += length

    else:  # null encoder #####################################################
        yield from data

def decode_data(data):
    if IMAGE_ENCODER == "zero-run":  # zero-run decoder #######################
        pos = 0
        while pos < len(data):
            # decode zero run
            length = data[pos]; pos += 1
            yield from ([0] * length)
            if pos >= len(data): break

            # decode literal run
            length = data[pos]; pos += 1
            yield from data[pos : pos+length]
            pos += length

    else:  # null decoder #####################################################
        yield from data

###############################################################################

if __name__ == "__main__":
    with open("font.json") as f:
        data = json.load(f)
    metrics = data.get('metrics') or data['variants'][0]['metrics']
    mscale = 1 / metrics['lineHeight']
    mbase  = metrics['ascender'] * mscale
    if 'variants' in data:
        glyphs = {}
        for var in data['variants']:
            glyphs.update({ g['unicode']: g for g in var['glyphs'] })
    else:
        glyphs = { g['unicode']: g for g in data['glyphs'] }

    img = Image.open("font.png")
    raw = b''
    enc = b''
    dec = b''
    for band in img.split():
        band = band.tobytes()
        benc = bytes(encode_data(predict_image(band, img.size[0])))
        bdec = unpredict_image(bytes(decode_data(benc)), img.size[0])
        raw += band
        enc += benc
        dec += bdec
    with open("font_raw.bin", 'wb') as f: f.write(raw)
    with open("font_enc.bin", 'wb') as f: f.write(enc)
    with open("font_dec.bin", 'wb') as f: f.write(dec)
    assert len(raw) == len(dec), f"{len(raw)=} != {len(dec)=}"
    assert raw == dec

    print("encoded image size:", len(enc), "bytes")
    hist = {x:enc.count(x) for x in set(enc)}
    print(len(hist), "out of 256 values used")
    entropy = -sum(f * math.log2(f / len(enc)) for f in hist.values()) / len(enc)
    print(f"Shannon entropy: {entropy:.2f} bits -> ideal size = {math.ceil(len(enc) / 8 * entropy):.0f} bytes")

    maxlinhist = max(hist.values())
    maxloghist = max(math.log(n) for n in hist.values())
    with open("font_hist.txt", 'w') as f:
        f.write(f"{IMAGE_PREDICTOR=}\n{IMAGE_ENCODER=}\n\n")
        for x in range(256):
            nlin = hist.get(x, 0)
            linbar = "#" * int(nlin / maxlinhist * 32 + 0.9)
            logbar = "#" * int(math.log(nlin) / maxloghist * 31 + 1.0) if nlin else ""
            f.write(f"${x:02X} | {nlin:6d}x |{linbar:<32} |{logbar}\n")

    w, h = img.size
    order = sorted(glyphs)
    for cp in (0xFFFD, ord('?')):
        if cp in order:
            fgi = order.index(cp)
            break

    with open("font_data.cpp", 'w') as f:
        f.write("// This file has been generated automatically, DO NOT EDIT!\n\n")
        f.write('#include "font_data.h"\n\n')
        f.write("namespace FontData {\n\n")
        f.write(f"const int TexWidth           = {w:6};\n")
        f.write(f"const int TexHeight          = {h:6};\n")
        f.write(f"const int TexDataSize        = {len(enc):6};\n\n")
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
        for pos in range(0, len(enc), BPL):
            f.write(comma + "\n    " + ','.join(f"0x{b:02X}" for b in enc[pos : pos + BPL]))
            comma = ","
        f.write("\n};\n\n} // namespace FontData\n")
