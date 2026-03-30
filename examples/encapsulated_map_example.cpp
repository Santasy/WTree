#include "../include/wtree/map.hpp"
#include <iostream>
#include <string>

using namespace std;
using namespace WTreeLib;

using WTreeLib::map;

struct StructWithMap {
  map<int, int> wmap;
};

template <typename K, typename V>
void init_struct(StructWithMap &s, map<K, V> m) {
  s.wmap = m;
}

template <typename K, typename V> void add_dummy_value(map<K, V> &wmap) {
  wmap[100] = 1000;
}

int main() {
  map<int, int> wmap;
  wmap[10] = 100;

  StructWithMap smap;
  init_struct(smap, wmap);

  add_dummy_value(wmap);

  cout << "From struct:\n";
  for (auto &item : smap.wmap) {
    cout << item.first << " ___ " << item.second << '\n';
  }

  cout << "Original map:\n";
  for (auto &item : wmap) {
    cout << item.first << " ___ " << item.second << '\n';
  }

  return 0;
}