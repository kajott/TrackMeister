// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#include <string>
#include <functional>

#include "config.h"

struct ConfigParserContext {
    std::string filename;
    int lineno;
    std::string key;
    void error(const char* msg, const char* s) const;
    inline bool checkParseResult(bool result, const char* s) const {
        if (!result) { error("invalid value", s); }
        return result;
    }
};

struct ConfigItem {
    enum class DataType : int {
        SectionHeader=0, String, Bool, Int, Float, Color, Enum
    };

    enum Flags : int {
        Hidden   = (1 << 0),  // hide from interactive configuration UI
        File     = (1 << 1),  // only relevant for per-file configuration
        Global   = (1 << 2),  // only relevant for global configuration
        Startup  = (1 << 3),  // only evaluated at application startup (implies Global)
        Reload   = (1 << 4),  // module reload required after changing
        Image    = (1 << 5),  // image reload required after changing
    };

    int ordinal;
    DataType type;
    int flags;
    const char* name;
    const char* description;
    const char* values;
    float vmin, vmax;
    std::function<void*(Config& src)> ptr;
    std::function<void(const Config& src, Config& dest)> copy;

    static const ConfigItem* find(const char* key);
    bool parse(ConfigParserContext& ctx, Config& cfg, const char* value) const;
    std::string format(const Config& cfg) const;
    inline bool valid() const { return (type != DataType::SectionHeader) || description; }
};

extern const ConfigItem g_ConfigItems[];
extern const char* g_DefaultConfigFileIntro;
extern const int g_ConfigItemMaxNameLength;
