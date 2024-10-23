// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#include <string>
#include <functional>

#include "config.h"

struct EnumItem {
    const char* name;
    int value;
};

struct ConfigParserContext {
    std::string filename;
    int lineno;
    void error(const char* msg, const char* s) const;
    inline bool checkParseResult(bool result, const char* s) const {
        if (!result) { error("invalid value", s); }
        return result;
    }
};

struct ConfigItem {
    enum Flags : int {
        NewGroup = (1 << 0),  // new option group (
        Reload   = (1 << 1),  // module reload required after changing
        Image    = (1 << 2),  // image reload required after changing
        Global   = (1 << 3),  // only relevant for global configuration
        Startup  = (1 << 4),  // only evaluated at application startup (implies Global)
        Hidden   = (1 << 5),  // hide from interactive configuration UI
    };

    int ordinal;
    int flags;
    const char* name;
    const char* description;
    std::function<std::string(const Config& cfg)> getter;
    std::function<void(ConfigParserContext& ctx, Config& cfg, const char* s)> setter;
    std::function<void(const Config& src, Config& dest)> copy;

    static bool parseBool(bool &value, const char* s);
    static std::string formatBool(bool value);
    static bool parseInt(int &value, const char* s);
    static std::string formatInt(int value);
    static bool parseFloat(float &value, const char* s);
    static std::string formatFloat(float value);
    static bool parseEnum(int &value, const char* s, const EnumItem* items);
    static std::string formatEnum(int value, const EnumItem* items);
    static bool parseColor(uint32_t &value, const char* s);
    static std::string formatColor(uint32_t value);
};

extern const ConfigItem g_ConfigItems[];
extern const char* g_DefaultConfigFileIntro;
extern const int g_ConfigItemMaxNameLength;