// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#ifndef NDEBUG
    #include <cstdio>
    #define Dprintf printf
#else
    #define Dprintf(...) do{}while(0)
#endif

//! clone of std::isspace() that's guaranteed to not choke on 8-bit input
constexpr inline bool isSpace(char c)
    { return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'); }

//! clone of std::tolower() that's guaranteed to not choke on 8-bit input
constexpr inline char toLower(char c)
    { return ((c >= 'A') && (c <= 'Z')) ? (c + ('a' - 'A')) : c; }
