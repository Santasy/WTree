#ifndef _WTREE_PRINT_H_
#define _WTREE_PRINT_H_

#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace WTreeLib {

#define TypedPrinter(__T, __SIZE)                                              \
  WTreeLib::WTreePrinter<typename WTreeLib::set<__T, __SIZE>::wtree_type>

// Type trait to detect if value_type is a pair (map) or not (set)
template <typename T> struct is_pair : std::false_type {};

template <typename K, typename V>
struct is_pair<std::pair<K, V>> : std::true_type {};

template <typename T> inline constexpr bool is_pair_v = is_pair<T>::value;

template <typename WTreeType> class WTreePrinter {
  using node_type = typename WTreeType::node_type;
  using field_type = typename WTreeType::field_type;
  using value_type = typename WTreeType::value_type;

  // Detect if this is a map (pair values) or set (single values)
  static constexpr bool is_map_type = is_pair_v<value_type>;

public:
  struct PrintOptions {
    bool show_indices = false;        // Show [0], [1], [2] for keys
    bool show_node_addresses = false; // Show memory addresses
    bool show_capacity_info = true;   // Show [size/capacity]
    bool compact_values = false;      // Compress long value lists
    bool color_output = false;        // Use ANSI colors (if terminal supports)
    int max_values_per_line = 8;      // Break long value lists
    int indent_size = 2;              // Spaces per indentation level
    bool show_null_ranges = true;     // Show NULL pointer ranges
    bool vertical_spacing = false;    // Add blank lines between levels
    bool show_map_values = true;      // For maps: show values alongside keys
  };

  PrintOptions options;

public:
  WTreePrinter(const PrintOptions &opts = PrintOptions{}) : options(opts) {}

  void print_tree_simple(const node_type *root) const {
    print_tree(root, "WTree Container");
  }

  void print_tree(const node_type *root,
                  const std::string &title = "WTree") const {
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "Printing " << title;
    if (root) {
      std::cout << " [k:" << (int)node_type::kTargetK << "]";
    }
    std::cout << "\n" << std::string(50, '=') << "\n";

    if (root == nullptr) {
      std::cout << "<empty tree>\n";
      return;
    }

    print_node_improved(root, "", true, 0);
    std::cout << "\n";
  }

  void print_node(const node_type *node) const {
    if (node == nullptr) {
      std::cout << "[[NULL]]\n";
      return;
    }
    std::cout << format_node(node, 0) << "\n";
    if (node->fields.is_internal) {
      print_children_improved(node, "", true, 0);
    }
  }

  void print_compact(const node_type *root) const {
    if (root == nullptr) {
      std::cout << "<empty>\n";
      return;
    }

    std::cout << "WTree: ";
    print_compact_node(root, 0);
    std::cout << "\n";
  }

  // Node-level convenience methods (assert correct container kind)
  void print_set(const node_type *root,
                 const std::string &title = "WTree Set") const {
    static_assert(!is_map_type, "Use print_map() for map WTrees");
    print_tree(root, title);
  }

  void print_map(const node_type *root,
                 const std::string &title = "WTree Map") const {
    static_assert(is_map_type, "Use print_set() for set WTrees");
    print_tree(root, title);
  }

  // Container-level convenience methods (extract root from container)
  template <typename ContainerType>
  void print_set_container(const ContainerType &container,
                           const std::string &title = "WTree Set") const {
    static_assert(!is_map_type, "Use print_map_container() for map WTrees");
    print_tree(container.tree()->root(), title);
  }

  template <typename ContainerType>
  void print_map_container(const ContainerType &container,
                           const std::string &title = "WTree Map") const {
    static_assert(is_map_type, "Use print_set_container() for set WTrees");
    print_tree(container.tree()->root(), title);
  }

  // Get the type name for display
  static constexpr const char *tree_type_name() {
    return is_map_type ? "Map" : "Set";
  }

private:
  // Format a single value - handles both set (key only) and map (key:value)
  template <typename V> std::string format_value(const V &val) const {
    std::ostringstream oss;
    if constexpr (is_map_type) {
      oss << val.first;
      if (options.show_map_values) {
        oss << ":" << val.second;
      }
    } else {
      oss << val;
    }
    return oss.str();
  }

  std::string format_value_at(const node_type *node, int idx) const {
    return format_value(node->fields.values[idx]);
  }

  std::string format_node(const node_type *node, int indent) const {
    std::ostringstream oss;

    oss << std::dec;

    if (options.show_capacity_info || options.show_node_addresses) {
      oss << "[[";
      if (options.show_node_addresses) {
        oss << "@" << std::hex << node << std::dec;
        oss << (options.show_capacity_info ? " " : "");
      }

      oss << (int)node->size();
      if (node->capacity() != node->size()) {
        oss << "/" << (int)node->capacity();
      }
      oss << "]] ";
    }

    if (node->fields.size == 0) {
      oss << "<empty>";
      return oss.str();
    }

    if (options.compact_values &&
        node->fields.size > options.max_values_per_line) {
      oss << format_compact_keys(node, indent);
    } else {
      oss << "\n" << std::string(indent + options.indent_size, ' ');
      oss << format_all_keys(node, indent);
    }
    return oss.str();
  }

  std::string format_all_keys(const node_type *node, int indent) const {
    std::ostringstream oss;

    for (int i = 0; i < node->size(); ++i) {
      if (i > 0)
        oss << " ";
      if (options.show_indices) {
        oss << "[" << i << "]:";
      }
      oss << format_value_at(node, i);

      if (options.max_values_per_line > 0 &&
          (i + 1) % options.max_values_per_line == 0 && i + 1 < node->size()) {
        oss << "\n" << std::string(indent + options.indent_size, ' ');
      }
    }

    return oss.str();
  }

  std::string format_compact_keys(const node_type *node, int indent) const {
    std::ostringstream oss;

    if (node->fields.size <= 6) {
      return format_all_keys(node, indent);
    }

    oss << format_value_at(node, 0) << " " << format_value_at(node, 1) << " ";
    oss << "... (" << (node->fields.size - 4) << " more) ... ";
    oss << format_value_at(node, node->fields.size - 2) << " "
        << format_value_at(node, node->fields.size - 1);

    return oss.str();
  }

  void print_node_improved(const node_type *node, const std::string &prefix,
                           bool is_last, int depth) const {
    std::cout << prefix;
    std::cout << (is_last ? "└── " : "├── ");

    if (node == nullptr) {
      std::cout << "[[NULL]]\n";
      return;
    }

    // Connector is 4 chars wide ("└── " or "├── ")
    const int indent = static_cast<int>(prefix.size()) + 4;

    if (options.color_output) {
      if (node->fields.is_internal) {
        std::cout << "\033[1;34m"; // Blue for internal nodes
      } else {
        std::cout << "\033[1;32m"; // Green for leaf nodes
      }
    }

    std::cout << format_node(node, indent);
    if (options.color_output) {
      std::cout << "\033[0m";
    }

    std::cout << "\n";

    if (options.vertical_spacing && depth > 0) {
      std::cout << prefix << (is_last ? "    " : "│   ") << "\n";
    }

    if (node->fields.is_internal) {
      print_children_improved(node, prefix, is_last, depth);
    }
  }

  void print_children_improved(const node_type *node, const std::string &prefix,
                               bool isLast, int depth) const {
    const std::string child_prefix = prefix + (isLast ? "    " : "│   ");
    const int last_index = node->fields.size - 2;

    int null_start = 0;
    bool in_null_sequence = false;

    for (int i = 0; i <= last_index; ++i) {
      bool is_last_child = (i == last_index);

      if (node->child(i) == nullptr) {
        if (!in_null_sequence) {
          null_start = i;
          in_null_sequence = true;
        }

        if (is_last_child && options.show_null_ranges) {
          print_null_range(child_prefix, null_start, i, true);
        }
      } else {
        if (in_null_sequence && options.show_null_ranges) {
          print_null_range(child_prefix, null_start, i - 1, false);
          in_null_sequence = false;
        }

        print_node_improved(node->child(i), child_prefix, is_last_child,
                            depth + 1);
      }
    }
  }

  void print_null_range(const std::string &prefix, int start, int end,
                        bool isLast) const {
    std::cout << prefix << (isLast ? "└── " : "├── ");

    if (options.color_output) {
      std::cout << "\033[2;37m";
    }

    if (start == end) {
      std::cout << "[[NULL #" << start << "]]";
    } else {
      std::cout << "[[NULL #" << start << "-" << end << "]]";
    }

    if (options.color_output) {
      std::cout << "\033[0m";
    }

    std::cout << "\n";
  }

  void print_compact_node(const node_type *node, int depth) const {
    if (node == nullptr) {
      std::cout << "NULL ";
      return;
    }

    std::cout << "[";
    for (int i = 0; i < std::min(3, static_cast<int>(node->fields.size)); ++i) {
      if (i > 0)
        std::cout << ",";
      std::cout << format_value_at(node, i);
    }
    if (node->fields.size > 3) {
      std::cout << "...+" << (node->fields.size - 3);
    }
    std::cout << "] ";

    if (node->fields.is_internal && depth < 2) {
      std::cout << "{ ";
      for (int i = 0; i < std::min(2, static_cast<int>(node->fields.size) - 1);
           ++i) {
        print_compact_node(node->fields.pointers[i], depth + 1);
      }
      std::cout << "} ";
    }
  }
};

// Printable container mixin - extends any WTree container with print methods.
// Works for both set and map containers via is_map_type detection.
template <typename ContainerType>
class PrintableContainer : public ContainerType {
  using wtree_type = typename ContainerType::wtree_type;
  using printer_type = WTreePrinter<wtree_type>;
  using value_type = typename wtree_type::value_type;

  static constexpr bool is_map_type = is_pair_v<value_type>;

  printer_type m_printer;

public:
  using PrintOptions = typename printer_type::PrintOptions;

  // Forward all constructors to the base container
  using ContainerType::ContainerType;

  PrintableContainer() : ContainerType() {}

  PrintableContainer(const PrintOptions &opts)
      : ContainerType(), m_printer(opts) {}

  printer_type &printer() { return m_printer; }
  const printer_type &printer() const { return m_printer; }

  void set_print_options(const PrintOptions &opts) { m_printer.options = opts; }

  void print(const std::string &title = "WTree") const {
    m_printer.print_tree(this->tree()->root(), title);
  }

  void print_compact() const { m_printer.print_compact(this->tree()->root()); }

  // Set-only: print with "WTree Set" title
  template <bool Enabled = !is_map_type>
  typename std::enable_if<Enabled>::type
  print_set(const std::string &title = "WTree Set") const {
    static_assert(!is_map_type,
                  "print_set() is only available for set containers");
    m_printer.print_tree(this->tree()->root(), title);
  }

  // Map-only: print with "WTree Map" title
  template <bool Enabled = is_map_type>
  typename std::enable_if<Enabled>::type
  print_map(const std::string &title = "WTree Map") const {
    static_assert(is_map_type,
                  "print_map() is only available for map containers");
    m_printer.print_tree(this->tree()->root(), title);
  }
};

} // namespace WTreeLib

// Convenience aliases - available when set.hpp / map.hpp are included before
// this header (or after, as long as both are eventually included).
// Use these in a translation unit that includes both headers.
#ifdef _WTREE_SET__H_
namespace WTreeLib {
template <typename Key, int TargetNodeSize = WTREE_TARGET_NODE_BYTES,
          typename Compare = std::less<Key>,
          typename Alloc = std::allocator<Key>>
using PrintableSet =
    PrintableContainer<set<Key, TargetNodeSize, Compare, Alloc>>;
} // namespace WTreeLib
#endif

#ifdef _WTREE_MAP__H_
namespace WTreeLib {
template <
    typename Key, typename Value, int TargetNodeSize = WTREE_TARGET_NODE_BYTES,
    typename Compare = std::less<Key>, typename Alloc = std::allocator<Key>>
using PrintableMap =
    PrintableContainer<map<Key, Value, TargetNodeSize, Compare, Alloc>>;
} // namespace WTreeLib
#endif

#endif
