#include "WTree.h"
#include <algorithm>
#include <cassert>
#include <utility>

using namespace std;
using namespace WTreeLib;

/**
  Insert all keys in 'keys'.
*/
template <typename T> vector<bool> WTree<T>::insert(const vector<T> &keys) {
  vector<bool> results;
  for (T key : keys)
    results.push_back(_insert(key, nullptr, root, 0));
  return results;
}

/**
  Insert all keys in 'keys'.
*/
template <typename T>
vector<bool> WTree<T>::insert(const T *&keys, unsigned int n) {
  vector<bool> results;
  for (unsigned int i = 0; i < n; ++i)
    results.push_back(_insert(keys[i], nullptr, root, 0));
  return results;
}

/**
  Insert key 'key' in the tree starting at 'u'. Due to some rules applied, is
  required to keep track of the previus ascendent node 'p'
  and the index 'p_lwb' of 'u' inside 'p'.

  Returns true if the key was inserted.
*/
template <typename T>
bool WTree<T>::_insert(T key, t_node *p, t_node *u, t_field p_lwb) {
  while (u != nullptr) {
    t_field lwb;
    if (u->_fields.n < k) {
      // R2: There is space left
      bool ans = u->insert(key, k); // O(k)
      size += ans;
      return ans;
    }

    // u->n == k

    // R1: Swap laterals if needed
    if (key < u->_fields.keys[0]) {
      swap(key, u->_fields.keys[0]);
      lwb = 0;
    } else if (key > u->_fields.keys[k - 1]) {
      swap(key, u->_fields.keys[k - 1]);
      lwb = k - 2; // This is garanteed to be in the last valid ptr
    } else {
      for (lwb = 0; key > u->_fields.keys[lwb]; ++lwb)
        ;

      if (u->_fields.keys[lwb] == key)
        return false;
      --lwb;
    }

    if (u->_fields.isInternal == false) {
      // First try to balance, otherwise make 'u' internal

      // R4: Slide to side
      if (p != nullptr) [[likely]] {
        if (trySlide(key, p, p_lwb, u, lwb) == true) {
          ++size;
          return true;
        }

        if (trySplit(key, p, p_lwb, u, lwb) == true) {
          ++size;
          return true;
        }
      }

      // Else: must connect to a new node
      p = u;
      p->setAsInternal(k);
      p_lwb = lwb;
      break;
    }
    // if 'u' isInternal, the next 'u' can exist or be null.
    p = u;
    u = u->_fields.pointers[lwb];
    p_lwb = lwb;
  }
  // R5: Create the node
  ++size;
  u = new t_node(k);
  u->_fields.keys[0] = key;
  ++u->_fields.n;

  if (p != nullptr) {
    p->_fields.pointers[p_lwb] = u;
  } else {
    root = u;
    root->setAsInternal(k); // TODO: Test effectiveness
  }
  return true;
}

/**
  Tries to slide a key from the node 'u' to some adyascent node 'v'
  that exists and has space left.
  Returns true if the process was successful.
*/
template <typename T>
bool WTree<T>::trySlide(T key, t_node *p, const t_field p_lwb, t_node *u,
                        const t_field lwb) {
  assert(p != nullptr);
  assert(u != nullptr && u->_fields.isInternal == false);
  assert(u->_fields.keys[0] < key &&
         key < u->_fields.keys[k - 1]); // This is garanteed

  const bool rightIsValid = (p_lwb < k - 2) &&
                            (p->_fields.pointers[p_lwb + 1] != nullptr) &&
                            (p->_fields.pointers[p_lwb + 1]->_fields.n < k);
  if (rightIsValid) {
    t_node *v = p->_fields.pointers[p_lwb + 1];
    const ushort u_end = ((k + v->_fields.n) >> 1) + 1;
    v->reserve(min((ushort)(u_end + v->_fields.n + (u_end >> 1)), k));

    if (u_end == k) { // Move just one key
      move_backward(v->_fields.keys, v->_fields.keys + v->_fields.n,
                    v->_fields.keys + v->_fields.n + 1);
      v->_fields.keys[0] = p->_fields.keys[p_lwb + 1];
      ++v->_fields.n;

      p->_fields.keys[p_lwb + 1] = u->_fields.keys[k - 1];

      // Insert key in u
      move_backward(u->_fields.keys + lwb + 1, u->_fields.keys + k - 1,
                    u->_fields.keys + k);
      u->_fields.keys[lwb + 1] = key;
      return true;

    } else {
      const ushort v_end = k - u_end + 1;
      // Move keys in v node to the right
      move_backward(v->_fields.keys, v->_fields.keys + v->_fields.n,
                    v->_fields.keys + v->_fields.n + v_end);

      v->_fields.keys[v_end - 2] = u->_fields.keys[k - 1];
      v->_fields.keys[v_end - 1] = p->_fields.keys[p_lwb + 1];
      v->_fields.n += v_end;

      // Insert key in u
      move_backward(u->_fields.keys + lwb + 1, u->_fields.keys + k - 1,
                    u->_fields.keys + k);
      u->_fields.keys[lwb + 1] = key;

      // Move keys from u to v
      move(u->_fields.keys + u_end + 1, u->_fields.keys + k, v->_fields.keys);

      // Corrects in p
      p->_fields.keys[p_lwb + 1] = u->_fields.keys[u_end];
      u->_fields.n = u_end;

      return true;
    }
  }

  const bool leftIsValid = (p_lwb > 0) &&
                           (p->_fields.pointers[p_lwb - 1] != nullptr) &&
                           (p->_fields.pointers[p_lwb - 1]->_fields.n < k);
  if (leftIsValid) {
    t_node *v = p->_fields.pointers[p_lwb - 1];
    ushort u_start = 1 + ((k + v->_fields.n) >> 1); // First is similar to u_end
    v->reserve(min((ushort)(u_start + v->_fields.n + (u_start >> 1)), k));
    u_start = k - u_start;

    v->_fields.keys[v->_fields.n] = p->_fields.keys[p_lwb];
    // ++v.n is postponed

    if (u_start == 0) { // Move just one key
      ++(v->_fields.n);
      p->_fields.keys[p_lwb] = u->_fields.keys[0];

      // Insert key in u
      move(u->_fields.keys + 1, u->_fields.keys + lwb + 1, u->_fields.keys);
      u->_fields.keys[lwb] = key;
      return true;
    }

    v->_fields.keys[v->_fields.n + 1] = u->_fields.keys[0];
    // another ++v.n is postponed

    // Insert key in u
    move(u->_fields.keys + 1, u->_fields.keys + lwb + 1, u->_fields.keys);
    u->_fields.keys[lwb] = key;

    p->_fields.keys[p_lwb] = u->_fields.keys[u_start - 1];

    move(u->_fields.keys, u->_fields.keys + u_start - 1,
         v->_fields.keys + v->_fields.n + 2);
    v->_fields.n += u_start + 1; // +2 added for the top and u.keys[o]

    // Shift u keys to the left
    move(u->_fields.keys + u_start, u->_fields.keys + k, u->_fields.keys);
    u->_fields.n -= u_start;

    return true;
  }

  return false;
}

/**
  Tries to split the node 'u' in two only if an adyacent node is available.
  Returns true if the process was successful.S
*/
template <typename T>
bool WTree<T>::trySplit(T key, t_node *p, const t_field p_lwb, t_node *u,
                        const t_field lwb) {
  assert(p != nullptr);
  assert(u != nullptr && u->_fields.isInternal == false);
  assert(u->_fields.keys[0] < key &&
         key < u->_fields.keys[k - 1]); // This is garanteed

  const bool rightIsValid =
      (p_lwb < k - 2) && (p->_fields.pointers[p_lwb + 1] == nullptr);
  if (rightIsValid) {
    const ushort u_end = 1 + (k >> 1);
    t_node *v = new t_node(k, k);
    p->_fields.pointers[p_lwb + 1] = v;

    v->_fields.keys[k - u_end] = p->_fields.keys[p_lwb + 1]; // topval
    v->_fields.keys[k - u_end - 1] = u->_fields.keys[k - 1]; // rightmost

    // insert value in u
    move_backward(u->_fields.keys + lwb + 1, u->_fields.keys + k - 1,
                  u->_fields.keys + k);
    u->_fields.keys[lwb + 1] = key;
    p->_fields.keys[p_lwb + 1] =
        u->_fields.keys[u_end]; // Must be after the shifts

    move(u->_fields.keys + u_end + 1, u->_fields.keys + k, v->_fields.keys);
    v->_fields.n = k - u_end + 1; // +2 because of topval
    u->_fields.n = u_end;

    return true;
  }

  const bool leftIsValid =
      (p_lwb > 0) && (p->_fields.pointers[p_lwb - 1] == nullptr);
  if (leftIsValid) {
    const ushort u_start = k - 1 - (k >> 1);
    t_node *v = new t_node(k, k);
    p->_fields.pointers[p_lwb - 1] = v;

    v->_fields.keys[0] = p->_fields.keys[p_lwb];
    v->_fields.keys[1] = u->_fields.keys[0];

    // Insert key in u
    move(u->_fields.keys + 1, u->_fields.keys + lwb + 1, u->_fields.keys);
    u->_fields.keys[lwb] = key;

    p->_fields.keys[p_lwb] = u->_fields.keys[u_start - 1];
    // Move keys from u to v
    move(u->_fields.keys, u->_fields.keys + u_start - 1, v->_fields.keys + 2);
    v->_fields.n = u_start + 1; // +2 for topval and leftmost

    // Shift keys in u
    move(u->_fields.keys + u_start, u->_fields.keys + k, u->_fields.keys);
    u->_fields.n -= u_start;
    return true;
  }

  return false;
}