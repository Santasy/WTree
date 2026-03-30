#include <iostream>
#include <string>

#include "../include/wtree/map.hpp"
#include "../include/wtree/optional/print.hpp"

using namespace std;

using WTreeLib::map;

int main() {
  map<string, int> bmap;
  bmap["1000"] = 100;
  bmap["2000"] = 100;
  bmap["3000"] = 100;

  cout << "Original map:\n";
  for (auto &item : bmap) {
    cout << item.first << " ___ " << item.second << '\n';
  }

  return 0;
}