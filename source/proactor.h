#ifndef PROACTOR_H
#define PROACTOR_H

#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

#include "ProactorPartition.h"

template <typename KEY, typename HASH_POLICY, std::size_t N_PARTITIONS,
          typename COMPUTABLE>
class Proactor {
 private:
  using Partition = ProactorPartition<COMPUTABLE>;

 public:
  template <typename... Args>
  Proactor(std::size_t capacity, const Args&... args) : hash_policy() {
    static_assert(
        std::is_constructible_v<Partition, std::size_t, std::size_t, Args...>,
        "Arguments do not match Partition constructor");
    for (std::size_t i = 0; i < N_PARTITIONS; ++i) {
      new (&partitions_[i]) Partition(capacity, i, args...);
    }
  }

  ~Proactor() {
    for (std::size_t i = 0; i < N_PARTITIONS; ++i) {
      const auto computable = reinterpret_cast<COMPUTABLE*>(&partitions_[i]);
      if (computable != nullptr) {
        reinterpret_cast<COMPUTABLE*>(&partitions_[i])->~COMPUTABLE();
      }
    }
  }

  template <typename MemberFunc, typename Callback, typename... Args>
  bool process(const KEY& key, MemberFunc func, Callback&& callback,
               Args&&... args) {
    const std::size_t index = hash_policy(key) % N_PARTITIONS;
    Partition* partition = reinterpret_cast<Partition*>(&partitions_[index]);
    return partition->process(func, std::forward<Callback>(callback),
                              std::forward<Args>(args)...);
  }

  template <typename MemberFunc, typename Callback, typename... Args>
  bool process(MemberFunc func, Callback&& callback, Args&&... args) {
    for (int i = 0; i < N_PARTITIONS; ++i) {
      Partition* partition = reinterpret_cast<Partition*>(&partitions_[i]);
      partition->process(func, std::forward<Callback>(callback),
                         std::forward<Args>(args)...);
    }
    return true;
  }

  void stop() noexcept {
    for (std::size_t i = 0; i < N_PARTITIONS; ++i) {
      reinterpret_cast<Partition*>(&partitions_[i])->stop();
    }
  }

 private:
  HASH_POLICY hash_policy;
  // use char because we need to be able to default initialize the array and
  // Partition might not have default ctor.
  alignas(Partition) char partitions_[N_PARTITIONS][sizeof(Partition)];
  Partition& partition(int i) {
    return *reinterpret_cast<Partition*>(&partitions_[i]);
  }

  template <typename T, typename K>
  struct hash_policy_checks {
    static constexpr bool is_callable = std::is_invocable_v<T, K>;
    static constexpr bool returns_size_t =
        std::is_same_v<std::invoke_result_t<T, K>, std::size_t>;
  };

  static_assert(N_PARTITIONS > 0, "N_PARTITIONS must be greater than 0");
  static_assert(hash_policy_checks<HASH_POLICY, KEY>::is_callable,
                "HASH_POLICY must be callable with KEY");
  static_assert(hash_policy_checks<HASH_POLICY, KEY>::returns_size_t,
                "HASH_POLICY must return std::size_t");
  static_assert(std::is_default_constructible_v<HASH_POLICY>,
                "HASH_POLICY must be default constructible");
};

#endif  // PROACTOR_H