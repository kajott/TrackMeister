#!/usr/bin/env python3
import io
import re
import sys

uppercase_words = "X Y VU".lower().split()
def camelCase_to_space_case(x):
    words = ''.join(((' ' + c.lower()) if c.isupper() else c) for c in x).split()
    return ' '.join((word.upper() if (word in uppercase_words) else word) for word in words)

if __name__ == "__main__":
    try:
        _, infile, outfile = sys.argv
    except ValueError:
        print("Usage:", sys.argv[0], "<config.h> <config_data.cpp>", file=sys.stderr)
        sys.exit(2)

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

    enums = {}
    fields = []
    intro = [
        '; Edit and save this file as "tm.ini" in the program directory',
        '; to customize TrackMeister appearance and behavior.',
        '',
        '[TrackMeister]'
    ]
    name_maxlen = 0
    flags_maxlen = 0

    with open(infile, 'r') as f:
        in_enum, in_config = None, False
        lineno = 0
        comment = []
        for line in f:
            lineno += 1
            line = line.strip()

            # empty line -> clear comment block
            if not line:
                comment = []

            # comment
            m = re.match(r'//!\s?(.*)', line)
            if m:
                c = m.group(1)
                if not c.lower().startswith("in code,"):
                    comment.append('; ' + c)
                continue

            # start of enum
            m = re.match(r'enum class (\w+) : int {', line)
            if m:
                in_enum = m.group(1)
                enums[in_enum] = []
                continue

            # enum item
            m = in_enum and re.match(r'(\w+)(\s*=\s*(\d+))?,?', line)
            if m:
                n = m.group(3)
                if n:
                    assert int(n) == len(enums[in_enum]), "invalid enum order"
                enums[in_enum].append(m.group(1))
                continue

            # start of config structure
            if line.startswith('struct Config {'):
                in_config = True
                new_section = True
                intro += comment
                continue

            # empty line inside config structure
            if in_config and not(line):
                new_section = True
                continue

            # comment at section start inside config structure
            m = in_config and new_section and re.match(r'''
                ^ // (?!!)
                \s* (?P<name>.*?)
                (\s* \[ (?P<flags> .*?) \] \s* )?
            $''', line, flags=re.X)
            if m:
                name = m.group('name')
                flags = m.group('flags')
                flags = {f.strip() for f in flags.split(',')} if flags else set()
                fields.append((flags, None, None, None, name))
                continue

            # config item
            m = in_config and re.match(r'''
                (?P<type>   [a-zA-Z0-9_:]+) \s*
                (?P<field>  \w+) \s*
                ( = \s* (?P<default> .*?) )? ;
                \s* //!< \s*
                (?P<comment> .*?)
                (\s* \[ (?P<flags> .*?) \] \s* )?
            $''', line, flags=re.X)
            if m:
                type = m.group('type')
                field = m.group('field')
                flags = m.group('flags')
                flags = {f.strip() for f in flags.split(',')} if flags else set()
                name = camelCase_to_space_case(field)
                name_maxlen = max(name_maxlen, len(name))
                flags_maxlen = max(flags_maxlen, len(flags))
                if not(type in {'bool', 'int', 'float', 'uint32_t', 'std::string'}) and not(type in enums):
                    print(f"{infile}:{lineno}: invalid type '{type}'", file=sys.stderr)
                else:
                    fields.append((flags, type, field, name, m.group('comment').strip()))
                new_section = False

            # end of struct or enum
            if line.startswith('};'):
                in_enum, in_config = None, False
                continue

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

    with io.StringIO() as f:
        f.write('// This file has been generated by generate_config_data.py.\n')
        f.write('// DO NOT EDIT! Changes will be overwritten without asking.\n\n')
        f.write('#include <cstdint>\n\n')
        f.write('#include "config.h"\n#include "config_item.h"\n')
        f.write(f'\nconst int g_ConfigItemMaxNameLength = {name_maxlen};\n')

        # write main config item table
        f.write('\nconst ConfigItem g_ConfigItems[] = { {\n')
        first = True
        ordinal = 0
        for flags, type, field, name, desc in fields:
            if first: first = False
            else: f.write('    }, {\n')

            # extract special flags
            xflags = {f for f in flags if f.lower().startswith(("min ", "max ", "values "))}
            flags -= xflags
            flags = " | ".join("ConfigItem::Flags::" + f.capitalize() for f in sorted(flags)) or "0"

            # handle section header
            if not type:
                desc = desc.replace('"', '\\"')
                f.write(f'        0, ConfigItem::DataType::SectionHeader, {flags}, nullptr,\n        "{desc}",\n        nullptr, 0.0f, 0.0f, nullptr, nullptr\n')
                continue
            lname = field.lower()
            ldesc = desc.lower()

            # add possible values to description
            if type in enums:
                desc += " [possible values: " + ", ".join(f"'{e}'" for e in enums[type]) + "]"

            # resolve DataType
            if type in enums:
                dt = "Enum"
            elif type == "uint32_t":
                dt = "Color"
            else:
                dt = type.split('::', 1)[-1].capitalize()

            # resolve value ranges, step 1: load defaults
            values = "nullptr"
            vmin = 0
            vmax = 1000 if (type == "int") else 1
            if type in enums:
                values = enums[type]
            if "percent" in ldesc:
                vmax = 100
            elif "pos" in lname:
                vmax = 1000
            elif ("margin" in lname) or ("padding" in lname):
                vmax = 100
            elif "textsize" in lname:
                vmin, vmax = 1, 200
            elif "spacing" in lname:
                vmin, vmax = -100, 100
            elif "shadowsize" in lname:
                vmax = 100
            elif ("duration" in lname) or ("fadetime" in lname):
                vmax = 60
            elif "decibels" in ldesc:
                vmin, vmax = -24, 24

            # resolve value ranges, step 2: override with special flags
            for k, v in (f.split(maxsplit=1) for f in xflags):
                k = k.lower()
                if k == "min": vmin = float(v)
                if k == "max": vmax = float(v)
                if k == "values": values = [x.strip() for x in v.split("|")]
            if isinstance(values, (list, tuple)):
                values = '"{}\\0\\0"'.format('\\0'.join(values))

            # write name and description
            ordinal += 1
            f.write(f'        {ordinal}, ConfigItem::DataType::{dt}, {flags},\n')
            f.write(f'        "{name}",\n')
            desc = desc.replace('"', '\\"')
            f.write(f'        "{desc}",\n')
            f.write(f'        {values}, {vmin:.1f}f, {vmax:.1f}f,\n')

            # write methods
            f.write(f'        [] (Config& src) -> void* {{ return static_cast<void*>(&src.{field}); }},\n')
            f.write(f'        [] (const Config& src, Config& dest) {{ dest.{field} = src.{field}; }}\n')

        f.write('    },\n    { 0, ConfigItem::DataType::SectionHeader, 0, nullptr, nullptr, nullptr, 0.0f, 0.0f, nullptr, nullptr }\n};\n')

        f.write('\nconst char* g_DefaultConfigFileIntro =\n')
        first = True
        for line in intro:
            f.write((not(first) and '"\n' or '') + '    "' + line.replace('"', '\\"') + '\\n')
            first = False
        f.write('";\n')

        data = f.getvalue()

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

    with open(outfile, 'wb') as f:
        f.write(data.encode('utf-8').replace(b'\r', b''))
