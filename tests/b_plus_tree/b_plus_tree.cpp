#include <algorithm>
#include <catch_amalgamated.hpp>
#include <concepts>
#include <disk/disk_manager_mock.hpp>
#include <numeric>
#include <random>
#include <unordered_map>
#include <vector>

#include "b_plus_tree/b_plus_tree.hpp"
#include "misc/config.hpp"
#include "spdlog/spdlog.h"

struct int_key {
  std::int64_t key;
  std::int64_t _padding[31];

  int_key(std::int64_t x) : key(x) {};  // NOLINT

  [[nodiscard]]
  constexpr auto operator<=>(const int_key &rhs) const noexcept = default;

  [[nodiscard]]
  constexpr bool operator!=(const int_key &rhs) const {
    return key != rhs.key;
  }

  [[nodiscard]] std::string to_string() const { return std::to_string(key); }

  int_key &operator++() {
    this->key++;
    return *this;
  }

  static int_key invalid_key() { return {-99}; };

  static int_key *deserialize(char *buffer) {
    return reinterpret_cast<int_key *>(buffer);
  }
};

namespace std {
template <>
struct hash<int_key> {
  std::size_t operator()(const auto &s) const noexcept {
    std::hash<std::int64_t> temp;
    return temp(s.key);
  }
};
}  // namespace std

std::ostream &operator<<(std::ostream &os, const int_key &rhs) {
  os << rhs.key;
  return os;
}

template <>
struct fmt::formatter<int_key> : fmt::formatter<std::int64_t> {
  template <typename FormatContext>
  auto format(const int_key &k, FormatContext &ctx) const {
    return fmt::formatter<std::int64_t>::format(k.key, ctx);
  }
};

TEST_CASE("b_plus_tree trivial leaf test", "[b_plus_tree_leaf_trivial]") {
  hivedb::b_plus_tree<hivedb::disk_manager_mock, int_key, int_key, int_key>
      tree{hivedb::INVALID_PAGE_ID, 10, ""};

  try {
    std::random_device rnd_device;
    std::mt19937 mersenne_engine{rnd_device()};

    std::vector<int_key> vec(50, 0);
    std::iota(vec.begin(), vec.end(), int_key{0});
    std::shuffle(vec.begin(), vec.end(), mersenne_engine);

    std::vector<std::pair<int_key, int_key>> values;
    int idx = 0;
    for (const auto &element : vec) {
      auto temp = int_key{idx++};
      REQUIRE(tree.insert(element, temp));
      spdlog::info("TESTS: Emplacing {} and {} in values", element.key,
                   temp.key);
      values.emplace_back(element, temp);
    }

    for (const auto &element : vec) {
      int_key found_value{int_key::invalid_key()};
      tree.find(element, found_value);
      spdlog::info("TESTS: Trying to search for {} and {}", element.key,
                   found_value.key);
      REQUIRE(std::find(values.cbegin(), values.cend(),
                        std::pair{element, found_value}) != values.cend());
    }

    // Now remove
    // std::shuffle(vec.begin(), vec.end(), mersenne_engine);
    // for (const auto& element: vec) {
    //     REQUIRE(tree.remove(element));
    // }

  } catch (const std::exception &err) {
    FAIL(err.what());
  }
}

TEST_CASE("b_plus_tree create another root test", "[b_plus_tree_new_root]") {
  try {
    std::random_device rnd_device;
    std::mt19937 mersenne_engine{rnd_device()};

    std::vector<int_key> vec(300, 0);
    std::iota(vec.begin(), vec.end(), int_key{0});
    std::shuffle(vec.begin(), vec.end(), mersenne_engine);

    hivedb::b_plus_tree<hivedb::disk_manager_mock, int_key, int_key, int_key>
        tree{hivedb::INVALID_PAGE_ID, 10, ""};

    std::vector<std::pair<int_key, int_key>> values;
    int idx = 0;
    for (const auto &element : vec) {
      auto temp = int_key{idx++};
      REQUIRE(tree.insert(element, temp));
      spdlog::info("TESTS: Emplacing {} and {} in values", element.key,
                   temp.key);
      values.emplace_back(element, temp);
    }

    for (const auto &element : vec) {
      int_key found_value{int_key::invalid_key()};
      tree.find(element, found_value);
      spdlog::info("TESTS: Trying to search for {} and {}", element.key,
                   found_value.key);
      if (found_value == int_key::invalid_key()) {
        tree.dump_contents();
      }
      REQUIRE(std::find(values.cbegin(), values.cend(),
                        std::pair{element, found_value}) != values.cend());
    }

  } catch (const std::exception &err) {
    FAIL(err.what());
  }
}

TEST_CASE("b_plus_tree insert into inner node", "[b_plus_tree_insert_inner_node]") {
  try {
    std::random_device rnd_device;
    std::mt19937 mersenne_engine{rnd_device()};

    std::vector<int_key> vec(900, 0);
    std::iota(vec.begin(), vec.end(), int_key{0});
    std::shuffle(vec.begin(), vec.end(), mersenne_engine);

    hivedb::b_plus_tree<hivedb::disk_manager_mock, int_key, int_key, int_key>
        tree{hivedb::INVALID_PAGE_ID, 10, ""};

    std::vector<std::pair<int_key, int_key>> values;
    int idx = 0;
    for (const auto &element : vec) {
      auto temp = int_key{idx++};
      REQUIRE(tree.insert(element, temp));
      spdlog::info("TESTS: Emplacing {} and {} in values", element.key,
                   temp.key);
      values.emplace_back(element, temp);
    }

    for (const auto &element : vec) {
      int_key found_value{int_key::invalid_key()};
      tree.find(element, found_value);
      spdlog::info("TESTS: Trying to search for {} and {}", element.key,
                   found_value.key);
      if (found_value == int_key::invalid_key()) {
        tree.dump_contents();
      }
      REQUIRE(std::find(values.cbegin(), values.cend(),
                        std::pair{element, found_value}) != values.cend());
    }

  } catch (const std::exception &err) {
    FAIL(err.what());
  }
}

TEST_CASE("b_plus_tree split inner node", "[b_plus_tree_insert_and_split_inner_node]") {
  try {
    std::random_device rnd_device;
    std::mt19937 mersenne_engine{rnd_device()};

    std::vector<int_key> vec(64000, 0);
    std::iota(vec.begin(), vec.end(), int_key{0});
    std::shuffle(vec.begin(), vec.end(), mersenne_engine);

    hivedb::b_plus_tree<hivedb::disk_manager_mock, int_key, int_key, int_key>
        tree{hivedb::INVALID_PAGE_ID, 10, ""};

    std::unordered_map<int_key, int_key> values;
    int idx = 0;
    for (const auto &element : vec) {
      auto temp = int_key{idx++};
      REQUIRE(tree.insert(element, temp));
      values.emplace(element, temp);
    }

    for (const auto &element : vec) {
      int_key found_value{int_key::invalid_key()};
      tree.find(element, found_value);
      if (found_value == int_key::invalid_key()) {
        tree.dump_contents();
      }
      auto it = values.find(element);
      REQUIRE(it != values.end());
      REQUIRE(it->second == found_value);
    }

  } catch (const std::exception &err) {
    FAIL(err.what());
  }
}
