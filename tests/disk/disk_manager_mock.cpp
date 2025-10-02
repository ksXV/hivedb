#include <catch_amalgamated.hpp>
#include <array>
#include <string_view>

#include <misc/config.hpp>
#include <disk/disk_manager_mock.hpp>


TEST_CASE("Disk manager mock", "[disk_manager_mock]") {
    constexpr std::string_view page_1_data = "ILOVEJOE!?!?!?!?!?!?!?!?!?!?!?!?!?!?!?!";
    std::array<char, hivedb::PAGE_SIZE> buffer{};

    hivedb::disk_manager_mock manager{""};

    manager.write_page(-1, nullptr);
    manager.read_page(-1, &buffer[0]);

    REQUIRE(buffer[0] == 0);
    std::memcpy(&buffer[0], page_1_data.data(), page_1_data.size());

    manager.write_page(0, &buffer[0]);

    std::memset(&buffer[0], 0, hivedb::PAGE_SIZE);

    manager.read_page(0, &buffer[0]);

    REQUIRE(std::string_view{buffer.data()} == page_1_data);

    constexpr std::string_view page_2_data = "IDONTLOVEJOE!?!?!?!?!?!?!?!?!?!?!?!?!?!?!?!";
    std::memset(&buffer[0], 0, hivedb::PAGE_SIZE);
    std::memcpy(&buffer[0], page_2_data.data(), page_2_data.size());

    manager.write_page(1, &buffer[0]);
    std::memset(&buffer[0], 0, hivedb::PAGE_SIZE);

    manager.read_page(1, &buffer[0]);
    REQUIRE(std::string_view{buffer.data()} == page_2_data);

    //now we rewrite page 1
    constexpr std::string_view page_3_data = "ILOVEDENVER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    std::memset(&buffer[0], 0, hivedb::PAGE_SIZE);
    std::memcpy(&buffer[0], page_3_data.data(), page_3_data.size());

    manager.write_page(0, &buffer[0]);
    std::memset(&buffer[0], 0, hivedb::PAGE_SIZE);

    manager.read_page(0, &buffer[0]);
    REQUIRE(std::string_view{buffer.data()} == page_3_data);
}
