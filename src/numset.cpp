// SPDX-FileCopyrightText: 2024 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <cstddef>

#include <vector>

#include "numset.h"

void NumberSet::add(int n) {
    if (n < 0) { return; }
    constexpr size_t BitsPerItem = 8u * sizeof(T);
    size_t idx = size_t(n) / BitsPerItem;
    if (idx >= bits.size()) {
        bits.resize(idx + 1u);
    }
    bits[idx] |= T(1) << (size_t(n) & (BitsPerItem - 1u));
}

void NumberSet::remove(int n) {
    if (n < 0) { return; }
    constexpr size_t BitsPerItem = 8u * sizeof(T);
    size_t idx = size_t(n) / BitsPerItem;
    if (idx < bits.size()) {
        bits[idx] &= ~(T(1) << (size_t(n) & (BitsPerItem - 1u)));
    }
}

void NumberSet::set(int n, bool value) {
    if (n < 0) { return; }
    constexpr size_t BitsPerItem = 8u * sizeof(T);
    size_t idx = size_t(n) / BitsPerItem;
    if (idx >= bits.size()) {
        if (!value) { return; }
        bits.resize(idx + 1u);
    }
    T bit = T(1) << (size_t(n) & (BitsPerItem - 1u));
    if (value) { bits[idx] |=  bit; }
    else       { bits[idx] &= ~bit; }
}

bool NumberSet::contains(int n) const {
    if (n < 0) { return false; }
    constexpr size_t BitsPerItem = 8u * sizeof(T);
    size_t idx = size_t(n) / BitsPerItem;
    if (idx >= bits.size()) { return false; }
    return !!(bits[idx] & (T(1) << (size_t(n) & (BitsPerItem - 1u))));
}

NumberSet NumberSet::union_(const NumberSet& other) const {
    size_t idx;
    NumberSet res;
    for (idx = 0;  (idx < bits.size()) && (idx < other.bits.size());  ++idx) {
        res.bits.push_back(bits[idx] | other.bits[idx]);
    }
    for (;  idx < bits.size();  ++idx) {
        res.bits.push_back(bits[idx]);
    }
    for (;  idx < other.bits.size();  ++idx) {
        res.bits.push_back(other.bits[idx]);
    }
    return res;
}

void NumberSet::update(const NumberSet& other) {
    size_t idx;
    for (idx = 0;  (idx < bits.size()) && (idx < other.bits.size());  ++idx) {
        bits[idx] |= other.bits[idx];
    }
    for (;  idx < other.bits.size();  ++idx) {
        bits.push_back(other.bits[idx]);
    }
}

NumberSet NumberSet::intersection(const NumberSet& other) const {
    NumberSet res;
    size_t idx;
    for (idx = 0;  (idx < bits.size()) && (idx < other.bits.size());  ++idx) {
        res.bits.push_back(bits[idx] & other.bits[idx]);
    }
    return res;
}

void NumberSet::intersectionUpdate(const NumberSet& other) {
    size_t idx;
    for (idx = 0;  (idx < bits.size()) && (idx < other.bits.size());  ++idx) {
        bits[idx] &= other.bits[idx];
    }
    bits.resize(idx);
}

NumberSet NumberSet::difference(const NumberSet& other) const {
    NumberSet res;
    size_t idx;
    for (idx = 0;  (idx < bits.size()) && (idx < other.bits.size());  ++idx) {
        res.bits.push_back(bits[idx] & ~other.bits[idx]);
    }
    for (;  idx < bits.size();  ++idx) {
        res.bits.push_back(bits[idx]);
    }
    return res;
}

void NumberSet::differenceUpdate(const NumberSet& other) {
    size_t idx;
    for (idx = 0;  (idx < bits.size()) && (idx < other.bits.size());  ++idx) {
        bits[idx] &= ~other.bits[idx];
    }
}

NumberSet NumberSet::symmetricDifference(const NumberSet& other) const {
    NumberSet res;
    size_t idx;
    for (idx = 0;  (idx < bits.size()) && (idx < other.bits.size());  ++idx) {
        res.bits.push_back(bits[idx] ^ other.bits[idx]);
    }
    for (;  idx < bits.size();  ++idx) {
        res.bits.push_back(bits[idx]);
    }
    for (;  idx < other.bits.size();  ++idx) {
        res.bits.push_back(other.bits[idx]);
    }
    return res;
}

void NumberSet::symmetricDifferenceUpdate(const NumberSet& other) {
    size_t idx;
    for (idx = 0;  (idx < bits.size()) && (idx < other.bits.size());  ++idx) {
        bits[idx] ^= other.bits[idx];
    }
    for (;  idx < other.bits.size();  ++idx) {
        bits.push_back(other.bits[idx]);
    }
}

int NumberSet::pop() {
    size_t idx;
    for (idx = 0;  (idx < bits.size()) && !bits[idx];  ++idx);
    if (idx >= bits.size()) { return -1; }
    int n = int(idx * (8u * sizeof(T)));
    T bit = 1;
    while (!(bits[idx] & bit)) { bit <<= 1; n++; }
    bits[idx] &= ~bit;
    return n;
}

int NumberSet::first() const {
    size_t idx;
    for (idx = 0;  (idx < bits.size()) && !bits[idx];  ++idx);
    if (idx >= bits.size()) { return -1; }
    int n = int(idx * (8u * sizeof(T)));
    T b = bits[idx];
    while (!(b & 1u)) { b >>= 1; ++n; }
    return n;
}

int NumberSet::last() const {
    size_t idx;
    for (idx = bits.size();  (idx > 0) && !bits[idx-1u];  --idx);
    if (idx < 1) { return -1; }
    int n = int(idx * (8u * sizeof(T))) - 1;
    T b = bits[idx - 1u];
    while (!(b >> (8u * sizeof(T) - 1u))) { b <<= 1; --n; }
    return n;
}

int NumberSet::next(int n) const {
    if (n < 0) { return -1; }
    constexpr size_t BitsPerItem = 8u * sizeof(T);
    size_t idx = size_t(n) /  BitsPerItem;
    size_t bit = size_t(n) & (BitsPerItem - 1u);
    while (idx < bits.size()) {
        if (bits[idx] & (T(1) << bit)) { return int(idx * BitsPerItem + bit); }
        if (++bit >= BitsPerItem) { ++idx; bit = 0; }
    }
    return -1;
}

bool NumberSet::empty() const {
    for (T v : bits) {
        if (v) { return false; }
    }
    return true;
}

int NumberSet::count() const {
    int res = 0;
    for (T v : bits) {
        while (v) { res += int(v & 1u); v >>= 1; }
    }
    return res;
}

bool NumberSet::isEqual(const NumberSet& other) const {
    size_t idx;
    for (idx = 0;  (idx < bits.size()) && (idx < other.bits.size());  ++idx) {
        if (bits[idx] != other.bits[idx]) { return false; }
    }
    for (;  idx < other.bits.size();  ++idx) {
        if (other.bits[idx]) { return false; }
    }
    for (;  idx < bits.size();  ++idx) {
        if (bits[idx]) { return false; }
    }
    return true;
}

bool NumberSet::isDisjoint(const NumberSet& other) const {
    size_t idx;
    for (idx = 0;  (idx < bits.size()) && (idx < other.bits.size());  ++idx) {
        if (bits[idx] & other.bits[idx]) { return false; }
    }
    return true;
}

bool NumberSet::isSuperset(const NumberSet& other) const {
    size_t idx;
    for (idx = 0;  (idx < bits.size()) && (idx < other.bits.size());  ++idx) {
        if (other.bits[idx] & ~bits[idx]) { return false; }
    }
    for (;  idx < other.bits.size();  ++idx) {
        if (other.bits[idx]) { return false; }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////

#ifdef NUMSET_TEST_PROGRAM

#include <cstdlib>

const int contentsA []= { 0, 8, 15, 47, 11, 33                  };
const int contentsB []= {                   33, 17, 23, 42, 234 };
const int refOR     []= { 0, 8, 15, 47, 11, 33, 17, 23, 42, 234 };
const int refAND    []= {                   33                  };
const int refAminB  []= { 0, 8, 15, 47, 11                      };
const int refBminA  []= {                       17, 23, 42, 234 };
const int refXOR    []= { 0, 8, 15, 47, 11,     17, 23, 42, 234 };

#define LOAD_SET(set, contents) \
        set.assign(contents, sizeof(contents) / sizeof(contents[0]))
#define LOAD_SETS() do { LOAD_SET(a, contentsA); LOAD_SET(b, contentsB); } while (0)

#define DUMP_BITS(name, set) do { \
            printf("%s", name); \
            for (NumberSet::T v : set.bits) { printf(" 0x%016lX", v); } \
            printf("\n"); \
        } while (0)

#define DUMP_SET(name, set) do { \
            printf("%s", name); \
            for (int v : set) { printf(" %d", v); } \
            printf("\n"); \
        } while (0)

#define ASSERT_EQUAL(name, got, expected) do { \
            printf("%s %d, expected %d => ", name, got, expected); \
            if (got != expected) { printf("MISMATCH\n"); exit(7); } \
            printf("OK\n"); \
        } while (0)

#define CHECK_REL(op, cond) do { if (cond) { printf(" %s", op); } } while (0)

#define CHECK_OP(name, a, b, op_copy, op_inplace, exdata) do { \
            LOAD_SET(e, exdata); DUMP_SET("\nexpecting:     ", e); \
            r = a.op_copy(b); \
            DUMP_SET(name " (copy):  ", r); if (r != e) { printf("MISMATCH\n"); exit(7); } \
            a.op_inplace(b); \
            DUMP_SET(name " (update):", a); if (a != e) { printf("MISMATCH\n"); exit(7); } \
            printf("relations to other:"); \
            CHECK_REL("==",  a == b); \
            CHECK_REL("!=",  a != b); \
            CHECK_REL("<=",  a <= b); \
            CHECK_REL(">=",  a >= b); \
            CHECK_REL("<",   a <  b); \
            CHECK_REL(">",   a >  b); \
            printf("\n"); \
        } while (0)

int main(void) {
    NumberSet a, b, r, e;
    ASSERT_EQUAL("empty set first():    ", a.first(), -1);
    ASSERT_EQUAL("empty set last():     ", a.last(),  -1);
    a.add(10);
    ASSERT_EQUAL("bits.size after add():", int(a.bits.size()), 1);
    ASSERT_EQUAL("bits[0] after add():  ", int(a.bits[0]), 1024);
    LOAD_SETS();
    DUMP_SET(    "set A:                ", a);
    DUMP_SET(    "set B:                ", b);
    DUMP_BITS(   "set A raw bits:       ", a);
    DUMP_BITS(   "set B raw bits:       ", b);
    ASSERT_EQUAL("set A first():        ", a.first(), 0);
    ASSERT_EQUAL("set A last():         ", a.last(),  47);
    ASSERT_EQUAL("set A count():        ", a.count(), 6);
    ASSERT_EQUAL("set B first():        ", b.first(), 17);
    ASSERT_EQUAL("set B last():         ", b.last(),  234);
    ASSERT_EQUAL("set B count():        ", b.count(), 5);
    LOAD_SETS(); CHECK_OP("A | B", a,b, union_, update, refOR);
    LOAD_SETS(); CHECK_OP("B | A", b,a, union_, update, refOR);
    LOAD_SETS(); CHECK_OP("A & B", a,b, intersection, intersectionUpdate, refAND);
    LOAD_SETS(); CHECK_OP("B & A", b,a, intersection, intersectionUpdate, refAND);
    LOAD_SETS(); CHECK_OP("A - B", a,b, difference, differenceUpdate, refAminB);
    LOAD_SETS(); CHECK_OP("B - A", b,a, difference, differenceUpdate, refBminA);
    LOAD_SETS(); CHECK_OP("A ^ B", a,b, symmetricDifference, symmetricDifferenceUpdate, refXOR);
    LOAD_SETS(); CHECK_OP("B ^ A", b,a, symmetricDifference, symmetricDifferenceUpdate, refXOR);

    printf("\npopping items off what's left from B:");
    for (;;) {
        int v = b.pop();
        if (v < 0) { break; }
        printf(" %d", v);
    }
    printf("\nB is now empty: %s\n", b.empty() ? "yes" : "no");
    return 0;
}

#endif // NUMSET_TEST_PROGRAM
