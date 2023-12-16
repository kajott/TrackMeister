#!/usr/bin/env python3
"""
A very simple and limited Markdown to HTML exporter.

Only supports a small subset of Markdown:
- Only **bold**, *italic*, _italic_ and `monospace` inline styles supported.
- Inline links are supported, but reference links aren't.
- Images are included inline as base64 (JPEG/PNG) or text (SVG) data URIs.
- Embedded HTML is **not** supported, except <br>.
- Preformatted code blocks must use 4-space indentation syntax, no fenced blocks.
- Headings must use unclosed ATX style ('#' at the beginning of the line),
  not Setext-style underlined headings.
- Rudimentary table support; alignment is ignored.
- Unordered lists only, bullet must be a dash ('-').
- No block quotes.
- No horizontal rules.
- There are certainly lots of other quirks that are non-standard.
"""
# SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
# SPDX-License-Identifier: MIT

import argparse
import base64
import io
import os
import re
import sys

def H(s):
    return s.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;')

def count_and_strip_leading(s, c):
    l = len(s)
    s = s.lstrip(c)
    return (s, l - len(s))

def make_id(s):
    return '-'.join(word.lower() for word in re.findall('[a-zA-Z0-9]+', s))

def wrap(s):
    return ''.join(f"\x01{ord(c):X}" for c in s) + '\x02'
def unwrap(s):
    return re.sub('\x01([0-9A-F]+)', lambda m: chr(int(m.group(1),16)), s).replace('\x02', '')

###############################################################################

HTMLHeader = '''<html><head>
<meta charset="utf-8">
<title>$TITLE</title>
<style type="text/css">
    body {
        max-width: 50em;
        margin-left: auto;
        margin-right: auto;
        font-family: "Segoe UI", "Noto Sans", "DejaVu Sans", Verdana, sans-serif;
    }
    img, svg {
        max-width: 95%;
    }
    code, pre {
        font-family: Inconsolata, "DejaVu Sans Mono", monospace;
    }
    code {
        background: rgba(0,0,0,0.015625);
        border: solid 1px rgba(0,0,0,0.125);
        border-radius: 0.2em;
    }
    pre {
        margin-left: 4em;
        margin-right: 4em;
        padding: .5em;
        background: rgba(0,0,0,0.0625);
        border: solid 1px rgba(0,0,0,0.25);
        border-radius: 0.5em;
    }
    table {
        border-collapse: collapse;
    }
    td, th {
        text-align: left;
        vertical-align: top;
        border: solid 1px rgba(0,0,0,0.375);
        padding: .125em .25em .125em .25em;
    }
    th {
        background: rgba(0,0,0,0.125);
    }
</style>
</head><body>
'''
HTMLFooter = '''
</body></html>
'''

###############################################################################

class MarkdownToHTML:
    def __init__(self, title=None, excludes=[], basedir=None):
        self.title = title
        self.excludes = set(map(make_id, excludes))
        self.basedir = basedir or ""
        self.html = io.StringIO()
        self.current_stack = []
        self.para_stack = []
        self.enum_indent_stack = []
        self.para = ""
        self.indent = 0
        self.auto_close = True
        self.table_header = True
        self.skip = 0

    def in_tag(self, tag):
        return self.para_stack and (self.para_stack[-1].split()[0] == tag)

    def feed(self, line):
        """feed a single line of text, or a file-like object"""
        if not isinstance(line, str):
            for actual_line in line:
                self.feed(actual_line)
            return

        rawline = line.rstrip()
        if not rawline:
            if self.in_tag("pre"):
                self.para += "\n"
            else:
                self.flush([])
                self.indent = 0
                self.enum_indent_stack = []
                self.table_header = True
            return
        indent = len(rawline)
        line = rawline.lstrip()
        indent -= len(line)
        bullet = line.startswith("- ")
        if bullet:
            line = line[1:].lstrip()
            indent += 2

        if self.para and not(bullet) and (indent >= self.indent):
            # add to existing paragraph
            if self.in_tag('pre'):
                self.para += "\n" + rawline[4:]
            elif self.para.endswith('-') or not(self.para):
                self.para += line
            else:
                self.para += " " + line
        elif bullet:
            # start enumeration
            self.enum_indent_stack = [i for i in self.enum_indent_stack if (i < indent)] + [indent]
            depth = len(self.enum_indent_stack)
            self.flush(['ul','li'] * (depth - 1) + ['ul'])  # ensure closing the previous item
            self.new_para(['ul','li'] * depth, line, indent, auto_close=False)
        elif indent >= 4:
            # start preformatted text
            self.new_para(['pre'], rawline[4:], indent)
        elif not(indent) and line.startswith('#'):
            # heading
            line, level = count_and_strip_leading(line, '#')
            self.flush([])
            if not self.title: self.title = line.strip()
            hid = make_id(line)
            if not(self.skip) or (level <= self.skip):
                self.skip = level if (hid in self.excludes) else 0
            self.new_para([f'h{level} id="{hid}"'], line.strip())
        elif not(indent) and line.startswith('|'):
            # table row
            td = 'th' if self.table_header else 'td'
            self.table_header = False
            cols = list(map(str.strip, line[1:].split('|')))
            if not cols[-1]: cols.pop()
            if any(c.strip('-:') for c in cols):
                for c in cols:
                    self.new_para(['table', 'tr', td], c, 0)
                self.flush(['table'])  # force-close row
        else:
            # start normal paragraph
            self.new_para(['p'], line, indent)

    def new_para(self, target_stack, content, indent=0, auto_close=True):
        self.flush(target_stack)
        self.para = content
        self.indent = indent
        self.auto_close = auto_close

    def flush(self, target_stack=None):
        if target_stack is None:
            target_stack = self.para_stack
        if not self.skip:
            ended = False
            if self.para:
                if self.in_tag("pre"):
                    self.html.write(H(self.para.rstrip()))
                else:
                    self.html.write(self.format_line(self.para))
                ended = self.auto_close and self.current_stack_pop()
            while self.current_stack and ((len(self.current_stack) > len(target_stack)) or (self.current_stack != target_stack[:len(self.current_stack)])):
                ended = self.current_stack_pop() or ended
            if ended:
                self.html.write('\n')
            while self.current_stack < target_stack:
                tag = target_stack[len(self.current_stack)]
                self.html.write(f'<{tag}>')
                self.current_stack.append(tag)
        self.para = ""
        self.para_stack = target_stack
        self.auto_close = True

    def current_stack_pop(self):
        if not self.current_stack: return False
        tag = self.current_stack.pop().split()[0]
        self.html.write(f'</{tag}>')
        return True

    def format_line(self, line):
        line = re.sub(r'<br\s*/?>', wrap("<br>"), line)
        line = re.sub(r'&([a-zA-Z0-9]+|#\d+|#[xX][0-9a-fA-F]+);', lambda m: wrap(m.group(0)), line)
        line = H(line)
        line = re.sub(r'`(.*?)`', lambda m: '<code>' + wrap(m.group(1)) + '</code>', line)
        line = re.sub(r'!\[(.*?)\]\((.*?)\)', lambda m: self.embed_image(m.group(2), m.group(1)), line)
        line = re.sub(r'\[(.*?)\]\((.*?)\)', lambda m: f'<a href="{wrap(m.group(2))}">{m.group(1)}</a>', line)
        line = re.sub(r'\*\*(.*?)\*\*', r'<strong>\1</strong>', line)
        line = re.sub(r'\*(.*?)\*', r'<em>\1</em>', line)
        line = re.sub(r'\b_(.*?)_\b', r'<em>\1</em>', line)
        return unwrap(line)

    def embed_image(self, uri, alt=None):
        data = None
        try:
            print(self.basedir)
            print(uri)
            if not uri.startswith(("http://", "https://", "/")):
                with open(os.path.join(self.basedir, uri), 'rb') as f:
                    data = f.read()
        except EnvironmentError:
            pass
        alt = f' alt="{wrap(alt)}"' if alt else ''
        ext = os.path.splitext(uri)[-1].strip('.').lower()
        ctype = {
            "jpg":  "image/jpeg",
            "jpeg": "image/jpeg",
            "png":  "image/png",
            "svg":  "image/svg+xml",
        }.get(ext)
        if data and (ext == "svg"):
            data = data.decode('utf-8', 'replace').strip()
            data = data.replace('\n', ' ').replace('\r', '').replace('\t', ' ')
            data = data.replace('%', '%25').replace('"', '%22').replace('<', '%3C').replace('>', '%3E').replace(' ', '%20')
            return f'<img{alt} src="data:{ctype};utf8,{data}">'
        if data and ctype:
            return f'<img{alt} src="data:{ctype};base64,{base64.b64encode(data).decode()}">'
        # fall back to normal <img> tag referencing an external image
        return f'<img src="{wrap(uri)}"{alt}>'

    def get_html(self):
        self.flush([])
        return HTMLHeader.replace("$TITLE", self.title) + self.html.getvalue().strip() + HTMLFooter

###############################################################################

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("infile", metavar="INPUT.md",
                        help="input Markdown file")
    parser.add_argument("-o", "--output", metavar="OUTPUT.html",
                        help="output HTML file [default: derive from input filename]")
    parser.add_argument("-T", "--title", metavar="TEXT",
                        help="document title [default: derive from first heading]")
    parser.add_argument("-x", "--exclude", metavar="ID", nargs='*',
                        help="exclude sections with specified IDs")
    args = parser.parse_args()

    infile = args.infile
    outfile = args.output
    if not outfile:
        outfile = os.path.splitext(infile)[0] + ".html"
    conv = MarkdownToHTML(title=args.title, excludes=args.exclude or [], basedir=os.path.dirname(infile))

    try:
        with open(infile, encoding='utf-8', errors='replace') as md:
            conv.feed(md)
    except EnvironmentError as e:
        print(f"ERROR: could not read input file '{infile}' - {e}", file=sys.stderr)
        sys.exit(1)

    try:
        with open(outfile, 'w', encoding='utf-8', errors='replace') as f:
            f.write(conv.get_html())
    except EnvironmentError as e:
        print(f"ERROR: could not write output file '{outfile}' - {e}", file=sys.stderr)
        sys.exit(1)
