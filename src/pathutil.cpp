// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <cstddef>

#include <string>
#include <algorithm>

#include "util.h"
#include "pathutil.h"

#ifdef _WIN32
    extern "C" const char pathSep = '\\';
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#else
    extern "C" const char pathSep = '/';
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <dirent.h>
#endif

namespace PathUtil {

size_t pathSepPos(const std::string& path) {
    size_t res = 0u;
    for (size_t i = 0u;  i < path.size();  ++i) {
        if (isPathSep(path[i])) { res = i; }
    }
    return res;
}

size_t filenameStartPos(const std::string& path) {
    size_t res = 0u;
    for (size_t i = 0u;  i < path.size();  ++i) {
        if (isPathSep(path[i])) { res = i + 1u; }
    }
    return std::min(res, path.size());
}

size_t extSepPos(const std::string& path) {
    size_t res = path.size();
    for (size_t i = 0u;  i < path.size();  ++i) {
        if (isPathSep(path[i])) { res = path.size(); }
        if (path[i] == '.') { res = i; }
    }
    return res;
}

uint32_t getExtFourCC(const char* filename) {
    if (!filename) { return 0u; }
    const char* ext = nullptr;
    while (*filename) {
        if (isPathSep(*filename)) { ext = nullptr; }
        if (*filename == '.') { ext = &filename[1]; }
        ++filename;
    }
    if (!ext) { return 0u; }
    uint32_t fourCC = 0u;
    for (int bit = 0;  (bit < 32) && *ext;  bit += 8) {
        fourCC |= uint32_t(toLower(*ext++)) << bit;
    }
    return fourCC;
}

bool isAbsPath(const std::string& path) {
    if (!path.empty() && isPathSep(path[0])) { return true; }
    #ifdef _WIN32
        if ((path.size() >= 3u)
        && (((path[0] >= 'a') && (path[0] <= 'z')) ||
            ((path[0] >= 'A') && (path[0] <= 'Z')))
        &&   (path[1] == ':')
        &&   isPathSep(path[2]))
            { return true; }
    #endif
    return false;
}

std::string join(const std::string& a, const std::string& b) {
    if (b.empty()) { return a; }
    if (a.empty() || isAbsPath(b)) { return b; }
    if (isPathSep(a[a.size() - 1u])) { return a + b; }
    else { return a + std::string(1, pathSep) + b; }
}

void joinInplace(std::string& a, const std::string& b) {
    if (b.empty()) { return; }
    if (a.empty() || isAbsPath(b)) { a.assign(b); return; }
    a.reserve(a.size() + b.size() + 1u);
    if (!isPathSep(a[a.size() - 1u])) { a.append(1, pathSep); }
    a.append(b);
}

bool matchFilename(const std::string& pattern, const std::string& filename) {
    if (pattern.empty() || filename.empty()) { return false; }
    size_t patternPos = 0, filenamePos = 0;
    while ((patternPos < pattern.size()) && (filenamePos < filename.size())) {
        char pc = pattern[patternPos++];
        if (pc == '*') {  // wildcard -> jump to end of filename
            size_t d = pattern.size() - patternPos;  // distance to end of pattern
            if (d > filename.size()) { return false; }  // pattern tail is longer than the entire filename
            size_t p = filename.size() - d;  // new filename position
            if (p < patternPos) { return false; }  // head and tail overlap
            filenamePos = p;
        } else if (toLower(pc) != toLower(filename[filenamePos++])) { return false; }
    }
    return (patternPos == pattern.size()) && (filenamePos == filename.size());
}

bool isDir(const char* path) {
    #ifdef _WIN32
        DWORD attr = GetFileAttributesA(path);
        if (attr == INVALID_FILE_ATTRIBUTES) { return false; }
        return !!(attr & FILE_ATTRIBUTE_DIRECTORY);
    #else
        struct stat st;
        if (stat(path, &st) < 0) { return false; }
        return !!S_ISDIR(st.st_mode);
    #endif
}

int64_t getFileMTime(const char* path) {
    if (!path || !path[0]) { return 0; }
    #ifdef _WIN32
        HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) { return 0; }
        FILETIME mtime;
        int64_t res = 0;
        if (GetFileTime(hFile, nullptr, nullptr, &mtime)) {
            res = (int64_t(mtime.dwHighDateTime) << 32) + int64_t(mtime.dwLowDateTime);
        }
        CloseHandle(hFile);
        return res;
    #else
        struct stat st;
        if (stat(path, &st) < 0) { return 0; }
        return int64_t(st.st_mtime);
    #endif
}

std::string findSibling(const std::string& path, bool next, const uint32_t* exts) {
    // prepare directory and base name
    std::string dir(dirname(path));
    if (dir.empty()) { dir.assign("."); }
    std::string ref(basename(path));
    if (ref.empty() && !next) { ref.assign("\xff"); }
    std::string best;
    const char* curr;

    // start directory enumeration loop
    #ifdef _WIN32
        std::string searchPath(dir);
        searchPath.append("\\*.*");
        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &fd);
        if (hFind == INVALID_HANDLE_VALUE) { return ""; }
        do {
            curr = fd.cFileName;
    #else
        DIR* dd = opendir(dir.c_str());
        if (!dd) { return ""; }
        struct dirent *entry;
        while ((entry = readdir(dd))) {
            curr = entry->d_name;
    #endif

            // check entry (1): ignore dot files
            if (!curr[0] || (curr[0] == '.')) { continue; }

            // check entry (2): match extensions
            if (exts) {
                uint32_t ext = getExtFourCC(curr);
                const uint32_t* checkExt;
                for (checkExt = exts;  *checkExt && (*checkExt != ext);  ++checkExt);
                if (!*checkExt) { continue; }
            }

            // check entry (3): compare to see where it slots in
            auto cmp = [] (const char* check, const std::string& sRef, int ifRefEmpty=0) {
                if (sRef.empty() && ifRefEmpty) { return ifRefEmpty; }
                const char* ref = sRef.c_str();
                while (*check && *ref) {
                    int d = int(uint8_t(toLower(*check++))) - int(uint8_t(toLower(*ref++)));
                    if (d) { return d; }
                }
                return int(uint8_t(toLower(*check))) - int(uint8_t(toLower(*ref)));
            };
            if ((next && ((cmp(curr, ref) <= 0) || (cmp(curr, best, -1) > 0)))
            || (!next && ((cmp(curr, ref) >= 0) || (cmp(curr, best, +1) < 0))))
                { continue; }

            // check entry (4): confirm that it's a file, not a directory
            #ifdef _WIN32
                if (fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY)) { continue; }
            #else
                std::string temp(join(dir, curr));
                struct stat st;
                if (stat(temp.c_str(), &st) < 0) { continue; }
                if (!S_ISREG(st.st_mode)) { continue; }
            #endif

            // all checks passed -> this is the new best match
            best.assign(curr);

    // end directory enumeration loop
    #ifdef _WIN32
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    #else
        }  // while (readdir)
        closedir(dd);
    #endif

    // return final path
    if (best.empty()) { return ""; }
    return join(dir, best);
}

}  // namespace PathUtil
