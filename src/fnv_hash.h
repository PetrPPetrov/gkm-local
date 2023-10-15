// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cstddef>
#include <cstdint>

// Fowler / Noll / Vo (FNV) Hash.
class FnvHash {
public:
    typedef std::uint64_t Hash;

    void update(const std::uint8_t* data, std::size_t size) {
        for (std::size_t i = 0; i < size; ++i) {
            hash = hash ^ (data[i]); // Xor the low 8 bits.
            hash = hash * g_fnv_multiple; // Multiply by the magic number.
        }
    }
    template <class T>
    void update(const T& value) {
        update(reinterpret_cast<const std::uint8_t*>(&value), sizeof(T));
    }
    std::uint64_t getHash() const {
        return hash;
    }
private:
    // Magic numbers are from http://www.isthe.com/chongo/tech/comp/fnv/
    static const Hash g_initial_fnv = 2166136261U;
    static const Hash g_fnv_multiple = 16777619;
    Hash hash = g_initial_fnv;
};
