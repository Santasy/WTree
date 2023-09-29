#ifndef EXPTIMER_H
#define EXPTIMER_H

#include <ctime>

struct ExperimentTimer {
  clock_t total = 0, _start, _end;
  unsigned long count = 0;

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

  unsigned long get() { return total; }

  double toSecs() { return double(total) / CLOCKS_PER_SEC; }

  void clear() {
    total = 0;
    count = 0;
  }
};
#endif