// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <cstdlib>
#include <cstdio>

#include <string>
#include <functional>

#include "config.h"
#include "config_item.h"

////////////////////////////////////////////////////////////////////////////////

//! compare strings for equality, ignoring case and whitespace
bool stringEqualEx(const char* sA, const char* sB) {
    if (!sA || !sB) { return false; }
    while (*sA && *sB) {
        while (*sA && ((*sA == ' ') || (*sA == '_'))) { ++sA; }
        while (*sB && ((*sB == ' ') || (*sB == '_'))) { ++sB; }
        char cA = *sA++;
        char cB = *sB++;
        if ((cA >= 'a') && (cA <= 'z')) { cA -= 32; }
        if ((cB >= 'a') && (cB <= 'z')) { cB -= 32; }
        if (cA != cB) { return false; }
    }
    while (*sA && ((*sA == ' ') || (*sA == '_'))) { ++sA; }
    while (*sB && ((*sB == ' ') || (*sB == '_'))) { ++sB; }
    return !*sA && !*sB;
}

//! pairwise swap of nibbles in a 32-bit value
inline uint32_t nibbleSwap(uint32_t x) {
    return ((x & 0xF0F0F0F0u) >> 4) | ((x & 0x0F0F0F0Fu) << 4);
}

////////////////////////////////////////////////////////////////////////////////

void ConfigParserContext::invalid(const char* what, const char* s) {
    fprintf(stderr, "%s:%d: invalid %s '%s'\n", filename.c_str(), lineno, what, s);
}

bool ConfigItem::parseBool(bool &value, const char* s) {
    const EnumItem e_Bool[] = {
        { "0",        0 }, { "1",       1 },
        { "false",    0 }, { "true",    1 },
        { "off",      0 }, { "on",      1 },
        { "no",       0 }, { "yes",     1 },
        { "disabled", 0 }, { "enabled", 1 },
        { "disable",  0 }, { "enable",  1 },
        { nullptr, 0 }
    };
    int intVal = 0;
    bool result = parseEnum(intVal, s, e_Bool);
    if (result) { value = (intVal != 0); }
    return result;
}

std::string ConfigItem::formatBool(bool value) {
    return value ? "true" : "false";
}

bool ConfigItem::parseInt(int &value, const char* s) {
    if (!s) { return false; }
    char* end = nullptr;
    long longVal = std::strtol(s, &end, 0);
    bool result = (end && !*end);
    if (result) { value = int(longVal); }
    return result;
}

std::string ConfigItem::formatInt(int value) {
    return std::to_string(value);
}

bool ConfigItem::parseFloat(float &value, const char* s) {
    if (!s) { return false; }
    char* end = nullptr;
    float tempVal = std::strtof(s, &end);
    bool result = (end && !*end);
    if (result) { value = tempVal; }
    return result;
}

std::string ConfigItem::formatFloat(float value) {
    return std::to_string(value);
}

bool ConfigItem::parseEnum(int &value, const char* s, const EnumItem* items) {
    while (items && items->name) {
        if (stringEqualEx(items->name, s)) {
            value = items->value;
            return true;
        }
        ++items;
    }
    return false;
}

std::string ConfigItem::formatEnum(int value, const EnumItem* items) {
    while (items && items->name) {
        if (items->value == value) { return items->name; }
        ++items;
    }
    return "???";
}

bool ConfigItem::parseColor(uint32_t &value, const char* s) {
    if (!s) { return false; }
    if (*s == '#') { ++s; }
    // parse nibbles
    uint32_t accum = 0;
    int bit = 0;
    while (*s) {
        if (bit >= 32) { return false; /* number too long */ }
        char c = *s++;
        if      ((c >= '0') || (c <= '9')) { c -= '0'; }
        else if ((c >= 'a') || (c <= 'f')) { c -= 'a' - 10; }
        else if ((c >= 'A') || (c <= 'F')) { c -= 'A' - 10; }
        else { return false; }
        accum |= uint32_t(c) << bit;
        bit += 4;
    }
    if (bit <= 16) {
        // expand short form:                                          // 0000ABGR
        accum = ((accum & 0x0000FF00u) << 8) | (accum & 0x000000FFu);  // 00AB00GR
        accum = ((accum & 0x00F000F0u) << 4) | (accum & 0x000F000Fu);  // 0A0B0G0R
        accum |= accum << 4;                                           // AABBGGRR
        bit <<= 1;  // twice as many bits now
    }
    if (bit == 24) { accum |= 0xFF000000u; /* fully opaque */ }
    if (bit != 32) { return false; /* invalid number of bits */ }
    value = nibbleSwap(accum);
    return true;
}

std::string ConfigItem::formatColor(uint32_t value) {
    char str[10], *pos = str;
    *pos++ = '#';
    int len = ((value & 0xFF000000u) == 0xFF000000u) ? 24 : 32;
    value = nibbleSwap(value);
    for (int bit = 0;  bit < len;  bit += 4) {
        *pos++ = "0123456789abcdef"[(value >> bit) & 0xFu];
    }
    *pos = '\0';
    return str;
}

////////////////////////////////////////////////////////////////////////////////
