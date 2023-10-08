// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cstdint>
#include <atomic>

static_assert(std::atomic<bool>::is_always_lock_free);

class SpinLock {
    std::atomic<bool>& locked_flag;

public:
    SpinLock(std::atomic<bool>& locked_flag_) noexcept : locked_flag(locked_flag_) {
        bool cur_locked_value = false;
        do {
            cur_locked_value = false;
            if (locked_flag.compare_exchange_strong(cur_locked_value, true)) {
                break;
            }
        } while (cur_locked_value); // Spin (wait) in loop while locked_flag is still locked.
    }
    ~SpinLock() noexcept {
        locked_flag = false;
    }
    void unlock() noexcept {
        locked_flag = false;
    }
};

template <class ValueType>
class SpinLocked {
    mutable std::atomic<bool> locked_flag = false;
    ValueType value;

public:
    SpinLocked() noexcept {
        value = ValueType();
    }
    SpinLocked(const ValueType& init) noexcept {
        value = init;
    }
    ValueType read() const noexcept {
        SpinLock lock(locked_flag);
        return value;
    }
    void write(const ValueType& new_value) noexcept {
        SpinLock lock(locked_flag);
        value = new_value;
    }
};
