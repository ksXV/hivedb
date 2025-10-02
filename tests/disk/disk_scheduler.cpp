#include "catch_amalgamated.hpp"

#include <misc/config.hpp>
#include <disk/disk_scheduler.hpp>
#include <disk/disk_manager_mock.hpp>
#include <thread>


TEST_CASE("Disk scheduler simple test", "[disk_scheduler]") {
    using namespace std::chrono_literals;

    hivedb::disk_scheduler<hivedb::disk_manager_mock> scheduler{""};

    constexpr std::string_view data = "foobar";
    std::array<char, hivedb::PAGE_SIZE> buffer{};

    std::memcpy(&buffer[0], data.data(), data.size());

    std::promise<bool> is_done_write_promise{};
    auto is_done = is_done_write_promise.get_future();

    hivedb::disk_request req{
        .type = hivedb::disk_request_type::write,
        .data = buffer.data(),
        .page_id = 0,
        .is_done = std::move(is_done_write_promise)
    };

    scheduler.schedule(std::move(req));
    std::this_thread::sleep_for(1s);
    REQUIRE(is_done.get());

    buffer.fill(0);

    std::promise<bool> is_done_read_promise{};
    is_done = is_done_read_promise.get_future();

    req = hivedb::disk_request{
        .type = hivedb::disk_request_type::read,
        .data = buffer.data(),
        .page_id = 0,
        .is_done = std::move(is_done_read_promise)
    };

    scheduler.schedule(std::move(req));
    std::this_thread::sleep_for(1s);
    REQUIRE(is_done.get());

    REQUIRE(data == std::string_view{buffer.data()});
}
