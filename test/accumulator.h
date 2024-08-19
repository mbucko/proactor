#ifndef ACCUMULATOR_H
#define ACCUMULATOR_H

#include <cstdint>

class Accumulator {
 public:
  Accumulator() = delete;
  Accumulator(std::uint32_t initial_value) : value_(initial_value) {}

  void add(uint32_t value) { value_ += value; }

  std::uint32_t get() { return value_; }

 private:
  std::uint32_t value_;
};

#endif  // ACCUMULATOR_H