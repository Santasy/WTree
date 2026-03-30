#include "../include/wtree/map.hpp"
#include <iostream>
#include <string>

using namespace std;
using namespace WTreeLib;

#ifdef __BASIC_TEST_PAIR
bool test_pair();
#endif

struct DummyStruct {
  int x, y;
};
bool test_using_DummyStruct();

class DummyClass {
public:
  int origin, destiny;
  string name;

  DummyClass(int origin, int destiny, string name)
      : origin(origin), destiny(destiny), name(name) {
    cout << "Constructing dummy class: " << name << '\n';
  }

  void printData() const {
    cout << "Name: " << name << "\n";
    cout << "Origin: " << origin << "\n";
    cout << "Destiny: " << destiny << "\n";
  }
};
bool test_using_DummyClass();
bool test_using_DummyClass_pointers();

int main() {
  test_using_DummyStruct();
  test_using_DummyClass();
  test_using_DummyClass_pointers();

  {
    WTreeLib::map<int, string> map;
    map[1] = "test";
    WTreeLib::map<int, string> map2 = map; // Does copy work?
    cout << "YES!!\n";
  } // Does destruction work without double-free?

  cout << "\n\n===== Done with all examples =====\n";
  return 0;
}

#ifdef __BASIC_TEST_PAIR
bool test_pair() {
  using map_type = WTreeLib::map<uint, std::pair<int, int>>;
  map_type wmap;
  wmap.insert({42, {1, 2}});
  wmap.insert({43, {1, 5}});
  wmap.insert({45, {1, 6}});

  map_type::iterator res = wmap.find(41);
  cout << res.key() << '\n';
  cout << ((res == wmap.end()) ? "is end.\n" : "is not end.") << '\n';

  res = wmap.find(43);
  cout << res.key() << '\n';
  cout << ((res == wmap.end()) ? "is end.\n" : "is not end.") << '\n';

  int erase_result = wmap.erase(33);
  cout << "Removed: " << (erase_result > 0 ? "Yes" : "No") << '\n';

  // A lot of keys:

  for (uint i = 0; i < 1000; ++i) {
    uint val = 50 + (i * 20);
    wmap.insert({val, std::pair(10 * i, val * 10)});
  }

  for (uint i = 0; i < 1000; ++i) {
    uint val = 50 + (i * 20);
    wmap.insert({val, std::pair(10 * i, val * 10)});
  }

  return true;
}
#endif

bool test_using_DummyStruct() {
  using map_type = WTreeLib::map<uint, DummyStruct>;
  map_type wmap;

  cout << "\n\n=== Testing DummyStruct values ===\n";

  wmap.insert({42, DummyStruct{1, 2}});
  wmap.insert({43, DummyStruct{1, 5}});
  wmap.insert({45, DummyStruct{1, 6}});

  // Test finding non-existent key
  map_type::iterator res = wmap.find(41);
  cout << "Searching for key 41: ";
  cout << ((res == wmap.end()) ? "not found (correct)\n"
                               : "found (unexpected)\n");

  // Test finding existing key and modification
  res = wmap.find(43);
  if (res != wmap.end()) {
    cout << "Found key 43:\n";
    cout << res->second.x + res->second.y << '\n';

    // Test modification through reference
    DummyStruct &structRef = res->second;
    cout << "Modifying through reference...\n";
    structRef.x = 999;
    structRef.y = 888;
    cout << "After modification:\n";
    cout << structRef.x + structRef.y << '\n';

    // Verify the modification persisted
    auto verifyRes = wmap.find(43);
    cout << "Verification - value in btree:\n";
    cout << verifyRes->second.x + verifyRes->second.y << '\n';
  }

  // Test erase
  int erase_result = wmap.erase(33);
  cout << "Removed non-existent key 33: " << (erase_result > 0 ? "Yes" : "No")
       << '\n';

  // Insert many keys
  cout << "Inserting 1000 keys...\n";
  for (uint i = 0; i < 1000; ++i) {
    uint val = 50 + (i * 20);
    wmap.insert({val, DummyStruct{10 * (int)i, (int)val * 10}});
  }

  // Verify some insertions
  for (uint i = 0; i < 5; ++i) {
    uint val = 50 + (i * 20);
    auto found = wmap.find(val);
    if (found != wmap.end()) {
      cout << "Key " << val << ": ";
      cout << found->second.x + found->second.y << '\n';
    }
  }

  return true;
}

bool test_using_DummyClass() {
  using map_type = WTreeLib::map<uint, DummyClass>;
  map_type wmap;

  cout << "\n\n=== Testing DummyClass values ===\n";

  wmap.insert({42, DummyClass(1, 2, "A")});
  wmap.insert({43, DummyClass(1, 5, "B")});
  wmap.insert({45, DummyClass(1, 6, "C")});

  // Test finding non-existent key
  map_type::iterator res = wmap.find(41);
  cout << "Searching for key 41: ";
  cout << ((res == wmap.end()) ? "not found (correct)\n"
                               : "found (unexpected)\n");

  // Test finding existing key and modification
  res = wmap.find(43);
  if (res != wmap.end()) {
    cout << "Found key 43:\n";
    res->second.printData();

    // Test modification through reference
    DummyClass &classRef = res->second;
    cout << "Modifying through reference...\n";
    classRef.name = "Modified_B";
    cout << "After modification:\n";
    classRef.printData();

    // Verify the modification persisted
    auto verifyRes = wmap.find(43);
    cout << "Verification - value in btree:\n";
    verifyRes->second.printData();
  }

  // Test erase
  int erase_result = wmap.erase(33);
  cout << "Removed non-existent key 33: " << (erase_result > 0 ? "Yes" : "No")
       << '\n';

  // Test successful erase
  cout << "Before erasing key 42, size check:\n";
  auto beforeErase = wmap.find(42);
  if (beforeErase != wmap.end()) {
    cout << "Key 42 exists\n";
  }

  erase_result = wmap.erase(42);
  cout << "Removed existing key 42: " << (erase_result > 0 ? "Yes" : "No")
       << '\n';

  auto afterErase = wmap.find(42);
  cout << "After erase, key 42: ";
  cout << ((afterErase == wmap.end()) ? "not found (correct)\n"
                                      : "still found (error)\n");

  return true;
}

bool test_using_DummyClass_pointers() {
  using map_type = WTreeLib::map<uint, DummyClass *>;

  cout << "\n\n=== Testing DummyClass* (pointer) values ===\n";
  map_type wmap;

  // Create DummyClass objects and insert pointers
  DummyClass *obj1 = new DummyClass(10, 20, "PointerA");
  DummyClass *obj2 = new DummyClass(30, 40, "PointerB");
  DummyClass *obj3 = new DummyClass(50, 60, "PointerC");

  wmap.insert({100, obj1});
  wmap.insert({200, obj2});
  wmap.insert({300, obj3});

  // Test finding and accessing through pointer
  map_type::iterator res = wmap.find(200);
  if (res != wmap.end()) {
    cout << "Found key 200, object data:\n";
    res->second->printData();

    // Test modification through pointer reference
    DummyClass *&ptrRef = res->second;
    cout << "Modifying object through pointer reference...\n";
    ptrRef->name = "Modified_PointerB";
    cout << "After modification:\n";
    ptrRef->printData();

    // Verify the modification persisted in original object
    cout << "Verification - original obj2:\n";
    obj2->printData();

    // Verify the modification persisted in btree
    auto verifyRes = wmap.find(200);
    cout << "Verification - value in btree:\n";
    verifyRes->second->printData();
  }

  // Test pointer reassignment
  DummyClass *newObj = new DummyClass(111, 222, "NewPointer");
  res = wmap.find(100);
  if (res != wmap.end()) {
    cout << "Before pointer reassignment:\n";
    res->second->printData();

    DummyClass *oldPtr = res->second;

    // Reassign pointer
    res->second = newObj;
    cout << "After pointer reassignment:\n";
    res->second->printData();

    delete oldPtr;
  }

  // Test finding non-existent key
  res = wmap.find(999);
  cout << "Searching for key 999: ";
  cout << ((res == wmap.end()) ? "not found (correct)\n"
                               : "found (unexpected)\n");

  // Clean up remaining objects
  for (auto it = wmap.begin(); it != wmap.end(); ++it) {
    delete it->second;
  }

  cout << "Pointer test cleanup complete.\n";
  return true;
}