#pragma once
#include <b_plus_tree/b_plus_tree_node.hpp>
#include <buffer_pool/buffer_pool.hpp>
#include <disk/disk_scheduler.hpp>
#include <memory>
#include <misc/config.hpp>
#include <optional>
#include <queue>
#include <stdexcept>
#include <vector>

namespace hivedb {
template <disk_manager_t T, index_t K, value_t V, value_t V_leaf>
struct b_plus_tree {
 private:
  std::unique_ptr<buffer_pool<T>> m_owned_bp{nullptr};
  buffer_pool<T> *m_bp_ptr{nullptr};
  page_id_t m_root_page_id{INVALID_PAGE_ID};

  [[nodiscard]] buffer_pool<T> &bp() noexcept { return *m_bp_ptr; }
  [[nodiscard]] const buffer_pool<T> &bp() const noexcept { return *m_bp_ptr; }

  static page_id_t to_page_id(const V& v) {
    if constexpr (requires { v.key; }) {
      return v.key;
    } else {
      return static_cast<page_id_t>(v);
    }
  }

 public:
  b_plus_tree() = delete;
  explicit b_plus_tree(page_id_t root_page_id, std::int32_t max_frames,
                       const std::filesystem::path &path)
      : m_owned_bp(std::make_unique<buffer_pool<T>>(max_frames, path)),
        m_bp_ptr(m_owned_bp.get()),
        m_root_page_id(root_page_id) {}

  explicit b_plus_tree(page_id_t root_page_id, buffer_pool<T> &shared_bp)
      : m_owned_bp(nullptr),
        m_bp_ptr(&shared_bp),
        m_root_page_id(root_page_id) {}

  [[nodiscard]]
  page_id_t get_root_page_id() const noexcept {
    return m_root_page_id;
  }

  template <typename Callback>
  void scan(Callback &&cb) {
    if (m_root_page_id == INVALID_PAGE_ID) {
      return;
    }

    page_id_t current_page_id = m_root_page_id;
    while (true) {
      auto const current_page = &bp().request_page(current_page_id);
      auto node = b_plus_tree_node(current_page->get_data());

      if (node.type == b_plus_tree_node_type::inner_node) {
        const auto inner_node =
            b_plus_tree_inner_node<K, V>(current_page->get_data());
        const auto next_page_id = *inner_node.page_ids(0);
        current_page_id = to_page_id(next_page_id);
        current_page->decrease_pin_count();
      } else {
        current_page->decrease_pin_count();
        break;
      }
    }

    while (current_page_id != INVALID_PAGE_ID) {
      auto const current_page = &bp().request_page(current_page_id);
      const auto leaf_node =
          b_plus_tree_leaf_node<K, V_leaf>(current_page->get_data());

      for (std::uint64_t i = 0; i < leaf_node.current_size; ++i) {
        cb(*leaf_node.indexes(i), *leaf_node.records(i));
      }

      const auto next_id = leaf_node.next_page_id;
      current_page->decrease_pin_count();
      current_page_id = next_id;
    }
  }

  void dump_contents() {
    std::queue<page_id_t> page_ids{};
    page_ids.push(m_root_page_id);

    while (!page_ids.empty()) {
      auto const current_page = &bp().request_page(page_ids.front(), false);
      page_ids.pop();
      auto node = b_plus_tree_node(current_page->get_data());

      if (node.type == b_plus_tree_node_type::inner_node) {
        const auto inner_node =
            b_plus_tree_inner_node<K, V>(current_page->get_data());

        inner_node.dump_contents();
        auto idx = 0u;
        while (idx < inner_node.current_size) {
          page_ids.push(to_page_id(*inner_node.page_ids(idx)));
          idx++;
        }
        continue;
      }

      const auto leaf_node =
          b_plus_tree_leaf_node<K, V_leaf>(current_page->get_data());

      leaf_node.dump_contents();
    }
  }

  void find(const K &key, V_leaf &value) {
    if (m_root_page_id == INVALID_PAGE_ID) {
      throw std::invalid_argument("B+ tree search failed: root page does not exist (tree is empty)");
    }

    page_id_t current_page_id = m_root_page_id;

    while (true) {
      auto const current_page = &bp().request_page(current_page_id);
      auto node = b_plus_tree_node(current_page->get_data());

      if (node.type == b_plus_tree_node_type::inner_node) {
        const auto inner_node =
            b_plus_tree_inner_node<K, V>(current_page->get_data());
        const auto next_key_index = inner_node.find_index(key);
        const auto next_page_id = *inner_node.page_ids(next_key_index);

        current_page_id = to_page_id(next_page_id);
        current_page->decrease_pin_count();
        continue;
      }

      const auto leaf_node =
          b_plus_tree_leaf_node<K, V_leaf>(current_page->get_data());
      const auto idx = leaf_node.find_index(key);
      if (!idx.has_value()) {
        current_page->decrease_pin_count();
        return;
      }

      value = *leaf_node.records(idx.value());
      current_page->decrease_pin_count();
      return;
    }
  }

  [[nodiscard]]
  bool insert(const K &key, const V_leaf &value) {
    if constexpr (requires { key.key; value.key; }) {
      spdlog::debug("Inserting {} and {}...", key.key, value.key);
    } else {
      spdlog::debug("Inserting entry into b_plus_tree...");
    }
    if (m_root_page_id == INVALID_PAGE_ID) {
      m_root_page_id = bp().allocate_new_page();
      auto const root_node_frame = &bp().request_page(m_root_page_id, false);

      root_node_frame->is_dirty = true;
      auto new_node =
          b_plus_tree_leaf_node<K, V_leaf>(root_node_frame->get_data());

      new_node.init_first_node(key, value);

      return bp().flush_page(m_root_page_id);
    }

    // Keep the ancestor search path local to this insert invocation so concurrent threads
    // maintain independent traversal stacks.
    std::vector<frame_header *> frame_queue;
    page_id_t current_page_id = m_root_page_id;

    K split_key = K::invalid_key();
    page_id_t split_child_page_id = INVALID_PAGE_ID;
    bool has_split = false;

    while (true) {
      auto const current_frame = &bp().request_page(current_page_id);
      auto node = b_plus_tree_node(current_frame->get_data());

      if (node.type == b_plus_tree_node_type::inner_node) {
        auto inner_node = b_plus_tree_inner_node<K, V>(current_frame->get_data());
        const auto next_child_idx = inner_node.find_index(key);
        current_page_id = to_page_id(*inner_node.page_ids(next_child_idx));
        frame_queue.push_back(current_frame);
      } else {
        current_frame->is_dirty = true;
        auto leaf_node = b_plus_tree_leaf_node<K, V_leaf>(current_frame->get_data());

        leaf_node.trivial_insert(key, value);

        if (!leaf_node.should_split()) {
          while (!frame_queue.empty()) {
            frame_queue.back()->decrease_pin_count();
            frame_queue.pop_back();
          }
          current_frame->decrease_pin_count();
          return bp().flush_pages();
        }

        const auto new_leaf_page_id = bp().allocate_new_page();
        auto const new_leaf_frame = &bp().request_page(new_leaf_page_id);
        new_leaf_frame->is_dirty = true;
        auto new_leaf_node = b_plus_tree_leaf_node<K, V_leaf>(new_leaf_frame->get_data());

        leaf_node.split_node(new_leaf_node, new_leaf_page_id);

        split_key = *new_leaf_node.indexes(0);
        split_child_page_id = new_leaf_page_id;
        has_split = true;

        // Leaf split complete: unpin both leaves so they are evictable/flushable
        // as inner node splits propagate up the tree.
        current_frame->decrease_pin_count();
        new_leaf_frame->decrease_pin_count();
        break;
      }
    }

    while (!frame_queue.empty()) {
      auto const current_frame = frame_queue.back();
      frame_queue.pop_back();

      if (!has_split) {
        current_frame->decrease_pin_count();
        continue;
      }

      current_frame->is_dirty = true;
      auto inner_node = b_plus_tree_inner_node<K, V>(current_frame->get_data());

      inner_node.insert_key_and_child(split_key, V{split_child_page_id});

      if (!inner_node.should_split()) {
        has_split = false;
        current_frame->decrease_pin_count();
        continue;
      }

      const auto new_inner_page_id = bp().allocate_new_page();
      auto const new_inner_frame = &bp().request_page(new_inner_page_id);
      new_inner_frame->is_dirty = true;

      auto new_inner_node =
          b_plus_tree_inner_node<K, V>(new_inner_frame->get_data());

      split_key = inner_node.split_node(new_inner_node);
      split_child_page_id = new_inner_page_id;
      has_split = true;

      // Inner node split complete: unpin both siblings.
      current_frame->decrease_pin_count();
      new_inner_frame->decrease_pin_count();
    }

    if (has_split) {
      const auto new_root = bp().allocate_new_page();
      auto const new_root_frame = &bp().request_page(new_root);
      new_root_frame->is_dirty = true;
      auto new_root_node = b_plus_tree_inner_node<K, V>(new_root_frame->get_data());

      new_root_node.max_size = b_plus_tree_inner_node<K, V>::MAX_NUMBER_OF_ELEMENTS;
      new_root_node.current_size = 2;
      new_root_node.type = b_plus_tree_node_type::inner_node;
      new_root_node.previous_page_id = INVALID_PAGE_ID;

      *new_root_node.indexes(0) = split_key;
      *new_root_node.page_ids(0) = V{m_root_page_id};

      *new_root_node.indexes(1) = K::invalid_key();
      *new_root_node.page_ids(1) = V{split_child_page_id};

      m_root_page_id = new_root;

      new_root_node.update_buffer_with_new_values();
      new_root_frame->decrease_pin_count();
    }

    return bp().flush_pages();
  }

  bool remove(const K &) { throw std::invalid_argument("B+ tree key deletion (remove) is not yet supported"); }
};
}  // namespace hivedb
