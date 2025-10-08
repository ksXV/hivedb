#include <catch_amalgamated.hpp>
#include <b_plus_tree/b_plus_tree.hpp>
#include <disk/disk_manager.hpp>
#include <exception>
#include <misc/config.hpp>
#include <random>

struct int_key {
    std::int64_t key;

    int_key(std::int64_t x): key(x) {}; // NOLINT

    [[nodiscard]]
    constexpr auto operator<=>(const int_key& rhs) const noexcept = default;

    [[nodiscard]]
    constexpr bool operator!=(const int_key& rhs) const {
        return key != rhs.key;
    }

    [[nodiscard]] std::string to_string() const {
        return std::to_string(key);
    }

    int_key& operator++() {
        this->key++;
        return *this;
    }

    static int_key invalid_key() {
       return {-99};
    };

    static int_key* deserialize(char* buffer) {
        return reinterpret_cast<int_key*>(buffer);
    }
};

namespace std {
    template<>
    struct hash<int_key>
    {
        std::size_t operator()(const auto& s) const noexcept
        {
            std::hash<std::int64_t> temp;
            return temp(s.key);
        }
    };
}

std::ostream& operator<<(std::ostream& os, const int_key& rhs) {
    os << rhs.key;
    return os;
}

TEST_CASE("b_plus_tree trivial leaf test", "[b_plus_tree]") {
    try {
    std::random_device rnd_device;
    std::mt19937 mersenne_engine {rnd_device()};

    std::vector<int_key> vec(50, 0);
    std::iota(vec.begin(), vec.end(), int_key{0});
    std::shuffle(vec.begin(), vec.end(), mersenne_engine);

    hivedb::b_plus_tree<hivedb::disk_manager_mock, int_key, int_key, int_key> tree{
        hivedb::INVALID_PAGE_ID,
        10,
        ""
    };

    std::vector<std::pair<int_key, int_key>> values;
    int idx = 0;
    for (const auto& element: vec) {
        auto temp = int_key{idx++};
        REQUIRE(tree.insert(element, temp));
        spdlog::info(fmt::format("TESTS: Emplacing {} and {} in values", element.key, temp.key));
        values.emplace_back(element, temp);
    }

    for (const auto& element: vec) {
        int_key found_value{int_key::invalid_key()};
        tree.find(element, found_value);
        spdlog::info(fmt::format("TESTS: Trying to search for {} and {}", element.key, found_value.key));
        REQUIRE(std::find(values.cbegin(), values.cend(), std::pair{element, found_value}) != values.cend());
    }

    } catch (const std::exception& err) {
        FAIL(err.what());
    }
}

TEST_CASE("b_plus_tree create another root test", "[b_plus_tree_new_root]") {
    try {
    std::random_device rnd_device;
    std::mt19937 mersenne_engine {rnd_device()};

    std::vector<int_key> vec(300, 0);
    std::iota(vec.begin(), vec.end(), int_key{0});
    std::shuffle(vec.begin(), vec.end(), mersenne_engine);

// std::vector<int_key> vec{207,257,171,1,69,184,179,181,94,12,299,266,68,116,151,26,87,164,196,202,106,72,285,91,0,44,98,112,117,279,108,66,9,56,260,242,29,201,19,95,237,20,231,67,278,64,194,152,264,215,82,110,267,160,10,23,13,115,263,292,50,4,88,182,80,177,174,243,58,273,262,43,223,122,38,148,281,71,62,248,144,213,83,75,214,297,79,24,283,238,40,146,234,192,21,143,218,265,251,57,255,210,53,93,268,175,8,149,290,81,209,140,167,96,247,86,65,90,133,137,49,55,32,232,60,70,54,277,27,156,150,162,16,211,34,105,100,286,190,195,48,92,228,161,46,145,229,212,11,17,221,128,291,230,78,259,239,15,220,51,141,233,138,244,295,28,294,271,14,282,103,254,153,204,135,126,165,99,97,73,63,47,41,22,224,61,280,186,36,200,163,35,183,168,33,272,222,121,89,31,118,39,123,170,252,172,18,187,180,142,216,127,37,246,2,74,275,173,104,155,169,102,120,136,241,240,225,276,205,288,139,298,158,208,113,166,206,154,7,269,197,274,227,124,189,107,284,6,193,236,235,199,191,30,114,119,261,198,226,270,203,159,5,287,217,125,293,76,249,176,130,59,258,84,185,250,289,157,296,256,45,109,147,131,77,25,219,253,101,42,134,111,129,132,85,178,3,188,245,52};

        // std::string vector_dump;

    hivedb::b_plus_tree<hivedb::disk_manager_mock, int_key, int_key, int_key> tree{
        hivedb::INVALID_PAGE_ID,
        10,
        ""
    };

    std::vector<std::pair<int_key, int_key>> values;
    int idx = 0;
    for (const auto& element: vec) {
        auto temp = int_key{idx++};
        REQUIRE(tree.insert(element, temp));
        spdlog::info("TESTS: Emplacing {} and {} in values", element.key, temp.key);
        values.emplace_back(element, temp);
    }

    for (const auto& element: vec) {
        int_key found_value{int_key::invalid_key()};
        tree.find(element, found_value);

        spdlog::info("TESTS: Trying to search for {} and {}", element.key, found_value.key);
        REQUIRE(std::find(values.cbegin(), values.cend(), std::pair{element, found_value}) != values.cend());
    }

    } catch (const std::exception& err) {
        FAIL(err.what());
    }
}
