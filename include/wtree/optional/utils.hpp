#ifndef _WTREE_UTILS__H_
#define _WTREE_UTILS__H_

#include "../detail/index.hpp"

#include <cassert>
#include <sys/types.h>
#include <type_traits>

namespace WTreeLib {

class WTreeValidationUtils {
public:
  template <typename Params>
  static bool validate_wtree(WTree<Params> &wt, long expected_values = -1) {
    return validate_since_node(wt.root(), expected_values);
  }

  template <typename Params>
  static bool validate_since_node(WTreeNode<Params> *node,
                                  long expected_values = -1) {
    long total_values = 0;
    bool res = validate_node_and_count(node, total_values);

    if (expected_values != -1 && total_values != expected_values) {
      printf("[VERIFY ERROR :: Expected Size] W-tree has [%4lu / %4lu] "
             "values.\n",
             total_values, expected_values);
      res = false;
    }

    return res;
  }

  template <typename IteratorType>
  static bool validate_iterator_path(IteratorType &it) {
    using node_type = typename IteratorType::node_type;
    using field_type = typename node_type::field_type;
    using key_type = typename node_type::key_type;

    std::vector<node_type *> path(it.ascendants);
    path.push_back(it.node);

    bool is_correct = true;
    const size_t size = it.ascendants.size();
    for (size_t i_node = 0; i_node < size; ++i_node) {
      node_type *node = it.ascendants[i_node];
      assert(node != nullptr);

      if (node->size() > node_type::kTargetK) {
        printf("[VERIFY ERROR :: Size] node has n=%lu/%lu\n",
               (ulong)node->size(), (ulong)node_type::kTargetK);
        printf("On node %lu/%lu of path:\n", (ulong)i_node + 1, (ulong)size);
        return false;
      }

      for (field_type i = 0; i < node->size() - 1; ++i) {
        if (!(node->key(i) < node->key(i + 1))) {
          if constexpr (std::is_arithmetic_v<key_type>) {
            printf("[VERIFY ERROR :: Order] Not in order: %lu %lu\n",
                   (ulong)node->key(i), (ulong)node->key(i + 1));
          } else {
            printf("[VERIFY ERROR :: Order] Not in order.\n");
          }
          printf("On node %lu/%lu of path:\n", (ulong)i_node + 1, (ulong)size);
          printf("At index = %lu\n", (ulong)i);
          is_correct = false;
        }
        if (!is_correct) {
          return false;
        }

        if (node->is_leaf())
          continue;
        node_type *w = node->child(i);
        if (w == nullptr)
          continue;

        if (!(node->key(i) < w->key(0)) ||
            !(w->key(w->size() - 1) < node->key(i + 1))) {
          if constexpr (std::is_arithmetic_v<key_type>) {
            printf("[VERIFY ERROR :: Not bounded] Child is not "
                   "correctly bound "
                   "by current node:\n"
                   "\t(u=%lu, w=%lu) , (w=%lu, u=%lu)\n",
                   (ulong)node->key(i), (ulong)w->key(0),
                   (ulong)w->key(w->size() - 1), (ulong)node->key(i + 1));
          } else {
            printf("[VERIFY ERROR :: Not bounded] Child is not "
                   "correctly bound by current node.\n");
          }
          printf("On node %lu/%lu of path:\n", (ulong)i_node + 1, (ulong)size);
          printf("At index = %lu\n", (ulong)i);
          is_correct = false;
        }

        if (!is_correct) {
          return false;
        }
      }
    }

    return true;
  }

  /**
   * Validates the iterator path (like validate_iterator_path) and
   * additionally checks the mapped value at each node slot using a
   * user-provided validator.
   * @param validator A callable (key, value) -> bool that returns true if the
   *                  value is consistent with its key.
   */
  template <typename IteratorType, typename Validator>
  static bool validate_map_iterator_path(IteratorType &it,
                                         Validator &&validator) {
    using node_type = typename IteratorType::node_type;
    using field_type = typename node_type::field_type;
    using key_type = typename node_type::key_type;
    using value_type = typename IteratorType::value_type;

    // First, run the structural validation.
    if (!WTreeLib::WTreeValidationUtils::validate_iterator_path(it))
      return false;

    // Then validate values along the path.
    for (size_t i_node = 0; i_node < it.ascendants.size(); ++i_node) {
      node_type *node = it.ascendants[i_node];
      for (field_type i = 0; i < node->size(); ++i) {
        const value_type &entry = node->fields.values[i];
        if (!validator(entry.first, entry.second)) {
          if constexpr (std::is_arithmetic_v<key_type>) {
            printf("[VERIFY ERROR :: Value] Value validation "
                   "failed for key %lu\n",
                   (ulong)entry.first);
          } else {
            printf("[VERIFY ERROR :: Value] Value validation "
                   "failed.\n");
          }
          printf("On node %lu/%lu of path, index %lu\n", (ulong)i_node + 1,
                 (ulong)it.ascendants.size(), (ulong)i);
          return false;
        }
      }
    }

    // Validate the leaf node the iterator points into.
    node_type *leaf = it.node;
    for (field_type i = 0; i < leaf->size(); ++i) {
      const value_type &entry = leaf->fields.values[i];
      if (!validator(entry.first, entry.second)) {
        if constexpr (std::is_arithmetic_v<key_type>) {
          printf("[VERIFY ERROR :: Value] Value validation failed "
                 "for key %lu\n",
                 (ulong)entry.first);
        } else {
          printf("[VERIFY ERROR :: Value] Value validation "
                 "failed.\n");
        }
        printf("On leaf node, index %lu\n", (ulong)i);
        return false;
      }
    }

    return true;
  }

private:
  template <typename Params>
  static bool validate_node_and_count(WTreeNode<Params> *node,
                                      long &count_values) {
    using node_type = WTreeNode<Params>;
    using field_type = typename node_type::field_type;
    using key_type = typename node_type::key_type;

    if (node == nullptr)
      return true;
    count_values += node->size();

    if (node->size() > node_type::kTargetK) {
      printf("[VERIFY ERROR :: Size] node has n=%lu/%lu\n", (ulong)node->size(),
             (ulong)node_type::kTargetK);
      return false;
    }

    for (field_type i = 0; i < node->size() - 1; ++i) {
      if (!(node->key(i) < node->key(i + 1))) {
        if constexpr (std::is_arithmetic_v<key_type>) {
          printf("[VERIFY ERROR :: Order] Not in order: %lu %lu\n",
                 (ulong)node->key(i), (ulong)node->key(i + 1));
        } else {
          printf("[VERIFY ERROR :: Order] Not in order.\n");
        }
        return false;
      }

      if (node->is_leaf())
        continue;
      node_type *w = node->child(i);
      if (w == nullptr)
        continue;

      if (!(node->key(i) < w->key(0)) ||
          !(w->key(w->size() - 1) < node->key(i + 1))) {
        if constexpr (std::is_arithmetic_v<key_type>) {
          printf("[VERIFY ERROR :: Not bounded] Descendant bounds are "
                 "not correct:\n"
                 "\t(u=%lu, w=%lu) , (w=%lu, u=%lu)\n",
                 (ulong)node->key(i), (ulong)w->key(0),
                 (ulong)w->key(w->size() - 1), (ulong)node->key(i + 1));
        } else {
          printf("[VERIFY ERROR :: Not bounded] Descendant bounds "
                 "are not correct.\n");
        }
        return false;
      }
      if (!validate_node_and_count(w, count_values))
        return false;
    }

    return true;
  }
};

template <typename Params> struct ModifiableWTree : public WTree<Params> {
  static bool increase_size(ModifiableWTree &tree, size_t size) {
    tree.m_manager.increase_size_by(size);
    // tree.m_size += size; // Can access protected member
    return true;
  }

  static bool increase_size(WTree<Params> &tree, size_t size) {
    static_cast<ModifiableWTree &>(tree).m_manager.increase_size_by(size);
    return true;
  }

  static bool decrease_size(ModifiableWTree &tree, size_t size) {
    assert(tree.size() >= size);
    tree.m_manager.decrease_size_by(size);
    return true;
  }

  static bool decrease_size(WTree<Params> &tree, size_t size) {
    assert(tree.size() >= size);
    static_cast<ModifiableWTree &>(tree).m_manager.decrease_size_by(size);
    return true;
  }
};

class WTreeGenerationUtils {
public:
  template <typename Params>
  static bool fill_root_node(WTree<Params> &wtree, ulong size,
                             typename WTree<Params>::key_type left,
                             typename WTree<Params>::key_type right,
                             bool updateSize = true) {
    assert(size <= WTree<Params>::node_type::kTargetK);
    using key_type = typename WTree<Params>::key_type;
    using modify_adapter = ModifiableWTree<Params>;

    if (updateSize)
      modify_adapter::decrease_size(wtree, wtree.mutable_root()->size());
    if (wtree.mutable_root()->capacity() < size) {
      wtree.mutable_root() = wtree.manager()->new_leaf_node(size);
    }

    // Try to leave a gap on the border to use specific values when
    // inserting.
    key_type gap = (right - left) / (size + 1);
    left += gap;
    for (ulong i = 0; i < size; ++i, left += gap)
      wtree.mutable_root()->fields.values[i] = left;

    wtree.mutable_root()->fields.size = size;
    if (updateSize)
      modify_adapter::increase_size(wtree, size);
    return true;
  }

  template <typename Params>
  static bool
  fill_node(WTree<Params> &wtree, typename WTree<Params>::node_type **u,
            ulong size, typename WTree<Params>::key_type left,
            typename WTree<Params>::key_type right, bool updateSize = true) {
    assert(size <= WTree<Params>::node_type::kTargetK);
    using key_type = typename WTree<Params>::key_type;
    using modify_adapter = ModifiableWTree<Params>;

    if (updateSize)
      modify_adapter::decrease_size(wtree, (*u)->size());
    if ((*u)->fields.capacity < size) {
      (*u) = wtree.manager()->new_leaf_node(size);
    }

    // Try to leave a gap on the border to use specific values when
    // inserting.
    key_type gap = (right - left) / (size + 1);
    left += gap;
    for (ulong i = 0; i < size; ++i, left += gap)
      (*u)->fields.values[i] = left;

    (*u)->fields.size = size;
    if (updateSize)
      modify_adapter::increase_size(wtree, size);
    return true;
  }

  template <typename Params>
  static bool fill_child(WTree<Params> &wtree,
                         typename WTree<Params>::node_type *node,
                         typename WTree<Params>::field_type index,
                         typename WTree<Params>::field_type size) {
    using node_type = typename WTree<Params>::node_type;
    assert(node->fields.is_internal);
    assert(index < node_type::kTargetK - 1);

    node_type *u = node->fields.pointers[index];
    if (!u)
      u = wtree.manager()->new_leaf_node(size);
    else if (u->fields.capacity < size)
      u = wtree.manager()->grow_leaf_to_size(u, size);
    node->fields.pointers[index] = u;

    return fill_node(wtree, &(node->fields.pointers[index]), size,
                     node->fields.values[index],
                     node->fields.values[index + 1]);
  }

  // template <typename Params>
  // bool eraseNodeChildren(node_type *u, bool updateSize) {
  //   if (!u->_fields.isInternal)
  //     return true;

  //   for (typename node_type::field_type i = 0; i < node_type::c_target_k -
  //   1;
  //        ++i) {
  //     if (u->_fields.pointers[i] == nullptr)
  //       continue;
  //     size -= u->_fields.pointers[i]->_fields.n;
  //     if (u->_fields.pointers[i]->_fields.isInternal) {
  //       eraseNodeChildren(u->_fields.pointers[i], updateSize);
  //       delete_internal_node(u->_fields.pointers[i]);
  //     } else
  //       delete_leaf_node(u->_fields.pointers[i]);
  //     u->_fields.pointers[i] = nullptr;
  //   }

  //   return true;
  // }
};

} // namespace WTreeLib
#endif