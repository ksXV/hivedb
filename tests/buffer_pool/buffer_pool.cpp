#include <algorithm>
#include <exception>
#include <catch_amalgamated.hpp>

#include <buffer_pool/buffer_pool.hpp>
#include <disk/disk_manager_mock.hpp>
#include <misc/config.hpp>


//TODO: write more tests
TEST_CASE("Buffer pool test", "[buffer_pool]") {
    constexpr auto max_frames = 4;
    hivedb::buffer_pool<hivedb::disk_manager_mock> bp{max_frames, ""};
    std::vector<hivedb::page_id_t> current_pages(5, 0);

    std::for_each(current_pages.begin(), current_pages.end() - 1,
    [&bp](hivedb::page_id_t& page_id) {
        page_id = bp.allocate_new_page();
        try {
            auto& frame = bp.request_page(page_id);
            if (page_id != 0) frame.decrease_pin_count();
        } catch (const std::exception& err) {
           FAIL(err.what());
        }
    });
    REQUIRE(current_pages.size() == 5);

    try {
        auto& page_0 = bp.request_page(0);
        REQUIRE(page_0.get_pin_count() == 2);

        page_0.decrease_pin_count();
        REQUIRE(page_0.get_pin_count() == 1);
    } catch (const std::exception& err) {
       FAIL(err.what());
    }

    //Our buffer pool now has the pages [0, 1, 2, 3] in the cache with page 0 being pinned by someone else
    //Time to request for another page
    try {
        auto& page_4 = bp.request_page(4);
        REQUIRE(page_4.get_pin_count() == 1);

        constexpr std::string_view dummy_data = "ilovejoe";
        std::memcpy(page_4.get_data(), dummy_data.data(), dummy_data.length());
        page_4.is_dirty = true;
        const auto status = bp.flush_page(4);

        REQUIRE(status);
    } catch (const std::exception& err) {
       FAIL(err.what());
    }

    //We request page 4 again and check if our content got written
    try {
        auto& page_4 = bp.request_page(4);
        REQUIRE(page_4.get_pin_count() == 1);
        std::string dummy_data{page_4.get_data()};
        REQUIRE(dummy_data == "ilovejoe");
        page_4.decrease_pin_count();
        REQUIRE(page_4.get_pin_count() == 0);
    } catch (const std::exception& err) {
        FAIL(err.what());
    }

    //We now have [2, 3, 0, 4] in our buffer pool.

    try {
        auto& page_2 = bp.request_page(2);
        REQUIRE(page_2.get_pin_count() == 1);
        constexpr std::string_view dummy_data = "ilovejoe";
        page_2.is_dirty = true;
        std::memcpy(page_2.get_data(), dummy_data.data(), dummy_data.length());
        page_2.decrease_pin_count();
        REQUIRE(page_2.get_pin_count() == 0);

        bp.request_page(0).decrease_pin_count();
        bp.request_page(3).decrease_pin_count();

        bp.request_page(0).decrease_pin_count();
        bp.request_page(3).decrease_pin_count();

        // The order should now be [2, 0, 3, 4]
        // We keep page 5 pinned so it doesn't kick it out
        bp.request_page(5).decrease_pin_count();

        // The bp should now have [5, 0, 3, 4]
        auto& page_2_again = bp.request_page(2);
        std::string page_2_content{page_2_again.get_data()};
        REQUIRE(dummy_data == page_2_content);
    } catch (const std::exception& err) {
        FAIL(err.what());
    }
}
