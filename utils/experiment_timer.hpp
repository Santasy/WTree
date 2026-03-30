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