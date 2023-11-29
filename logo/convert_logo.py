#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
# SPDX-License-Identifier: MIT

if __name__ == "__main__":
    with open("logo.png", 'rb') as f:
        png = f.read()

    with open("logo_data.cpp", 'w') as f:
        f.write("// This file has been generated automatically, DO NOT EDIT!\n\n")
        f.write(f'extern "C" const int LogoDataSize = {len(png)};\n')
        f.write('extern "C" const unsigned char LogoData[] = {')
        comma = ""
        BPL = (254 - 4) // 5
        for pos in range(0, len(png), BPL):
            f.write(comma + "\n    " + ','.join(f"0x{b:02X}" for b in png[pos : pos + BPL]))
            comma = ","
        f.write('\n};\n')
