// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS  // disable nonsense MSVC warnings

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <string>
#include <functional>

#include "util.h"
#include "config.h"
#include "config_item.h"

////////////////////////////////////////////////////////////////////////////////

//! check if a character is considered as ignored in INI file key comparisons
constexpr inline bool isIgnored(char c)
    { return (c == ' ') || (c == '_') || (c == '-'); }

//! compare strings for equality, ignoring case and whitespace
bool stringEqualEx(const char* sA, const char* sB) {
    if (!sA || !sB) { return false; }
    while (*sA && *sB) {
        while (*sA && isIgnored(*sA)) { ++sA; }
        while (*sB && isIgnored(*sB)) { ++sB; }
        char cA = *sA++;
        char cB = *sB++;
        if ((cA >= 'a') && (cA <= 'z')) { cA -= 32; }
        if ((cB >= 'a') && (cB <= 'z')) { cB -= 32; }
        if (cA != cB) { return false; }
    }
    while (*sA && isIgnored(*sA)) { ++sA; }
    while (*sB && isIgnored(*sB)) { ++sB; }
    return !*sA && !*sB;
}

//! pairwise swap of nibbles in a 32-bit value
inline uint32_t nibbleSwap(uint32_t x) {
    return ((x & 0xF0F0F0F0u) >> 4) | ((x & 0x0F0F0F0Fu) << 4);
}

////////////////////////////////////////////////////////////////////////////////

void ConfigParserContext::error(const char* msg, const char* s) {
    fprintf(stderr, "%s:%d: %s '%s'\n", filename.c_str(), lineno, msg, s);
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
    char buffer[32];
    ::snprintf(buffer, 32, "%.3f", value);
    return buffer;
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
        if (bit >= 32) { return false;  /* number too long */ }
        char c = *s++;
        if      ((c >= '0') && (c <= '9')) { c -= '0'; }
        else if ((c >= 'a') && (c <= 'f')) { c -= 'a' - 10; }
        else if ((c >= 'A') && (c <= 'F')) { c -= 'A' - 10; }
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
    if      (bit == 24) { accum |= 0xFF000000u;  /* fully opaque */ }
    else if (bit != 32) { return false;  /* invalid number of bits */ }
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

bool Config::load(const char* filename, const char* matchName) {
    if (!filename || !filename[0]) { return false; }
    Dprintf("Config::load('%s', '%s')\n", filename, matchName ? matchName : "");
    FILE* f = fopen(filename, "r");
    if (!f) { return false; }
(void)matchName;

    // iterate over lines
    constexpr int LineBufferSize = 128;
    char line[LineBufferSize];
    ConfigParserContext ctx;
    ctx.filename.assign(filename);
    ctx.lineno = 0;
    bool ignoreNextLineSegment = false;
    bool validSection = true;
    while (std::fgets(line, LineBufferSize, f)) {
        // check end of line
        if (!line[0]) { break; /* EOF */ }
        bool ignoreThisSegment = ignoreNextLineSegment;
        ignoreNextLineSegment = (line[std::strlen(line) - 1u] != '\n');
        if (ignoreThisSegment) { continue; }
        ctx.lineno++;

        // parse the line into a key/value pair
        char *key = nullptr, *end = nullptr, *value = nullptr;
        bool valid = false;
        for (char* pos = line;  *pos && (*pos != ';');  ++pos) {
            if (key && valid && !value && ((*pos == ':') || (*pos == '='))) {
                *end = '\0';  value = pos;  valid = false;
            } else if (!isSpace(*pos)) {
                end = &pos[1];
                if (!valid) { if (!key) { key = pos; } else { value = pos; } }
                valid = true;
            }
        }
        if (!valid) { if (value) { value = nullptr; } else { key = nullptr; } }
        if (end) { *end = '\0'; }

        // empty line? separator?
        if (!key) { continue; }
        if ((key[0] == '[') && !value && (end >= key) && (end[-1] == ']')) {
            ++key;  end[-1] = '\0';
            validSection = stringEqualEx(key, "TMCP");
            Dprintf("  - %s section '%s'\n", validSection ? "parsing" : "ignoring", key);
            continue;
        }
        if (!value) { ctx.error("no value for key", key); continue; }

        // parse the actual value
        if (validSection) {
            const ConfigItem *item;
            for (item = g_ConfigItems;  item->name;  ++item) {
                if (stringEqualEx(item->name, key)) {
                    item->setter(ctx, *this, value);
                    break;
                }
            }
            if (!item->name) { ctx.error("invalid key", key); }
        }
    }
    fclose(f);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool Config::save(const char* filename) {
    if (!filename || !filename[0]) { return false; }
    Dprintf("Config::save('%s')\n", filename);
    FILE* f = fopen(filename, "w");
    if (!f) { return false; }
    bool res = (fwrite(g_DefaultConfigFileIntro, std::strlen(g_DefaultConfigFileIntro), 1, f) == 1);
    for (const ConfigItem *item = g_ConfigItems;  item->name;  ++item) {
        if (item->newGroup) { fprintf(f, "\n"); }
        fprintf(f, "%s = %-10s ; %s\n", item->name, item->getter(*this).c_str(), item->description);
    }
    fclose(f);
    return res;
}
