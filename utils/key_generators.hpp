#ifndef KEYGENERATORS_H
#define KEYGENERATORS_H

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <random>
#include <sys/types.h>
#include <type_traits>

/**
Note that, for uint keys, the bit-vector uses 540MB.
*/
template <typename T> class BaseGenerator {
  using self_type = BaseGenerator<T>;

public:
  ulong seed;
  size_t annotated;
  T max_value;
  T min_value;
  ulong *visited = nullptr;

protected:
  size_t _block = 0;
  ushort _block_index = 0;

  explicit BaseGenerator(ulong _seed = 0,
                         T max_val = std::numeric_limits<T>::max(),
                         T min_val = std::numeric_limits<T>::min())
      : seed(_seed), annotated(0), max_value(max_val), min_value(min_val) {
    assert(max_val <= std::numeric_limits<T>::max());
    base_init();
  }

  // Copy assignment
  BaseGenerator &operator=(const BaseGenerator &other) {
    if (this != &other) {
      seed = other.seed;
      min_value = other.min_value;
      max_value = other.max_value;
      annotated = other.annotated;

      delete[] visited;
      visited = new ulong[get_bitvector_size()];
      std::copy(other.visited, other.visited + get_bitvector_size(), visited);
    }
    return *this;
  }

  size_t get_bitvector_size() {
    return (((size_t)max_value) >> 6) + 1 - (min_value >> 6);
  }

  void base_init() {
    delete[] visited; // Safe for nullptrs.
    visited =
        new ulong[get_bitvector_size()](); // The parenthesis sets values to 0.
  }

  void reset() {
    annotated = 0;
    _block = 0;
    _block_index = 0;
    base_init();
  };

public:
  // Must override these functions on derived classes.

  // A virtual destructor is always callled from derived classes.
  virtual ~BaseGenerator() { delete[] visited; }
  virtual T operator()() { return T(); };
  virtual void save_state() = 0;
  virtual void restore_state() = 0;

  // === Common use functions ===

  bool check_key(const T key) {
    // Correct bounds for a correct position.
    T normalized_key = key;
    if (min_value != 0) {
      normalized_key -= min_value;
    }
    _block = normalized_key / 64;
    _block_index = normalized_key % 64;
    assert(_block < get_bitvector_size());
    return (1LU << _block_index) & visited[_block];
  }

  T get_existing_key(bool erase = false) {
    T key;
    do
      key = (*this)();
    while (!check_key(key));
    if (erase) {
      visited[_block] ^= (1LU << _block_index);
      --annotated;
    }
    return key;
  };

  T try_get_existing_key(bool erase = false) {
    short repetitions = 0;
    T key;
    do {

      key = (*this)();
      if (++repetitions == 100)
        break;
    } while (!check_key(key));
    if (erase) {
      visited[_block] ^= (1LU << _block_index);
      --annotated;
    }
    return key;
  };

  T get_absent_key(bool annotate = true) {
    T key;
    do
      key = (*this)();
    while (check_key(key));
    if (annotate) {
      visited[_block] |= (1LU << _block_index);
      ++annotated;
    }
    return key;
  };

  void tag_key(T val) {
    check_key(val);
    visited[_block] |= (1LU << _block_index);
    ++annotated;
  }

  void untag_key(T val) {
    check_key(val);
    visited[_block] ^= (1LU << _block_index);
    --annotated;
  }
};

template <typename IntType>
class UniformGenerator : public BaseGenerator<IntType> {
protected:
  static_assert(std::is_integral_v<IntType>);

  using self_type = UniformGenerator<IntType>;
  using engine_type = std::default_random_engine;
  using distribution_type = std::uniform_int_distribution<IntType>;

  engine_type *engine = nullptr;
  distribution_type *distribution = nullptr;
  self_type *_backup = nullptr;

public:
  explicit UniformGenerator(
      size_t seed = 0, IntType max_val = std::numeric_limits<IntType>::max(),
      IntType min_val = std::numeric_limits<IntType>::min())
      : BaseGenerator<IntType>(seed, max_val, min_val) {
    engine = new std::default_random_engine(seed);
    distribution = new distribution_type(this->min_value, this->max_value);
  }

  ~UniformGenerator() {
    delete engine;
    delete distribution;
    delete _backup;
  };

  // Copy constructor
  UniformGenerator(const UniformGenerator &other)
      : BaseGenerator<IntType>(other.seed, other.max_value, other.min_value) {
    this->annotated = other.annotated;
    const size_t vsize = this->get_bitvector_size();
    std::copy(other.visited, other.visited + vsize, this->visited);

    engine = new engine_type(*other.engine);
    distribution = new distribution_type(*other.distribution);
  }

  // Copy assignment
  UniformGenerator &operator=(const UniformGenerator &other) {
    if (this != &other) {
      BaseGenerator<IntType>::operator=(other);
      delete engine;
      delete distribution;
      engine = new engine_type(*other.engine);
      distribution = new distribution_type(*other.distribution);
    }
    return *this;
  }

  IntType operator()() override final {
    return (*this->distribution)(*this->engine);
  }

  void save_state() final { _backup = new self_type(*this); }
  void restore_state() final {
    if (_backup == nullptr)
      return;
    *this = *_backup;
  }

  void reset() {
    delete engine;
    delete distribution;

    engine = new engine_type(this->seed);
    distribution = new distribution_type(this->min_value, this->max_value);
    BaseGenerator<IntType>::base_init();
  }
};

template <typename IntType>
class GaussianGenerator : public BaseGenerator<IntType> {
protected:
  static_assert(std::is_integral<IntType>());
  using self_type = GaussianGenerator<IntType>;
  using engine_type = std::default_random_engine;
  using _generation_type = double;
  using _distribution_type = std::normal_distribution<_generation_type>;

  engine_type *engine = nullptr;
  _distribution_type *distribution = nullptr;
  self_type *_backup = nullptr;

  IntType mean;
  IntType std;

private:
  static IntType calculate_max_value(IntType mean, IntType std) {
    constexpr IntType type_max = std::numeric_limits<IntType>::max();
    constexpr IntType five = 5;

    // Check for overflow: if mean > type_max - 5*std, then mean + 5*std would
    // overflow
    if (std > (type_max - mean) / five) {
      return type_max;
    }
    return mean + (five * std);
  }

  static IntType calculate_min_value(IntType mean, IntType std) {
    constexpr IntType type_min = std::numeric_limits<IntType>::min();
    constexpr IntType five = 5;

    if constexpr (std::is_unsigned_v<IntType>) {
      // For unsigned types, check if 5*std > mean to avoid underflow
      if (five * std > mean) {
        return 0;
      }
      return mean - (five * std);
    } else {
      // For signed types, check for underflow
      // We need to be careful with the arithmetic to avoid overflow in the
      // check itself If mean - type_min < 5*std, then mean - 5*std would
      // underflow
      if (mean < (type_min + five * std)) {
        return type_min;
      }
      return mean - (five * std);
    }
  }

public:
  explicit GaussianGenerator(size_t seed = 0, IntType mean = 0, IntType std = 1)
      : BaseGenerator<IntType>(
            seed, calculate_max_value(mean, std), // max_value: mean + 5*std
            calculate_min_value(mean, std)),      // min_value: mean - 5*std
        mean(mean), std(std) {
    engine = new engine_type(seed);
    distribution =
        new _distribution_type(0.0, 1.0); // Always standard normal N(0,1)
  }

  ~GaussianGenerator() {
    delete engine;
    delete distribution;
    delete _backup;
  };

  // Copy constructor
  GaussianGenerator(const GaussianGenerator &other)
      : BaseGenerator<IntType>(other.seed, other.max_value, other.min_value),
        mean(other.mean), std(other.std) {
    engine = new engine_type(*other.engine);
    distribution = new _distribution_type(*other.distribution);
  }

  // Copy assignment
  GaussianGenerator &operator=(const GaussianGenerator &other) {
    if (this != &other) {
      BaseGenerator<IntType>::operator=(other);
      mean = other.mean;
      std = other.std;
      delete engine;
      delete distribution;
      engine = new engine_type(*other.engine);
      distribution = new _distribution_type(*other.distribution);
    }
    return *this;
  }

  IntType operator()() override {
    const _generation_type sample = (*distribution)(*engine); // ~ N(0,1)
    const _generation_type scaled = (sample * std) + mean;
    IntType rounded = static_cast<IntType>(std::round(scaled));
    return std::clamp(rounded, this->min_value, this->max_value);
  }

  IntType get_mean() { return this->mean; }
  IntType get_std() { return this->std; }

  void save_state() final { _backup = new self_type(*this); }
  void restore_state() final {
    if (_backup == nullptr)
      return;
    *this = *_backup;
  }

  void reset() {
    delete engine;
    delete distribution;

    engine = new engine_type(this->seed);
    distribution = new _distribution_type(0, 1);
    BaseGenerator<IntType>::base_init();
  }
};

// template <typename T> class BimodalGenerator : public BaseGenerator<T> {
// public:
//   float mean2, std2;

//   void init() {
//     if (this->eng != nullptr)
//       delete this->eng;
//     this->eng = new default_random_engine(this->seed);

//     if (this->ndist1 != nullptr)
//       delete this->ndist1;
//     this->ndist1 = new normal_distribution<>(this->mean, this->std);

//     if (this->ndist2 != nullptr)
//       delete this->ndist2;
//     this->ndist2 = new normal_distribution<>(this->mean2, this->std2);
//   }

//   BimodalGenerator(float m1, float std1, float m2, float std2, ulong seed,
//                    ulong maxval = 2147483647)
//       : BaseGenerator<T>(seed, maxval) {

//     m1 *= maxval;
//     m2 *= maxval;
//     std1 = std1 * (maxval >> 1);
//     std2 = std2 * (maxval >> 1);

//     this->mean = m1;
//     this->std = std1;
//     this->mean2 = m2;
//     this->std2 = std2;

//     init();
//   }

//   T operator()() override {
//     double gen;
//     if (rand() % 2)
//       gen = (*this->ndist1)(*this->eng);
//     else
//       gen = (*this->ndist2)(*this->eng);
//     return max(min(gen, (double)this->max_value), 0.0);
//   }

//   void reset() override {
//     this->base_init();
//     init();
//   }
// };

// template <typename T> class SequentialGenerator : public BaseGenerator<T> {
// private:
//   ulong temp_currval = 0;

// public:
//   ulong currval = 0;

//   SequentialGenerator(ulong seed = 0, ulong maxval = 2147483647)
//       : BaseGenerator<T>(seed, maxval) {
//     this->currval = 0;
//   }

//   T operator()() override {
//     this->currval = (this->currval + 1) % this->max_value;
//     return this->currval;
//   }

//   void saveState() { this->temp_currval = this->currval; }

//   void restoreState() { this->currval = this->temp_currval; }

//   void reset() override {
//     *this = SequentialGenerator(this->seed, this->max_value);
//   }
// };

#endif