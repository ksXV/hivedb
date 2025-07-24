//
// Created by Pam Bondi herself on 7/22/25.
//
#pragma once
#include <future>
#include <thread>

#include <misc/channel.hpp>

namespace hivedb {
    using page_id_t = std::int64_t;
    enum struct disk_request_type {
        write,
        read,

        //shutdown the thread
        shutdown,
    };

    struct disk_request {
        disk_request_type type;
        std::byte* data;
        page_id_t page_id;
        std::promise<bool> is_done;
    };

    struct disk_scheduler {
    private:
     /* disk_manager */ int m_manager;
       std::thread m_worker_thread;
       channel m_requests_queue;


    public:
        disk_scheduler();

        void schedule(const disk_request&);

        disk_scheduler(const disk_scheduler &) = delete;
        disk_scheduler& operator=(const disk_scheduler &) = delete;
        disk_scheduler(const disk_scheduler &&) = delete;
        disk_scheduler& operator=(const disk_scheduler &&) = delete;

        ~disk_scheduler();
   };
}
