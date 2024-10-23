// SPDX-FileCopyrightText: 2024 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <cstdint>

#include <vector>

//! set of small non-negative integer numbers, with a Python-like API
// (in terms of how the data is stored, this is practically identical to
// the STL's specialization of std::vector<bool>, but it offers the efficient
// set operations on top of that)
struct NumberSet {
    using T = uintmax_t;  //!< bitfield base type
    std::vector<T> bits;  //!< bitfield data

    //! construct an empty set
    inline NumberSet() {}
    //! construct a set from a list of numbers
    inline NumberSet(const int *n, int count) { assign(n, count); }

    //! clear the set (initialize with empty set)
    inline void clear() { bits.clear(); }

    //! add a single number to the set
    void add(int n);
    //! remove a single number from the set
    void remove(int n);
    //! set a single number in the set to be either included (true) or excluded (false)
    void set(int n, bool value);
    //! report whether the set contains a specific number
    bool contains(int n) const;

    //! initialize the set with a list of numbers
    inline void assign(const int *n, int count) {
        bits.clear();
        while (n && (count > 0)) { add(*n++);  --count; }
    }
    //! add multiple numbers to the set
    inline void add(const int *n, int count) {
        while (n && (count > 0)) { add(*n++);  --count; }
    }
    //! remove multiple numbers to the set
    inline void remove(const int *n, int count) {
        while (n && (count > 0)) { remove(*n++);  --count; }
    }

    //! return the union of two sets as a new set
    NumberSet union_(const NumberSet& other) const;
    //! update the set with the union of itself and another
    void update(const NumberSet& other);

    //! return the intersection of two sets as a new set
    NumberSet intersection(const NumberSet& other) const;
    //! update the set with the intersection of itself and another
    void intersectionUpdate(const NumberSet& other);

    //! return the difference of two sets as a new set
    NumberSet difference(const NumberSet& other) const;
    //! remove all elements from another set from this set
    void differenceUpdate(const NumberSet& other);

    //! return the symmetric difference (logical XOR) of two sets as a new set,
    //! i.e. elements that occur in exactly one set
    NumberSet symmetricDifference(const NumberSet& other) const;
    //! replace the set by the symmetric difference with another set
    void symmetricDifferenceUpdate(const NumberSet& other);

    //! report whether two sets are identical
    bool isEqual(const NumberSet& other) const;
    //! report whether two sets have a empty intersection
    bool isDisjoint(const NumberSet& other) const;
    //! report whether this set contains another set
    bool isSuperset(const NumberSet& other) const;
    //! report whether another set contains this set
    inline bool isSubset(const NumberSet& other) const { return other.isSuperset(*this); }

    //! check if the set is empty
    bool empty() const;
    //! count the number of items in the set
    int count() const;

    //! remove and return the lowest number from the set; return -1 if the set is empty
    int pop();

    //! return the first (lowest) number in the set (or -1 if the set is empty)
    int first() const;
    //! return the last (highest) number in the set (or -1 if the set is empty)
    int last() const;
    //! return the next higher number after 'n' in the set (or n itself,
    //! if it's part of the set, or -1 if there are no higher numbers)
    int next(int n) const;

    // iterator protocol
    struct iterator {
        const NumberSet& parent;
        int n;
        inline iterator(const NumberSet& parent_, int n_) : parent(parent_), n(n_) {}
        inline iterator operator++() { n = parent.next(n + 1); return *this; }
        inline bool operator!= (const iterator& other) const { return n != other.n; }
        int operator*() const { return n; }
    };
    inline iterator begin() const { return iterator(*this, first()); }
    inline iterator end()   const { return iterator(*this, -1); }

    // operator syntactic sugar
    inline bool       operator[] (int n)                  const { return contains(n); }
    inline NumberSet  operator+  (const NumberSet& other) const { return union_(other); }
    inline NumberSet& operator+= (const NumberSet& other)       { update(other); return *this; }
    inline NumberSet  operator|  (const NumberSet& other) const { return union_(other); }
    inline NumberSet& operator|= (const NumberSet& other)       { update(other); return *this; }
    inline NumberSet  operator&  (const NumberSet& other) const { return intersection(other); }
    inline NumberSet& operator&= (const NumberSet& other)       { intersectionUpdate(other); return *this; }
    inline NumberSet  operator-  (const NumberSet& other) const { return difference(other); }
    inline NumberSet& operator-= (const NumberSet& other)       { differenceUpdate(other); return *this; }
    inline NumberSet  operator^  (const NumberSet& other) const { return symmetricDifference(other); }
    inline NumberSet& operator^= (const NumberSet& other)       { symmetricDifferenceUpdate(other); return *this; }
    inline bool       operator== (const NumberSet& other) const { return isEqual(other); }
    inline bool       operator!= (const NumberSet& other) const { return !isEqual(other); }
    inline bool       operator>= (const NumberSet& other) const { return isSuperset(other); }
    inline bool       operator<= (const NumberSet& other) const { return isSubset(other); }
    inline bool       operator>  (const NumberSet& other) const { return isSuperset(other) && !isEqual(other); }
    inline bool       operator<  (const NumberSet& other) const { return isSubset(other)   && !isEqual(other); }
};
