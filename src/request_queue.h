// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

#pragma once

#include <cassert>
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include "spin_lock.h"

template <class RequestType, std::uint32_t BufferSize = 1024>
class RequestQueue {
    mutable std::mutex wait_mutex;
    mutable std::condition_variable new_requests;
    mutable std::atomic<bool> locked_flag = false;
    std::uint32_t head_index = 0;
    std::uint32_t tail_index = 0;
    RequestType requests[BufferSize];

public:
    void waitForNewRequests(RequestType& request) const noexcept {
        std::unique_lock wait_lock(wait_mutex);
        while (!peek(request)) {
            new_requests.wait(wait_lock);
        }
    }
    bool peek(RequestType& request) const noexcept {
        SpinLock lock(locked_flag);
        if (head_index == tail_index) {
            return false;
        }
        request = requests[head_index];
        return true;
    }
    void pop() noexcept {
        SpinLock lock(locked_flag);
        if (head_index == tail_index) {
            return;
        }
        head_index = static_cast<std::uint32_t>((head_index + 1) % BufferSize);
    }
    void push(const RequestType& new_request) noexcept {
        SpinLock lock(locked_flag);
        const std::uint32_t cur_tail_index = tail_index;
        const std::uint32_t new_tail_index = (cur_tail_index + 1) % BufferSize;
        if (new_tail_index != head_index) {
            requests[cur_tail_index] = new_request;
            tail_index = new_tail_index;
        }
        else {
            assert(false);
        }
        lock.unlock();
        new_requests.notify_one();
    }
};
