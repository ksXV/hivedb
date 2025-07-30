#include <exception>

#include <disk/disk_manager.hpp>
#include <misc/temporary_file_wrapper.hpp>
#include <catch_amalgamated.hpp>


TEST_CASE("Disk manager simple tests", "[disk_manager_simple]") {
    hivedb::temporary_file_wrapper fw;
    hivedb::disk_manager manager{fw.get_path()};

    constexpr std::string_view page_1_data = "ILOVEJOE!?!?!?!?!?!?!?!?!?!?!?!?!?!?!?!";
    std::array<char, hivedb::PAGE_SIZE> buffer{};

    try {
        manager.write_page(-1, nullptr);
    } catch (std::exception& err) {
        REQUIRE(std::string_view{err.what()} == "Invalid id detected!: -1");
    }

    try {
        manager.read_page(-1, &buffer[0]);
    } catch (std::exception& err) {
        REQUIRE(std::string_view{err.what()} == "Invalid id detected!: -1");
    }
    REQUIRE(buffer[0] == 0);

    std::memcpy(&buffer[0], page_1_data.data(), hivedb::PAGE_SIZE);
    manager.write_page(0, &buffer[0]);
    std::memset(&buffer[0], 0, hivedb::PAGE_SIZE);
    manager.read_page(0, &buffer[0]);
    REQUIRE(std::string_view{buffer.data()} == page_1_data);

    constexpr std::string_view page_2_data = "IDONTLOVEJOE!?!?!?!?!?!?!?!?!?!?!?!?!?!?!?!";
    std::memset(&buffer[0], 0, hivedb::PAGE_SIZE);
    std::memcpy(&buffer[0], page_2_data.data(), hivedb::PAGE_SIZE);
    manager.write_page(1, &buffer[0]);
    std::memset(&buffer[0], 0, hivedb::PAGE_SIZE);
    manager.read_page(1, &buffer[0]);
    REQUIRE(std::string_view{buffer.data()} == page_2_data);

    //rewrite page_id = 0
    constexpr std::string_view page_3_data = "ILOVEDENVER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    std::memset(&buffer[0], 0, hivedb::PAGE_SIZE);
    std::memcpy(&buffer[0], page_3_data.data(), hivedb::PAGE_SIZE);
    manager.write_page(0, &buffer[0]);
    std::memset(&buffer[0], 0, hivedb::PAGE_SIZE);
    manager.read_page(0, &buffer[0]);
    REQUIRE(std::string_view{buffer.data()} == page_3_data);
}
