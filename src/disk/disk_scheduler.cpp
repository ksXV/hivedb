//
// Created by ksxv on 7/23/25.
//

#include <disk/disk_scheduler.hpp>

#include "fmt/args.h"


namespace hivedb {
   disk_scheduler::disk_scheduler(): m_manager(0), m_requests_queue() {
      m_worker_thread = std::thread([this]() {
        while (auto req = m_requests_queue.get()) {
            switch (req.type) {
                case disk_request_type::read:
                    fmt::println("we read.");
                    break;
                case disk_request_type::write:
                    fmt::println("we wrote.");
                    break;
                case disk_request_type::shutdown:
                    return;
                default:
                    throw std::invalid_argument("invalid request type");
            }
        }
      });
   }

   void disk_scheduler::schedule(const disk_request& req) {
      m_requests_queue.put(req);
   }

    disk_scheduler::~disk_scheduler() {
       m_requests_queue.put(disk_request{disk_request_type::shutdown, nullptr, 0, {}});
       if (m_worker_thread.joinable()) {
           m_worker_thread.join();
       }
    }


}
