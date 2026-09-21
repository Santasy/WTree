/*
 * Copyright (c) 2026 Sebastián Pacheco Cáceres
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _WTREE_INDEX__H_
#define _WTREE_INDEX__H_

// This file is the entry point of all fundamental WTree
// classes and functions implementations.

// See available features for the code base, like additional debug flags and
// default size parameters and thresholds.
#include "traits.hpp"

// === Main WTree Structure Header ===
#include "tree.hpp"

// === Implementations ===
#include "node.tpp"

#include "iterator.tpp"

#include "node_manager.tpp"

#include "locator.tpp"

#include "tree.tpp"

// For container's declarations see containers.hpp .

// Use this namespace for all classes and functionalities.
namespace WTreeLib {}

#endif