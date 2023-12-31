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
    std::uint32_t size = 0;
    RequestType requests[BufferSize];

public:
    void waitForNewRequests(RequestType& request) noexcept {
        std::unique_lock wait_lock(wait_mutex);
        while (!pop(request)) {
            new_requests.wait(wait_lock);
        }
    }
    bool pop(RequestType& request) noexcept {
        SpinLock lock(locked_flag);
        if (size == 0) {
            return false;
        }
        request = requests[head_index];
        head_index = static_cast<std::uint32_t>((head_index + 1) % BufferSize);
        --size;
        return true;
    }
    void push(const RequestType& new_request) noexcept {
        SpinLock lock(locked_flag);
        assert(size < BufferSize);
        requests[tail_index] = new_request;
        tail_index = (tail_index + 1) % BufferSize;
        ++size;
        lock.unlock();
        new_requests.notify_one();
    }
};
