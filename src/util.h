// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#ifndef NDEBUG
    #include <cstdio>
    #define Dprintf printf
#else
    #define Dprintf(...) do{}while(0)
#endif

constexpr inline bool isSpace(char c)
    { return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'); }
