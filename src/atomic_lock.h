// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <atomic>

static_assert(std::atomic<bool>::is_always_lock_free);

struct AtomicAutoRelease {
    std::atomic<bool>& locked_flag;

    AtomicAutoRelease(std::atomic<bool>& locked_flag_) : locked_flag(locked_flag_) {}
    ~AtomicAutoRelease() {
        locked_flag = false;
    }
};

template <class Operation>
void doLocked(std::atomic<bool>& locked_flag, Operation&& operation) {
    bool locked = false;
    do {
        locked = false;
        if (locked_flag.compare_exchange_strong(locked, true)) {
            AtomicAutoRelease auto_release(locked_flag);
            operation();
        }
    } while (locked);
}
