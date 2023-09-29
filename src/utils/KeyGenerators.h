#ifndef KEYGENERATORS_H
#define KEYGENERATORS_H

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <random>
#include <sys/types.h>

using namespace std;

template <typename T> class BaseGenerator {
public:
  ulong seed;
  ulong max_value; // Max value for int types
  ulong annotated = 0;
  ulong *visited = nullptr; // 540MB for all uint keys (as boolean aparition)
  float mean = -1, std = -1;

  normal_distribution<> *ndist1 = nullptr, *ndist2 = nullptr;
  uniform_int_distribution<T> *udist = nullptr;
  default_random_engine *eng = nullptr;

protected:
  ulong _block = 0;
  ushort _bi = 0;
  normal_distribution<> *_temp_ndist1 = nullptr, *_temp_ndist2 = nullptr;
  uniform_int_distribution<T> *_temp_udist = nullptr;
  default_random_engine *_temp_eng = nullptr;

  void baseInit() {
    if (visited != nullptr)
      delete[] visited;
    visited =
        new ulong[max_value / 64 + 1](); // The parenthesis sets values to 0.
  }

  BaseGenerator(ulong _seed = 0, ulong maxval = 2147483647)
      : seed(_seed), max_value(maxval), annotated(0) {
    assert(maxval <= numeric_limits<T>::max());
    baseInit();
  }

  virtual T operator()() { return T(); };
  virtual void reset(){};

public:
  ~BaseGenerator() {
    delete[] visited;
    if (_temp_ndist1)
      delete _temp_ndist1;
    if (_temp_ndist2)
      delete _temp_ndist2;
    if (_temp_udist)
      delete _temp_udist;
    if (_temp_eng)
      delete _temp_eng;
    if (ndist1)
      delete ndist1;
    if (ndist2)
      delete ndist2;
    if (udist)
      delete udist;
    if (eng)
      delete eng;
  }

  // virtual ~BaseGenerator() { delete[] visited; }

  bool checkKey(const T key) {
    _block = key / 64;
    _bi = key % 64;
    // ulong num = (1l << _bi) & visited[_block];
    // printf("Checking:\n%08lx\n%08lx --- %lu\n", visited[_block], (1l << _bi),
    // num);
    return (1lu << _bi) & visited[_block];
  }

  T getExistentKey(bool erase = false) {
    T key;
    do
      key = (*this)();
    while (!checkKey(key));
    if (erase) {
      visited[_block] ^= (1lu << _bi);
      --annotated;
    }
    return key;
  };

  T getInexistentKey(bool annotate = true) {
    T key;
    do
      key = (*this)();
    while (checkKey(key));
    if (annotate) {
      visited[_block] |= (1lu << _bi);
      ++annotated;
    }
    return key;
  };

  void annotateKey(T val) {
    checkKey(val);
    visited[_block] |= (1lu << _bi);
    ++annotated;
  }

  void unannotateKey(T val) {
    checkKey(val);
    visited[_block] ^= (1lu << _bi);
    --annotated;
  }

  void saveState() {
    if (eng)
      *_temp_eng = *eng;
    if (ndist1)
      *_temp_ndist1 = *ndist1;
    if (ndist2)
      *_temp_ndist2 = *ndist2;
    if (udist)
      *_temp_udist = *udist;
  }

  // Restores the random generator seed to the saved struct state.
  void restoreState() {
    if (_temp_eng)
      *eng = *_temp_eng;
    if (_temp_ndist1)
      *ndist1 = *_temp_ndist1;
    if (_temp_ndist2)
      *ndist2 = *_temp_ndist2;
    if (_temp_udist)
      *udist = *_temp_udist;
  }
};

template class BaseGenerator<int>;
template class BaseGenerator<unsigned int>;
template class BaseGenerator<long>;
template class BaseGenerator<unsigned long>;
template class BaseGenerator<long long>;
template class BaseGenerator<unsigned long long>;

template <typename T> class UniformGenerator : public BaseGenerator<T> {
public:
  UniformGenerator(ulong seed = 0, ulong maxval = 2147483647)
      : BaseGenerator<T>(seed, maxval) {
    this->eng = new default_random_engine(seed);
    this->udist = new uniform_int_distribution<T>(0, maxval);
  }

  T operator()() override { return (*this->udist)(*this->eng); }

  void reset() override {
    delete this->eng;
    delete this->udist;

    this->eng = new default_random_engine(this->seed);
    this->udist = new uniform_int_distribution<T>(0, this->max_value);
  }
};

template <typename T> class GaussGenerator : public BaseGenerator<T> {
public:
  void init() {
    if (this->eng != nullptr)
      delete this->eng;
    this->eng = new default_random_engine(this->seed);

    if (this->ndist1 != nullptr)
      delete this->ndist1;
    this->ndist1 = new normal_distribution<>(this->mean, this->std);
  }

  GaussGenerator(float mean, float std, ulong seed, ulong maxval = 2147483647)
      : BaseGenerator<T>(seed, maxval) {
    this->mean = float(maxval) * mean;
    this->std = float(maxval) * std;
    init();
  }

  T operator()() override {
    double gen = (*this->ndist1)(*this->eng);
    return max(min(gen, (double)this->max_value), 0.0);
  }

  void reset() override {
    this->baseInit();
    init();
  }
};

template <typename T> class BimodalGenerator : public BaseGenerator<T> {
public:
  float mean2, std2;

  void init() {
    if (this->eng != nullptr)
      delete this->eng;
    this->eng = new default_random_engine(this->seed);

    if (this->ndist1 != nullptr)
      delete this->ndist1;
    this->ndist1 = new normal_distribution<>(this->mean, this->std);

    if (this->ndist2 != nullptr)
      delete this->ndist2;
    this->ndist2 = new normal_distribution<>(this->mean2, this->std2);
  }

  BimodalGenerator(float m1, float std1, float m2, float std2, ulong seed,
                   ulong maxval = 2147483647)
      : BaseGenerator<T>(seed, maxval) {

    m1 *= maxval;
    m2 *= maxval;
    std1 = std1 * (maxval >> 1);
    std2 = std2 * (maxval >> 1);

    this->mean = m1;
    this->std = std1;
    this->mean2 = m2;
    this->std2 = std2;

    init();
  }

  T operator()() override {
    double gen;
    if (rand() % 2)
      gen = (*this->ndist1)(*this->eng);
    else
      gen = (*this->ndist2)(*this->eng);
    return max(min(gen, (double)this->max_value), 0.0);
  }

  void reset() override {
    this->baseInit();
    init();
  }
};

template <typename T> class SequentialGenerator : public BaseGenerator<T> {
private:
  ulong temp_currval = 0;

public:
  ulong currval = 0;

  SequentialGenerator(ulong seed = 0, ulong maxval = 2147483647)
      : BaseGenerator<T>(seed, maxval) {
    this->currval = 0;
  }

  T operator()() override {
    this->currval = (this->currval + 1) % this->max_value;
    return this->currval;
  }

  void saveState() override { this->temp_currval = this->currval; }

  void restoreState() override { this->currval = this->temp_currval; }

  void reset() override {
    *this = SequentialGenerator(this->seed, this->max_value);
  }
};

#endif