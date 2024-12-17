#ifndef __WTREE_DEFS__
#define __WTREE_DEFS__

#define WT_SINGLE_SIZE_VERSION

#ifndef WTREE_TARGET_NODE_BYTES
#define WTREE_TARGET_NODE_BYTES 256
#endif

#ifndef WTREE_TARGET_INITIAL_BYTES
#define WTREE_TARGET_INITIAL_BYTES 40
#endif

enum WTreeRulesAssumptions {
  slide_over_split = true,
  slide_to_right_first = true,
  split_to_right_first = true,
};

// Define a function for the next capacity of a leaf node.
// Current keeps n/k >= 75%.
#define NEW_SIZE(current_size) current_size + ((current_size) >> 2)

// N ->S + S/4
// N ->S 5/4
// N 4/5 -> S
#define LAST_GROWTH_LIMIT(k) (k / 5) << 2

#define SAFE_NEW_SIZE(current_size, limit, k)                                  \
  current_size < limit ? NEW_SIZE(current_size) : k

#endif