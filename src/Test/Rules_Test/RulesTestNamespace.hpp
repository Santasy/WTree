#ifndef RULESTESTNAMESPACE_H
#define RULESTESTNAMESPACE_H

#include <bits/stdc++.h>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <limits>
#include <sstream>
#include <sys/types.h>

#include "../../utils/KeyGenerators.h"
#include "../../wtree/WTree.h"

namespace RulesTestNamespace {

using namespace std;
using namespace WTreeLib;

template <typename T> bool Test_MassiveInsertDelete(ulong k);
bool Test_InstantiationType();

bool Test_AllRules(bool pause_between, bool print_tree);

} // namespace RulesTestNamespace

#endif