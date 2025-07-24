//
// Created by ksxv on 7/22/25.
//
#pragma once
#include <condition_variable>
#include <queue>

#include <disk/disk_scheduler.hpp>

namespace hivedb {

struct channel {

private:
    mutable std::mutex m_latch;
    std::condition_variable m_cv;
    std::queue<disk_request> m_queue;

public:
    channel() = default;

    channel(const channel&) = delete;
    channel &operator=(const channel&) = delete;

    channel(channel&&) = delete;
    channel &operator=(channel&&) = delete;

    void put(const disk_request&);
    disk_request get();

    ~channel() = default;
};

} // hivedb
