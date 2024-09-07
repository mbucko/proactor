#ifndef ACCUMULATOR_H
#define ACCUMULATOR_H

#include <cassert>
#include <cstdint>
#include <memory>

namespace mbucko_test {

class Accumulator {
 public:
  Accumulator() = delete;
  Accumulator(const std::unique_ptr<uint32_t> &initial_value,
              uint32_t init_value2)
      : value_(*initial_value + init_value2) {}

  void add(uint32_t value) { value_ += value; }

  uint32_t get() const { return value_; }

 private:
  uint32_t value_;
};

}  // namespace mbucko_test

#endif  // ACCUMULATOR_H