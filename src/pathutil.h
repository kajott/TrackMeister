// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <cstddef>

#include <string>
#include <functional>
#include <algorithm>

namespace PathUtil {

//! the platform's preferred path separator
extern "C" const char pathSep;

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

//! match a filename's extension to a zero-terminated list of (lowercase) FourCCs
bool matchExtList(const char* filename, const uint32_t *exts);

//! match a filename's extension to a zero-terminated list of (lowercase) FourCCs
inline bool matchExtList(std::string& filename, const uint32_t *exts)
    { return matchExtList(filename.c_str(), exts); }

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

//! check whether a path refers to a file
bool isFile(const char* path);
//! check whether a path refers to a file
inline bool isFile(const std::string& path)
    { return isFile(path.c_str()); }

//! check whether a path refers to a directory
bool isDir(const char* path);
//! check whether a path refers to a directory
inline bool isDir(const std::string& path)
    { return isDir(path.c_str()); }

//! determine the modification time of a file
//! \returns an opaque timestamp in an implementation-defined format;
//!          the only guarantee is that it's monotonically increasing,
//!          so comparisons between times are possible;
//!          returns 0 on failure
int64_t getFileMTime(const char* path);
inline int64_t getFileMTime(const std::string& path)
    { return getFileMTime(path.c_str()); }

//! search mode for findSibling()
enum class FindMode {
    First,     //!< lexicographically first file in the directory
    Last,      //!< lexicographically last file in the directory
    Previous,  //!< lexicographically previous file
    Next,      //!< lexicographically next file
    Random,    //!< pick any random file in the directory (except the current file)
};

//! find a sibling file in the same directory
//! \param path    full path of the reference file; <br>
//!                must end with a path separator if FindMode::First or
//!                FindMode::Last are used
//! \param mode    which file to return
//! \param filter  optional function to be called for every filename (without full path);
//!                returns true if the file is valid, or false if it shall be ignored
std::string findSibling(const std::string& path, FindMode mode, std::function<bool(const char*)> filter=nullptr);

}  // namespace PathUtil
