#pragma once
#include <b_plus_tree/b_plus_tree_node.hpp>
#include <buffer_pool/buffer_pool.hpp>
#include <deque>
#include <disk/disk_scheduler.hpp>
#include <misc/config.hpp>
#include <optional>
#include <stdexcept>

namespace hivedb {
template <disk_manager_t T, index_t K, value_t V, value_t V_leaf>
struct b_plus_tree {
 private:
  buffer_pool<T> m_bp;
  page_id_t m_root_page_id{INVALID_PAGE_ID};
  std::deque<frame_header *> m_frame_queue;


  void create_root_with_inner_nodes(page_id_t victim_page_id, page_id_t rhs_page_id) {
        spdlog::info("bruh");
        auto const victim_frame = &m_bp.request_page(victim_page_id);
        victim_frame->is_dirty = true;

        auto victim = b_plus_tree_inner_node<K, V>(victim_frame->get_data());

        const auto new_root = m_bp.allocate_new_page();
        auto const new_root_frame = &m_bp.request_page(new_root);
        new_root_frame->is_dirty = true;
        auto new_root_node = b_plus_tree_inner_node<K, V>(new_root_frame->get_data());

        new_root_node.max_size = b_plus_tree_inner_node<K, V>::MAX_NUMBER_OF_ELEMENTS;
        new_root_node.current_size = 2;
        new_root_node.type = b_plus_tree_node_type::inner_node;
        new_root_node.previous_page_id = INVALID_PAGE_ID;

        *new_root_node.indexes(0) = *victim.indexes(victim.current_size-1);
        *new_root_node.page_ids(0) = victim_page_id;

        *new_root_node.indexes(1) = K::invalid_key();
        *new_root_node.page_ids(1) = rhs_page_id;

        *victim.indexes(victim.current_size-1) = K::invalid_key();

        new_root_node.update_buffer_with_new_values();
        victim.update_buffer_with_new_values();
        // new_root_node.dump_contents(1);

        // victim.dump_contents(+1);

        m_root_page_id = new_root;
        // throw std::runtime_error("foo:(");
  }
 public:
  b_plus_tree() = delete;
  explicit b_plus_tree(page_id_t root_page_id, std::int32_t max_frames,
                       const std::filesystem::path &path)
      : m_bp(max_frames, path), m_root_page_id(root_page_id), m_frame_queue() {}


  void dump_contents() {
    std::queue<page_id_t> page_ids{};
    page_ids.push(m_root_page_id);

    while (!page_ids.empty()) {
      auto const current_page = &m_bp.request_page(page_ids.front(), false);
      page_ids.pop();
      auto node = b_plus_tree_node(current_page->get_data());

      if (node.type == b_plus_tree_node_type::inner_node) {
        const auto inner_node =
            b_plus_tree_inner_node<K, V>(current_page->get_data());

        inner_node.dump_contents();
        auto idx = 0u;
        while (idx < inner_node.current_size) {
            page_ids.push(inner_node.page_ids(idx)->key);
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
      throw std::invalid_argument("root page doesn't exist!");
    }

    page_id_t previous_page_id = m_root_page_id;

    while (true) {
      auto const current_page = &m_bp.request_page(previous_page_id);
      auto node = b_plus_tree_node(current_page->get_data());

      if (node.type == b_plus_tree_node_type::inner_node) {
        const auto inner_node =
            b_plus_tree_inner_node<K, V>(current_page->get_data());
        const auto next_key_index = inner_node.find_index(key);
        const auto next_page_id = *inner_node.page_ids(next_key_index);

        // TODO: REPLACE THIS
        previous_page_id = next_page_id.key;
        current_page->decrease_pin_count();

        continue;
      }
    const auto leaf_node =
        b_plus_tree_leaf_node<K, V_leaf>(current_page->get_data());
    const auto idx = leaf_node.find_index(key);
    if (!idx.has_value())
        return;

    value = *leaf_node.records(idx.value());

    current_page->decrease_pin_count();
    return;
    }
  }

  std::optional<page_id_t> find_place_for_new_key_and_insert(const K &key,
                                                             const V &value) {
    page_id_t previous_page_id = m_root_page_id;
    std::optional<page_id_t> new_page_id{std::nullopt};

    while (true) {
      auto const current_page = &m_bp.request_page(previous_page_id);
      auto node = b_plus_tree_node(current_page->get_data());

      if (node.type == b_plus_tree_node_type::inner_node) {
        auto inner_node =
            b_plus_tree_inner_node<K, V>(current_page->get_data());
        const auto next_key_index = inner_node.find_index(key);
        const auto next_page_id = *inner_node.page_ids(next_key_index);
        previous_page_id = next_page_id.key;
        m_frame_queue.emplace_back(current_page);
        continue;
      } else {
        current_page->is_dirty = true;
        auto leaf_node =
            b_plus_tree_leaf_node<K, V_leaf>(current_page->get_data());

        leaf_node.trivial_insert(key, value);
        if (!leaf_node.should_split()) {
          return std::nullopt;
        }

        new_page_id = m_bp.allocate_new_page();
        auto const new_page = &m_bp.request_page(new_page_id.value());
        new_page->is_dirty = true;
        auto new_leaf_node =
            b_plus_tree_leaf_node<K, V_leaf>(new_page->get_data());
        leaf_node.split_node(new_leaf_node, new_page_id.value());
        // new_page->decrease_pin_count();
        break;
      }
    };
    return new_page_id;
  }

  // handle the case when to root splits
  [[nodiscard]]
  bool insert(const K &key, const V_leaf &value) {
    spdlog::info("Inserting {} and {}...", key.key, value.key);
    if (m_root_page_id == INVALID_PAGE_ID) {
      m_root_page_id = m_bp.allocate_new_page();
      auto const root_node_frame = &m_bp.request_page(m_root_page_id, false);

      root_node_frame->is_dirty = true;
      auto new_node =
          b_plus_tree_leaf_node<K, V_leaf>(root_node_frame->get_data());

      new_node.init_first_node(key, value);

      return m_bp.flush_page(m_root_page_id);
    }

    auto new_page_id = find_place_for_new_key_and_insert(key, value);

    //the root leaf page was split, time to create an inner node as a root
    if (new_page_id.has_value() && m_frame_queue.empty()) {
        const auto new_root = m_bp.allocate_new_page();
        auto const new_root_frame = &m_bp.request_page(new_root);
        new_root_frame->is_dirty = true;
        auto new_root_node = b_plus_tree_inner_node<K, V>(new_root_frame->get_data());

        //this one will be the right node. look at b_plus_tree_leaf::split()
        auto const new_frame = &m_bp.request_page(new_page_id.value());
        auto new_leaf_node = b_plus_tree_leaf_node<K, V_leaf>(new_frame->get_data());

        new_root_node.max_size = b_plus_tree_inner_node<K, V_leaf>::MAX_NUMBER_OF_ELEMENTS;
        new_root_node.current_size = 2;
        new_root_node.type = b_plus_tree_node_type::inner_node;
        new_root_node.previous_page_id = INVALID_PAGE_ID;

        *new_root_node.indexes(0) = *new_leaf_node.indexes(0);
        *new_root_node.page_ids(0) = V{m_root_page_id};

        *new_root_node.indexes(1) = K::invalid_key();
        *new_root_node.page_ids(1) = new_page_id.value();

        m_root_page_id = new_root;

        new_root_node.update_buffer_with_new_values();
        return m_bp.flush_pages();
    }

    while (!m_frame_queue.empty()) {
      if (!new_page_id.has_value()) {
        m_frame_queue.back()->decrease_pin_count();
        m_frame_queue.pop_back();
        continue;
      }

      auto const current_frame = m_frame_queue.back();
      current_frame->is_dirty = true;

      auto inner_node = b_plus_tree_inner_node<K, V>(current_frame->get_data());

      auto const new_node_frame =
          &m_bp.request_page(new_page_id.value(), false);
      const auto new_node = b_plus_tree_node(new_node_frame->get_data());

      if (new_node.type == b_plus_tree_node_type::inner_node) {
        auto previous_node =
            b_plus_tree_inner_node<K, V>(current_frame->get_data());

        auto new_inner_node =
            b_plus_tree_inner_node<K, V>(new_node_frame->get_data());
        inner_node.trivial_insert_inner_node(new_page_id.value(),
                                             new_inner_node, previous_node);
      } else {
        auto new_leaf_node =
            b_plus_tree_leaf_node<K, V_leaf>(new_node_frame->get_data());
        inner_node.template trivial_insert_leaf_node<V_leaf>(
            new_page_id.value(), new_leaf_node);
      }

      if (!inner_node.should_split()) {
        new_page_id = std::nullopt;

        new_node_frame->decrease_pin_count();
        current_frame->decrease_pin_count();
        m_frame_queue.pop_back();
        continue;
      }

      const auto previous_page_id = new_page_id.value();
      new_page_id = m_bp.allocate_new_page();
      auto const new_page = &m_bp.request_page(new_page_id.value());
      new_page->is_dirty = true;
      auto new_inner_node = b_plus_tree_inner_node<K, V>(new_page->get_data());

      inner_node.split_node(new_inner_node, previous_page_id);

      new_node_frame->decrease_pin_count();
      current_frame->decrease_pin_count();
      m_frame_queue.pop_back();
    }

    if (new_page_id.has_value()) {
        create_root_with_inner_nodes(m_root_page_id, new_page_id.value());
    }

    return m_bp.flush_pages();
  };
  bool remove(const K &) { throw std::invalid_argument("doesnt owrk eyrt"); }
};
};  // namespace hivedb
