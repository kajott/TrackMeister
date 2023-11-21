// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <cstddef>

#include <string>

#include "pathutil.h"

namespace PathUtil {

#ifdef _WIN32
    const char pathSep = '\\';
#else
    const char pathSep = '/';
#endif

size_t pathSepPos(const std::string& path) {
    size_t res = 0u;
    for (size_t i = 0u;  i < path.size();  ++i) {
        if (isPathSep(path[i])) { res = i; }
    }
    return res;
}

size_t extSepPos(const std::string& path) {
    size_t res = path.size();
    for (size_t i = 0u;  i < path.size();  ++i) {
        if (isPathSep(path[i])) { res = path.size(); }
        if (path[i] == '.') { res = i; }
    }
    return res;
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

}  // namespace PathUtil
