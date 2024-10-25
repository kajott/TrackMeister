// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS  // disable nonsense MSVC warnings

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <list>
#include <string>
#include <functional>

#include "util.h"
#include "pathutil.h"
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
        if (toLower(*sA++) != toLower(*sB++)) { return false; }
    }
    while (*sA && isIgnored(*sA)) { ++sA; }
    while (*sB && isIgnored(*sB)) { ++sB; }
    return !*sA && !*sB;
}

//! pairwise swap of nibbles in a 32-bit value
inline uint32_t nibbleSwap(uint32_t x) {
    return ((x & 0xF0F0F0F0u) >> 4) | ((x & 0x0F0F0F0Fu) << 4);
}

//! parse a color value
bool parseColor(uint32_t *pValue, const char* s) {
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
    *pValue = nibbleSwap(accum);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void ConfigParserContext::error(const char* msg, const char* s) const {
    fprintf(stderr, "%s:%d: %s '%s'\n", filename.c_str(), lineno, msg, s);
}

const ConfigItem* ConfigItem::find(const char* key) {
    for (const ConfigItem* item = g_ConfigItems;  item->valid();  ++item) {
        if ((item->type != DataType::SectionHeader) && stringEqualEx(item->name, key)) {
            return item;
        }
    }
    return nullptr;
}

bool ConfigItem::parse(ConfigParserContext& ctx, Config& cfg, const char* value) const {
    if (!value || !ptr) { return false; }
    void* pValue = ptr(cfg);
    bool ok = false;
    switch (type) {
        case DataType::String:
            static_cast<std::string*>(pValue)->assign(value);
            ok = true;
            break;

        case DataType::Bool: {
            bool res = false;
            ok = true;
            if      (stringEqualEx(value, "0"))        { res = false; }
            else if (stringEqualEx(value, "1"))        { res = true; }
            else if (stringEqualEx(value, "false"))    { res = false; }
            else if (stringEqualEx(value, "true"))     { res = true; }
            else if (stringEqualEx(value, "off"))      { res = false; }
            else if (stringEqualEx(value, "on"))       { res = true; }
            else if (stringEqualEx(value, "no"))       { res = false; }
            else if (stringEqualEx(value, "yes"))      { res = true; }
            else if (stringEqualEx(value, "disabled")) { res = false; }
            else if (stringEqualEx(value, "enabled"))  { res = true; }
            else if (stringEqualEx(value, "disable"))  { res = false; }
            else if (stringEqualEx(value, "enable"))   { res = true; }
            else { ok = false; }
            if (ok) { *static_cast<bool*>(pValue) = res; }
            break; }

        case DataType::Int: {
            char* end = nullptr;
            long res = std::strtol(value, &end, 0);
            ok = (end && !*end);
            if (ok) { *static_cast<int*>(pValue) = int(res); }
            break; }

        case DataType::Float: {
            char* end = nullptr;
            float res = std::strtof(value, &end);
            ok = (end && !*end);
            if (ok) { *static_cast<float*>(pValue) = res; }
            break; }

        case DataType::Color:
            ok = parseColor(static_cast<uint32_t*>(pValue), value);
            break;

        case DataType::Enum: {
            const char* p = values;
            if (!p) { ctx.error("internal error: no values for key", ctx.key.c_str()); p = ""; }
            int n = 0;
            while (*p) {
                if (stringEqualEx(p, value)) {
                    *static_cast<int*>(pValue) = n;
                    ok = true;
                    break;
                }
                p += strlen(p) + 1u;
                ++n;
            }
            break; }

        default:  // unknown type, WTF?
            ctx.error("internal error: invalid data type for key", ctx.key.c_str());
            break;
    }
    if (ok) {
        cfg.set.add(ordinal);
    } else {
        ctx.error("invalid value", value);
    }
    return ok;
}

std::string ConfigItem::format(const Config& cfg) const {
    const void* pValue = ptr ? ptr(const_cast<Config&>(cfg)) : nullptr;
    std::string res;
    switch (type) {
        case DataType::SectionHeader:
            res.assign(description);
            break;

        case DataType::String:
            res.assign(*static_cast<const std::string*>(pValue));
            break;

        case DataType::Bool:
            res.assign(*static_cast<const bool*>(pValue) ? "true" : "false");
            break;

        case DataType::Int:
            res.assign(std::to_string(*static_cast<const int*>(pValue)));
            break;

        case DataType::Float: {
            char buffer[32];
            ::snprintf(buffer, 32, "%.3f", *static_cast<const float*>(pValue));
            res.assign(buffer);
            break; }

        case DataType::Color: {
            char str[10], *pos = str;
            *pos++ = '#';
            uint32_t value = *static_cast<const uint32_t*>(pValue);
            int len = ((value & 0xFF000000u) == 0xFF000000u) ? 24 : 32;
            value = nibbleSwap(value);
            for (int bit = 0;  bit < len;  bit += 4) {
                *pos++ = "0123456789abcdef"[(value >> bit) & 0xFu];
            }
            *pos = '\0';
            res.assign(str);
            break; }

        case DataType::Enum: {
            const char* p = values ? values : "";
            for (int n = *static_cast<const int*>(pValue);  *p && (n > 0);  --n) {
                p += strlen(p) + 1u;
            }
            res.assign(p);
            break; }

        default:  // unknown type, WTF?
            break;
    }
    return res;
}

////////////////////////////////////////////////////////////////////////////////

bool Config::load(const char* filename, const char* matchName) {
    if (!filename || !filename[0]) { return false; }
    Dprintf("Config::load('%s', '%s')\n", filename, matchName ? matchName : "");
    FILE* f = fopen(filename, "r");
    if (!f) { return false; }

    // iterate over lines
    constexpr int LineBufferSize = 128;
    char line[LineBufferSize];
    ConfigParserContext ctx;
    ctx.filename.assign(filename);
    ctx.lineno = 0;
    bool ignoreNextLineSegment = false;
    bool validSection = !matchName;
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
            validSection = (matchName && matchName[0])
                         ? PathUtil::matchFilename(key, matchName)
                         : (stringEqualEx(key, "TrackMeister") || stringEqualEx(key, "TM"));
            Dprintf("  - %s section '%s'\n", validSection ? "parsing" : "ignoring", key);
            continue;
        }
        if (!value) { ctx.error("no value for key", key); continue; }

        // parse the actual value
        if (validSection) {
            ctx.key.assign(key);
            const ConfigItem *item = ConfigItem::find(key);
            if (item) { item->parse(ctx, *this, value); }
            else      { ctx.error("invalid key", key); }
        }
    }
    fclose(f);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

Config::PreparedCommandLine Config::prepareCommandLine(int& argc, char** argv) {
    PreparedCommandLine cmdline;
    int argpOut = 1;
    for (int argpIn = 1;  argpIn < argc;  ++argpIn) {
        if (argv[argpIn][0] == '+') {
            cmdline.emplace_back(&argv[argpIn][1]);
        } else {
            argv[argpOut++] = argv[argpIn];
            cmdline.emplace_back("");
        }
    }
    argc = argpOut;
    return cmdline;
}

void Config::load(const Config::PreparedCommandLine& cmdline) {
    ConfigParserContext ctx;
    ctx.filename.assign("<cmdline>");
    ctx.lineno = 0;
    for (const auto& arg : cmdline) {
        ctx.lineno++;
        if (arg.empty()) { continue; }
        auto sep = arg.find_first_of(":=");
        if (sep == std::string::npos) {
            ctx.error("syntax error", arg.c_str());
            continue;
        }
        ctx.key = arg.substr(0, sep);
        const ConfigItem *item = ConfigItem::find(ctx.key.c_str());
        if (item) { item->parse(ctx, *this, arg.substr(sep + 1u).c_str()); }
        else      { ctx.error("invalid key", ctx.key.c_str()); }
    }
}

////////////////////////////////////////////////////////////////////////////////

bool Config::save(const char* filename) {
    if (!filename || !filename[0]) { return false; }
    Dprintf("Config::save('%s')\n", filename);
    FILE* f = fopen(filename, "w");
    if (!f) { return false; }
    bool res = (fwrite(g_DefaultConfigFileIntro, std::strlen(g_DefaultConfigFileIntro), 1, f) == 1);
    for (const ConfigItem *item = g_ConfigItems;  item->valid();  ++item) {
        if (item->type == ConfigItem::DataType::SectionHeader) {
            fprintf(f, "\n; %s\n", item->description);
            continue;
        }
        if (item->flags & ConfigItem::Flags::Hidden) { continue; }
        std::string name(item->name);
        if (int(name.size()) < g_ConfigItemMaxNameLength) {
            name.resize(g_ConfigItemMaxNameLength, ' ');
        }
        fprintf(f, "%s = %-10s ; %s\n", name.c_str(), item->format(*this).c_str(), item->description);
    }
    fclose(f);
    return res;
}

bool Config::saveLoudness(const char* filename) {
    if (!isValidLoudness(loudness)) { return false; }
    if (!filename || !filename[0]) { return false; }
    FILE* f = fopen(filename, "a");
    if (!f) { return false; }
    fprintf(f, "\n; EBU R128 loudness scan result for samplerate=%d, filter=%s, stereo_separation=%d:\nloudness = %.2f\n",
               sampleRate, ConfigItem::find("filter")->format(*this).c_str(), stereoSeparation, loudness);
    fclose(f);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void Config::import(const Config& src) {
    for (const ConfigItem *item = g_ConfigItems;  item->valid();  ++item) {
        if (src.set.contains(item->ordinal) && item->copy) {
            item->copy(src, *this);
        }
    }
    set.update(src.set);
}

void Config::importAllUnset(const Config& src) {
    for (const ConfigItem *item = g_ConfigItems;  item->valid();  ++item) {
        if (!set.contains(item->ordinal) && item->copy) {
            item->copy(src, *this);
        }
    }
}
