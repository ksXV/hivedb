#include <catch_amalgamated.hpp>
#include <b_plus_tree/b_plus_tree.hpp>
#include <disk/disk_manager.hpp>
#include <exception>
#include <misc/config.hpp>
#include <random>

struct int_key {
    int key;

    int_key(int x): key(x) {};

    [[nodiscard]]
    constexpr auto operator<=>(const int_key& rhs) const noexcept = default;

    [[nodiscard]]
    constexpr bool operator!=(const int_key& rhs) const {
        return key != rhs.key;
    }

    static int_key invalid_key() {
       return {-99};
    };

    static int_key* deserialize(char* buffer) {
        return reinterpret_cast<int_key*>(buffer);
    }
};

std::ostream& operator<<(std::ostream& os, const int_key& rhs) {
    os << rhs.key;
    return os;
}

TEST_CASE("b_plus_tree test", "[b_plus_tree]") {
    try {
    std::random_device rnd_device;
    std::mt19937 mersenne_engine {rnd_device()};  // Generates random integers
    std::uniform_int_distribution<int> dist {1, 100};

    static auto gen = [&mersenne_engine, &dist]() -> int_key {
        return int_key{dist(mersenne_engine)};
    };

    std::vector<int_key> vec(50, 0);
    std::generate(vec.begin(), vec.end(), gen);
    std::shuffle(vec.begin(), vec.end(), mersenne_engine);

    hivedb::b_plus_tree<hivedb::disk_manager_mock, int_key, int_key, int_key> tree{
        hivedb::INVALID_PAGE_ID,
        10,
        ""
    };

    std::vector<int_key> values;
    values.reserve(50);
    int idx = 0;
    for (const auto& element: vec) {
        auto temp = int_key{idx++};
        REQUIRE(tree.insert(element, temp));
        values.push_back(temp);
    }

    for (const auto& element: vec) {
        int_key found_value{int_key::invalid_key()};
        tree.find(element, found_value);
        REQUIRE(std::find(values.cbegin(), values.cend(), found_value) != values.cend());
    }

    } catch (const std::exception& err) {
        FAIL(err.what());
    }
}
