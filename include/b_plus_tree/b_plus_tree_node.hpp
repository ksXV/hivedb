#pragma once

#include <algorithm>
#include <buffer_pool/buffer_pool.hpp>
#include <cassert>
#include <concepts>
#include <disk/disk_manager.hpp>
#include <disk/disk_manager_mock.hpp>
#include <libassert/assert.hpp>
#include <misc/config.hpp>
#include <optional>
#include <sstream>
#include <type_traits>

namespace hivedb {
template <typename T>
concept index_t = requires(T a, T b, char* buffer) {
  { a < b } -> std::same_as<bool>;
  { a != b } -> std::same_as<bool>;
  { T::deserialize(buffer) } -> std::same_as<T*>;
  { T::invalid_key() } -> std::same_as<T>;
} && std::is_trivially_copy_assignable_v<T>;

template <typename T>
concept value_t = requires(T a, char* buffer) {
  { T::deserialize(buffer) } -> std::same_as<T*>;
} && std::is_trivially_copy_assignable_v<T>;
  //TODO: remove int64_t at some point
enum struct b_plus_tree_node_type : std::int64_t { leaf_node, inner_node };

// The structure of a b+tree node is:
// @ 0-8 bytes -> type of b+plus tree node:
// @ - 0 means its a child/leaf node
// @ - 1 means its a inner node
//
// @ 8-16 bytes -> the current size of the node
// @ 16-24 bytes -> the max size of key-value pairs a node can hold
struct b_plus_tree_node {

  explicit b_plus_tree_node(char* buffer)
      : type(),
        max_size(),
        current_size(),
        internal_data(buffer + 3 * sizeof(b_plus_tree_node_type)) {
    std::memcpy(&type, buffer, sizeof(b_plus_tree_node_type));
    std::memcpy(&max_size, buffer + sizeof(b_plus_tree_node_type),
                sizeof(b_plus_tree_node_type));
    std::memcpy(&current_size, buffer + 2 * sizeof(b_plus_tree_node_type),
                sizeof(b_plus_tree_node_type));
  }
  [[nodiscard]]
  constexpr bool can_insert_trivially() const {
    return current_size < max_size;
  }

  [[nodiscard]]
  constexpr bool should_split() const {
    return current_size == max_size;
  }

  b_plus_tree_node_type type;
  std::uint64_t max_size;
  std::uint64_t current_size;
  char* internal_data;

};
/*
 * Leaf page format (in order):
 *  ---------
 * | header |
 *  ---------
 * -----------------------
 * | key_1 | ... | key_n |
 * -----------------------
 * -----------------------
 * | rid_1 | ... | rid_n |
 * -----------------------
 *
 * Header format (32 bytes in total):
 * ---------------------------------------------------------------------
 * | page_type (8 bytes) | current_size (8 bytes) | max_size (8 bytes) |
 * ---------------------------------------------------------------------
 * --------------------------
 * | next_page_id (8 bytes) |
 * --------------------------
 */

template <index_t K, value_t V>
struct b_plus_tree_leaf_node final : public b_plus_tree_node {
  static constexpr auto MAX_NUMBER_OF_ELEMENTS =
      ((hivedb::PAGE_SIZE - (sizeof(std::int64_t) * 4)) /
       (sizeof(K) + sizeof(V)));

  static constexpr auto something_to_be_renamed = (MAX_NUMBER_OF_ELEMENTS + 1) % 2;
  explicit b_plus_tree_leaf_node(char* buffer)  // NOLINT
      : b_plus_tree_node(buffer), next_page_id() {
    std::memcpy(&next_page_id, internal_data, sizeof(next_page_id));
    internal_data += sizeof(next_page_id);
  }

  void dump_contents() const {
      std::stringstream ss;
      ss << "\nleaf_node: [";
      ss <<"max_size: " << max_size << " ";
      ss <<"current_size: " << current_size << " ";
      ss <<"values: ";
      for (auto i = 0u; i < current_size; ++i) {
          ss << indexes(i)->to_string() << " ";
      }
      ss << "]\n";

      spdlog::info("{}", ss.str());
  }

  void update_buffer_with_new_values() {
    std::memcpy(internal_data - sizeof(page_id_t), &next_page_id,
                sizeof(page_id_t));
    std::memcpy(internal_data - 2 * sizeof(page_id_t), &current_size,
                sizeof(current_size));
    std::memcpy(internal_data - 3 * sizeof(page_id_t), &max_size,
                sizeof(max_size));
    std::memcpy(internal_data - 4 * sizeof(page_id_t), &type, sizeof(type));
  }

  void init_first_node(const K& key, const V& value) {
    max_size = MAX_NUMBER_OF_ELEMENTS;
    current_size = 1;
    next_page_id = INVALID_PAGE_ID;
    type = b_plus_tree_node_type::leaf_node;
    *indexes(0) = key;
    *records(0) = value;
    update_buffer_with_new_values();
  }

  std::uint64_t find_index_for_insert(const K& key) const {
    ASSERT(can_insert_trivially());
    if (current_size < 5) {
      std::uint64_t idx = 0;
      while (idx != current_size) {
        if ((*indexes(idx)) > key) break;
        idx++;
      }

      return idx;
    }

    std::int64_t high = current_size - 1;
    std::int64_t low = 0;

    if (key > *indexes(high)) {
      return current_size;
    }

    ASSERT(low < high);
    while (high > low) {
      std::int64_t middle = (high + low) / 2;
      if (*indexes(middle) >= key) {
        high = middle;
      } else {
        low = middle + 1;
      }
    }

    ASSERT(low >= 0);
    return static_cast<std::uint64_t>(low);
  }

  std::optional<std::uint64_t> find_index(const K& key) const {
    ASSERT(current_size > 0);
    std::int64_t high = current_size - 1;
    std::int64_t low = 0;
    ASSERT(high >= low);
    std::int64_t middle = (high + low) / 2;

    while (high > low) {
      if (*indexes(middle) == key) {
        break;
      }
      if (*indexes(middle) > key) {
        high = middle - 1;
      } else {
        low = middle + 1;
      }
      middle = (high + low) / 2;
    }

    if (*indexes(middle) != key) return std::nullopt;

    ASSERT(middle >= 0);
    return static_cast<uint64_t>(middle);
  }

  void trivial_insert(const K& key, const V& value,
                      std::optional<std::uint64_t> where_to = std::nullopt) {
    ASSERT(can_insert_trivially());

    const auto where_to_insert =
        where_to.has_value() ? where_to.value() : find_index_for_insert(key);
    ASSERT(where_to_insert <= current_size);

    if (where_to_insert == current_size) {
      *indexes(where_to_insert) = key;
      *records(where_to_insert) = value;
      current_size++;

      update_buffer_with_new_values();
      return;
    }

    for (auto i = current_size; i > where_to_insert; --i) {
      *indexes(i) = *indexes(i - 1);
      *records(i) = *records(i - 1);
    }

    *indexes(where_to_insert) = key;
    *records(where_to_insert) = value;
    current_size++;

    update_buffer_with_new_values();
  }

  void split_node(b_plus_tree_leaf_node& new_node, page_id_t new_page_id) {
    spdlog::info("Splitting node with page_id {}", new_page_id);
    ASSERT(should_split());

    new_node.type = b_plus_tree_node_type::leaf_node;
    new_node.max_size = current_size;
    std::copy(indexes(max_size/2), indexes(max_size-1), new_node.indexes(0));
    std::copy(records(max_size/2), records(max_size-1), new_node.records(0));

    //15 26 35 45
    *new_node.indexes((max_size/2) - something_to_be_renamed) = *indexes(max_size-1);
    *new_node.records((max_size/2) - something_to_be_renamed) = *records(max_size-1);

    std::for_each(indexes(max_size/2), indexes(max_size-1), [] (K& key) {key = K::invalid_key();});
    std::for_each(records(max_size/2), records(max_size-1), [] (V& val) {val = V::invalid_key();});

    current_size = current_size / 2;
    new_node.current_size = current_size + ((something_to_be_renamed+1) % 2);
    next_page_id = new_page_id;

    update_buffer_with_new_values();
    new_node.update_buffer_with_new_values();
  }

  page_id_t next_page_id;
  K* indexes(std::uint64_t index) const {
    ASSERT(index < max_size);
    return K::deserialize(internal_data + index * sizeof(K));
  }

  V* records(std::uint64_t index) const {
    ASSERT(index < max_size);

    constexpr auto offset = MAX_NUMBER_OF_ELEMENTS * sizeof(K);
    return V::deserialize(internal_data + offset + index * sizeof(V));
  }
};

/* Internal page format (in increasing order):
 * ----------
 * | header |
 * ----------
 * --------------------------------------------
 * | key_1 | ... | key_n | (invalid or "nil") |
 * --------------------------------------------
 * -------------------------------
 * | page_id_1 | ... | page_id_n |
 * -------------------------------
 *
 * Header format (32 bytes in total):
 * ---------------------------------------------------------------------
 * | page_type (8 bytes) | current_size (8 bytes) | max_size (8 bytes) |
 * ---------------------------------------------------------------------
 * ------------------------------
 * | previous_page_id (8 bytes) |
 * ------------------------------
 */
template <index_t K, value_t V>
struct b_plus_tree_inner_node final : public b_plus_tree_node {
  static constexpr auto MAX_NUMBER_OF_ELEMENTS =
      ((hivedb::PAGE_SIZE - (sizeof(std::int64_t) * 4)) /
       (sizeof(K) + sizeof(V)));
  static constexpr auto something_to_be_renamed = MAX_NUMBER_OF_ELEMENTS % 2 ? 1 : 2;

  explicit b_plus_tree_inner_node(char* buffer)  // NOLINT
      : b_plus_tree_node(buffer), previous_page_id() {
    std::memcpy(&previous_page_id, internal_data, sizeof(previous_page_id));
    internal_data += sizeof(previous_page_id);
  }

  void dump_contents() const {
      std::stringstream ss;
      ss << "\ninner_node: [";
      ss <<"max_size: " << max_size << " ";
      ss <<"current_size: " << current_size << " ";
      ss <<"values: ";
      for (auto i = 0u; i < current_size; ++i) {
        ss << "[" << indexes(i)->to_string() << ", " << page_ids(i)->to_string() << "] ";
      }
      ss << "]\n";

      spdlog::info("{}", ss.str());
  }

  void update_buffer_with_new_values() {
    std::memcpy(internal_data - sizeof(page_id_t), &previous_page_id,
                sizeof(page_id_t));
    std::memcpy(internal_data - 2 * sizeof(page_id_t), &current_size,
                sizeof(current_size));
    std::memcpy(internal_data - 3 * sizeof(page_id_t), &max_size,
                sizeof(max_size));
    std::memcpy(internal_data - 4 * sizeof(page_id_t), &type, sizeof(type));
  }

  void append(const K& key, const V& value) {
    ASSERT(can_insert_trivially());

    *indexes(current_size - 1) = key;
    *page_ids(current_size - 1) = value;
    current_size++;

    update_buffer_with_new_values();
  }

  std::uint64_t find_index_for_insert(const K& key) const {
    ASSERT(can_insert_trivially());
    if (current_size < 5) {
      std::uint64_t idx = 0;
      while (idx != (current_size-1)) {
        if ((*indexes(idx)) > key) break;
        idx++;
      }

      return idx;
    }

    std::int64_t high = current_size - 2;
    std::int64_t low = 0;

    if (key > *indexes(high)) {
      return current_size-1;
    }

    ASSERT(low < high);
    while (high > low) {
      std::int64_t middle = (high + low) / 2;
      if (*indexes(middle) >= key) {
        high = middle;
      } else {
        low = middle + 1;
      }
    }

    ASSERT(low >= 0);
    return static_cast<std::uint64_t>(low);
  }

  template <value_t V_leaf>
  void trivial_insert_leaf_node(const V& value,
                                b_plus_tree_leaf_node<K, V_leaf>& new_node) {
    ASSERT(can_insert_trivially());

    const auto key = *new_node.indexes(0);

    const auto where_to_insert = find_index_for_insert(key);

    if (where_to_insert == (current_size - 1)) {
      *indexes(current_size) = *indexes(current_size-1);
      *page_ids(current_size) = *page_ids(current_size-1);

      *indexes(current_size-1) = key;
      *page_ids(current_size-1) = value;

      current_size++;
      std::swap(*page_ids(current_size - 2), *page_ids(current_size-1));

      update_buffer_with_new_values();
      return;
    }

    for (auto i = current_size; i > where_to_insert; --i) {
      *indexes(i) = *indexes(i - 1);
      *page_ids(i) = *page_ids(i - 1);
    }

    *indexes(where_to_insert) = key;
    *page_ids(where_to_insert) = value;

    ASSERT(where_to_insert + 1 < current_size);
    std::swap(*page_ids(where_to_insert), *page_ids(where_to_insert+1));

    current_size++;

    update_buffer_with_new_values();
  }

  void split_node(b_plus_tree_inner_node& new_node, page_id_t previous_page) {
    ASSERT(should_split());


    new_node.type = b_plus_tree_node_type::inner_node;
    new_node.max_size = max_size;
    std::copy(indexes(max_size/2+1), indexes(max_size-1), new_node.indexes(0));
    std::copy(page_ids(max_size/2+1), page_ids(max_size-1), new_node.page_ids(0));

    *new_node.indexes(max_size/2 - something_to_be_renamed) = *indexes(max_size-1);
    *new_node.page_ids(max_size/2 - something_to_be_renamed) = *page_ids(max_size-1);

    std::for_each(indexes(max_size/2+1), indexes(max_size-1), [] (K& key) {key = K::invalid_key();});
    std::for_each(page_ids(max_size/2+1), page_ids(max_size-1), [] (V& val) {val = V::invalid_key();});

    current_size = current_size / 2 + 1;
    new_node.current_size = current_size - something_to_be_renamed;
    new_node.previous_page_id = previous_page;

    update_buffer_with_new_values();
    new_node.update_buffer_with_new_values();
  }

  void trivial_insert_inner_node(const V& value,
                                 b_plus_tree_inner_node& new_node,
                                 b_plus_tree_inner_node& previous_node) {
    ASSERT(can_insert_trivially());
    ASSERT(false);

    const auto key = *new_node.indexes(0);

    const auto where_to_insert = find_index(key);

    if (where_to_insert == (current_size - 1)) {
      append(key, value);
      current_size++;
      std::swap(*indexes(current_size - 1), *indexes(current_size - 2));

      const auto value_to_insert_into_prev_node = *page_ids(0);

      std::memcpy(page_ids(0), page_ids(1), current_size - 1);
      std::memcpy(indexes(0), indexes(1), current_size - 1);
      current_size--;

      previous_node.append(K::invalid_key(), value_to_insert_into_prev_node);

      previous_node.update_buffer_with_new_values();
      update_buffer_with_new_values();
      return;
    }

    for (auto i = (current_size - 1); i > where_to_insert; --i) {
      *indexes(i) = *indexes(i - 1);
      *page_ids(i) = *page_ids(i - 1);
    }

    *indexes(where_to_insert) = key;
    *page_ids(where_to_insert) = value;

    current_size++;

    ASSERT(where_to_insert + 1 < current_size);
    std::swap(*page_ids(where_to_insert), *page_ids(where_to_insert + 1));

    const auto value_to_insert_into_prev_node = *page_ids(0);

    std::memcpy(page_ids(0), page_ids(1), current_size - 1);
    std::memcpy(indexes(0), indexes(1), current_size - 1);

    current_size--;

    previous_node.append(K::invalid_key(), value_to_insert_into_prev_node);

    update_buffer_with_new_values();
    previous_node.update_buffer_with_new_values();
  }

  std::uint64_t find_index(const K& key) const {
    ASSERT(current_size > 0);
    std::int64_t high = current_size - 2;

    // Handle the case where the last key is "nil" or the invalid one
    if (key >= *indexes(high)) {
      return current_size - 1;
    }

    std::int64_t low = 0;
    ASSERT(high >= low);

    while (high > low) {
      std::int64_t middle = (high + low) / 2;
      if (*indexes(middle) > key) {
        high = middle;
      } else {
        low = middle + 1;
      }
    }

    ASSERT(low >= 0);
    return static_cast<uint64_t>(low);
  }

  page_id_t previous_page_id;

  K* indexes(std::uint64_t index) const {
    ASSERT(index < max_size);
    return K::deserialize(internal_data + index * sizeof(K));
  }

  V* page_ids(std::uint64_t index) const {
    ASSERT(index < max_size);
    constexpr auto offset = MAX_NUMBER_OF_ELEMENTS * sizeof(K);
    return V::deserialize(internal_data + (offset + index * sizeof(V)));
  }
};

}  // namespace hivedb
