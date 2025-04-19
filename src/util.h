// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#ifndef NDEBUG
    #include <cstdio>
    #define Dprintf printf
#else
    #define Dprintf(...) do{}while(0)
#endif

//! create a FourCC from a string
constexpr inline uint32_t makeFourCC(const char* s) {
    return  !s ? 0u : ( uint32_t(s[0])        |
        (!s[1] ? 0u : ((uint32_t(s[1]) <<  8) |
        (!s[2] ? 0u : ((uint32_t(s[2]) << 16) |
                       (uint32_t(s[3]) << 24))))));
}

//! clone of std::isdigit() that's guaranteed to not choke on 8-bit input
constexpr inline bool isDigit(char c)
    { return (c >= '0') && (c <= '9'); }

//! clone of std::isspace() that's guaranteed to not choke on 8-bit input
constexpr inline bool isSpace(char c)
    { return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'); }

//! as isSpace, but doesn't match any newline characters
constexpr inline bool isSpaceNoNewline(char c)
    { return (c == ' ') || (c == '\t'); }

//! clone of std::tolower() that's guaranteed to not choke on 8-bit input
constexpr inline char toLower(char c)
    { return ((c >= 'A') && (c <= 'Z')) ? (c + ('a' - 'A')) : c; }
