//
// Created by ksxv on 7/22/25.
//
#pragma once
#include <condition_variable>
#include <queue>

namespace hivedb {

template<typename T>
struct channel {

private:
    mutable std::mutex m_latch;
    std::condition_variable m_cv;
    std::queue<T> m_queue;

public:
    channel() = default;

    channel(const channel&) = delete;
    channel &operator=(const channel&) = delete;

    channel(channel&&) = delete;
    channel &operator=(channel&&) = delete;

    void put(const T& req) {
        std::unique_lock ul{m_latch};
        m_queue.emplace(req.type, req.data, req.page_id, req.is_done);
        ul.unlock();
        m_cv.notify_all();
    }

    T get() {
        std::unique_lock ul{m_latch};
        m_cv.wait(ul, [&] {return !m_queue.empty();});
        auto req = std::move(m_queue.front());
        m_queue.pop();

        return req;
    }

    ~channel() = default;
};

} // hivedb
