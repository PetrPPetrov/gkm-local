// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cstdint>
#include <atomic>
#include "atomic_lock.h"

template <class T, std::uint32_t StateCount = 128>
class WaitFreeCompound {
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free);

    std::atomic<std::uint32_t> cur_state = 0;
    T data[StateCount];

public:
    WaitFreeCompound() noexcept {
        data[0] = T();
    }
    WaitFreeCompound(const T& init) noexcept {
        data[0] = init;
    }
    // Multi-threaded
    // Wait-free
    T read() const noexcept {
        return data[cur_state];
    }
    // Single-threaded
    // Wait-free
    void write(const T& value) noexcept {
        const std::uint32_t next = (cur_state + 1) % StateCount;
        data[next] = value;
        cur_state = next;
    }
};

template <class T>
class MutexFreeCompound {
    static_assert(std::atomic<bool>::is_always_lock_free);

    mutable std::atomic<bool> value_locked = false;
    T value;

public:
    MutexFreeCompound() noexcept {
        value = T();
    }
    MutexFreeCompound(const T& init) noexcept {
        value = init;
    }
    // Multi-threaded
    // Mutex-free
    T read() const noexcept {
        T result;
        doLocked(value_locked, [this, &result](){
            result = value;
        });
        return result;
    }
    // Multi-threaded
    // Mutex-free
    void write(const T& new_value) noexcept {
        doLocked(value_locked, [this, &new_value]() {
            value = new_value;
        });
    }
};
