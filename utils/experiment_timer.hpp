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

#ifndef _EXPTIMER_H_
#define _EXPTIMER_H_

#include <ctime>

class ExperimentTimer {
protected:
  clock_t _start, _end;

public:
  clock_t total = 0;
  unsigned long count = 0;

  ExperimentTimer() = default;

  void start() { _start = clock(); }

  void cstart() {
    clear();
    _start = clock();
  }

  void end() {
    _end = clock();
    total += _end - _start;
    ++count;
  }

  clock_t get() const { return total; }

  double toSecs() const { return (double)total / CLOCKS_PER_SEC; }

  void clear() {
    total = 0;
    count = 0;
  }

  void clear_all() {
    clear();
    count = 0;
  }
};
#endif