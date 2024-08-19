#ifndef PROACTOR_H
#define PROACTOR_H

#include <cstdint>
#include <functional>
#include <iostream>
#include <type_traits>
#include <utility>

#include "proactorpartition.h"

template <typename KEY, typename HASH_POLICY, std::size_t N_PARTITIONS,
          typename COMPUTABLE>
class Proactor {
 private:
  static_assert(N_PARTITIONS > 0, "N_PARTITIONS must be greater than 0");
  using Partition = ProactorPartition<COMPUTABLE>;

 public:
  template <typename... Args>
  Proactor(std::size_t capacity, Args&&... args) {
    for (std::size_t i = 0; i < N_PARTITIONS; ++i) {
      new (&partitions_[i]) Partition(capacity, i, std::forward<Args>(args)...);
    }
  }

  ~Proactor() {
    for (std::size_t i = 0; i < N_PARTITIONS; ++i) {
      reinterpret_cast<COMPUTABLE*>(&partitions_[i])->~COMPUTABLE();
    }
  }

  template <typename Callback, typename RetType, typename... Args>
  bool process(const KEY& key, RetType (COMPUTABLE::*func)(Args...),
               Callback&& callback, Args... args) {
    const std::size_t index = HASH_POLICY{}(key) % N_PARTITIONS;
    Partition* partition = reinterpret_cast<Partition*>(&partitions_[index]);
    return partition->process(func, std::forward<Callback>(callback),
                              std::forward<Args>(args)...);
  }

  void stop() noexcept {
    for (std::size_t i = 0; i < N_PARTITIONS; ++i) {
      reinterpret_cast<Partition*>(&partitions_[i])->stop();
    }
  }

 private:
  // Couldn't make std::array<COMPUTABLE> ctor work.
  alignas(Partition) char partitions_[N_PARTITIONS][sizeof(Partition)];

  template <typename T, typename K>
  struct hash_policy_checks {
    static constexpr bool is_callable = std::is_invocable_v<T, K>;
    static constexpr bool returns_size_t =
        std::is_same_v<std::invoke_result_t<T, K>, std::size_t>;
  };

  // In your Proactor class:
  static_assert(hash_policy_checks<HASH_POLICY, KEY>::is_callable,
                "HASH_POLICY must be callable with KEY");
  static_assert(hash_policy_checks<HASH_POLICY, KEY>::returns_size_t,
                "HASH_POLICY must return std::size_t");
};

#endif  // PROACTOR_H