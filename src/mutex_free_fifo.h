// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cassert>
#include <cstdint>
#include <atomic>
#include "atomic_lock.h"

template <class T, std::uint32_t BufferSize = 1024>
class MutexFreeFifo {
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);

    std::atomic<std::uint32_t> head_index = 0;
    std::atomic<std::uint32_t> tail_index = 0;
    std::atomic<bool> push_locked = false;
    T data[BufferSize];

public:
    // Single-threaded (thread1)
    // Wait-free
    bool peek(T& value) noexcept {
        const std::uint32_t cur_head_index = head_index;
        if (cur_head_index == tail_index) {
            return false;
        }
        value = data[cur_head_index];
        return true;
    }
    // Single-threaded (thread1)
    // Wait-free
    void pop() noexcept {
        const std::uint32_t cur_head_index = head_index;
        if (cur_head_index == tail_index) {
            return;
        }
        data[cur_head_index] = T();
        head_index = static_cast<std::uint32_t>((cur_head_index + 1) % BufferSize);
    }
    // Multi-threaded
    // Mutex-free
    void push(const T& value) noexcept {
        doLocked(push_locked, [this, &value](){
            const std::uint32_t cur_tail_index = tail_index;
            const std::uint32_t new_tail_index = (cur_tail_index + 1) % BufferSize;
            if (new_tail_index != head_index) {
                data[cur_tail_index] = value;
                tail_index = new_tail_index;
            }
        });
    }
};
