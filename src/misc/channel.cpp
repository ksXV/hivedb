//
// Created by ksxv on 7/22/25.
//
#include <misc/channel.hpp>

namespace hivedb {
    void channel::put(const disk_request& req) {
        std::unique_lock ul{m_latch};
        m_queue.emplace(req.type, req.data, req.page_id, req.is_done);
        ul.unlock();
        m_cv.notify_all();
    }

    disk_request channel::get() {
        std::unique_lock ul{m_latch};
        m_cv.wait(ul, [&] {return !m_queue.empty();});
        auto req = std::move(m_queue.front());
        m_queue.pop();

        return req;
    }

}