// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>

#include <string>
#include <algorithm>

namespace PathUtil {

//! the platform's preferred path separator
extern const char pathSep;

//! check if a character is a path separator
inline constexpr bool isPathSep(char c)
    { return (c == '/') || (c == '\\'); }

//! determine index of last path separator in 'path' (or 0 if none found)
size_t pathSepPos(const std::string& path);

//! determine index of the first character of the filename in 'path'
size_t filenameStartPos(const std::string& path);

//! determine index of the extension separator in 'path' (or path length if none found)
size_t extSepPos(const std::string& path);

//! check if a path is an absolute path
bool isAbsPath(const std::string& path);

//! determine directory part of a path
inline std::string dirname(const std::string& path)
    { return path.substr(0, pathSepPos(path)); }

//! determine directory part of a path (modify path in-place)
inline void dirnameInplace(std::string& path)
    { path.resize(pathSepPos(path)); }

//! determine filename part of a path
inline std::string basename(const std::string& path)
    { return path.substr(filenameStartPos(path)); }

//! determine filename part of a path (modify path in-place)
inline void basenameInplace(std::string& path)
    { path.erase(0, filenameStartPos(path)); }

//! join two paths
std::string join(const std::string& a, const std::string& b);

//! join two paths (modify first path in-place)
void joinInplace(std::string& a, const std::string& b);

//! extract extension from a filename
inline std::string getExt(const std::string& filename)
    { return filename.substr(std::min(filename.size(), extSepPos(filename) + 1u)); }

//! get file extension as a FourCC (converted to all-lowercase)
uint32_t getExtFourCC(const char* filename);

//! get file extension as a FourCC (converted to all-lowercase)
inline uint32_t getExtFourCC(const std::string& filename)
    { return getExtFourCC(filename.c_str()); }

//! remove extension from a filename
inline std::string stripExt(const std::string& filename)
    { return filename.substr(0, extSepPos(filename)); }

//! remove extension from a filename (modify filename in-place)
inline void stripExtInplace(std::string& filename)
    { filename.resize(extSepPos(filename)); }

//! match a filename against a pattern
//! \note Caveats:
//! <br> - always case-insensitive
//! <br> - only works properly for filenames, not full paths
//! <br> - only supports one '*' wildcard; no '?', no multiple wildcards!
bool matchFilename(const std::string& pattern, const std::string& filename);

//! check whether a path refers to a directory
bool isDir(const char* path);
//! check whether a path refers to a directory
inline bool isDir(const std::string& path)
    { return isDir(path.c_str()); }

//! find a sibling file in the same directory
//! \param path  full path of the reference file; <br>
//!              if the path ends with a path separator,
//!              the first/last file in the directory is searched instead
//! \param next  false = find previous/last file in lexicographical order; <br>
//!              true  = find next/first file <br> (matching is case-insensitive)
//! \param exts  optional zero-terminated list of valid extension code FourCCs
//!              (must be lowercase, and without the dot)
std::string findSibling(const std::string& path, bool next, const uint32_t* exts=nullptr);

}  // namespace PathUtil
