#ifndef PROACTOR_H
#define PROACTOR_H

#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

#include "ProactorPartition.h"

namespace mbucko {

/// The Proactor class implements a partitioned, multi-threaded, asynchronous
/// task processing framework. It distributes tasks across multiple partitions
/// based on a key and a hash policy, allowing for concurrent execution of
/// tasks on different COMPUTABLE objects. The class statically allocates
/// N_PARTITIONS partitions and uses a lock-free queue for efficient data
/// passing between threads.
///
/// \tparam KEY
///     The type used as a key for task distribution. Must be hashable.
/// \tparam HASH_POLICY
///     A functor type that provides the hashing mechanism for KEY.
///     Must have an operator() that takes a KEY and returns a std::size_t.
/// \tparam N_PARTITIONS
///     The number of partitions (and thus, the number of worker threads).
///     Must be greater than 0.
/// \tparam COMPUTABLE
///     The type of object on which tasks will be executed. Each partition
///     contains one instance of this type.
///
/// Example usage:
/// \code
/// class Adder {
///  public:
///   Adder(uint32_t init_value) : value_(init_value) {}
///
///   void add(uint32_t value) { value_ += value; }
///
///   uint32_t get() const { return value_; }
///
///  private:
///   uint32_t value_;
/// };
///
/// static constexpr std::size_t kPartitions = 10;
/// static constexpr std::size_t kQueueSize = 1000;
/// struct HashPolicy {
///   std::size_t operator()(int key) const { return key * 1009; }
/// };
///
/// // Inside code block
/// {
///   Proactor<int, HashPolicy, kPartitions, Adder> proactor(kQueueSize, 0);
///   std::atomic<uint32_t> retrievedSum{0};
///
///   proactor.process(0, &Adder::add, []() {}, 1u);
///   proactor.process(2, &Adder::get, [&retrievedSum](uint32_t sum) {
///     retrievedSum.store(sum);
///   });
///
///   proactor.stop();
/// }
/// \endcode
///
template <typename KEY, typename HASH_POLICY, std::size_t N_PARTITIONS,
          typename COMPUTABLE>
class Proactor {
 private:
  using Partition = ProactorPartition<COMPUTABLE>;

 public:
  /// Creates an instance of Proactor class.
  ///
  /// \param[in] capacity The maximum number of tasks the queue can hold.
  /// \param[in] args Arguments to be forwarded to the COMPUTABLE constructor.
  template <typename... Args>
  Proactor(std::size_t capacity, const Args&... args) : hash_policy() {
    static_assert(
        std::is_constructible_v<Partition, std::size_t, std::size_t, Args...>,
        "Arguments do not match Partition constructor");
    for (std::size_t i = 0; i < N_PARTITIONS; ++i) {
      new (&partitions_[i]) Partition(capacity, i, args...);
    }
  }

  /// Destructor. Stops the processing thread.
  ~Proactor() {
    for (std::size_t i = 0; i < N_PARTITIONS; ++i) {
      const auto computable = reinterpret_cast<COMPUTABLE*>(&partitions_[i]);
      if (computable != nullptr) {
        reinterpret_cast<COMPUTABLE*>(&partitions_[i])->~COMPUTABLE();
      }
    }
  }

  /// Enqueues a task to be processed asynchronously. Uses the provided key
  /// and HASH_POLICY to determine the partition on which to enqueue the task.
  /// It will block until space in the queue becomes available. This function
  /// is thread-safe and can be called concurrently from multiple threads.
  /// Calling this function after calling 'stop()' results in undefined
  /// behavior.
  ///
  /// \param[in] key
  ///     The key used to determine the target partition.
  /// \param[in] func
  ///     A member function pointer of COMPUTABLE to be executed
  ///     asynchronously on the selected partition.
  /// \param[in] callback
  ///     A function to be called with the result of func (if any).
  /// \param[in] args
  ///     Arguments to be passed to func.
  /// \return
  ///     Return true if the task was successfully enqueued, false otherwise.
  template <typename MemberFunc, typename Callback, typename... Args>
  bool process(const KEY& key, MemberFunc func, Callback&& callback,
               Args&&... args) {
    const std::size_t index = hash_policy(key) % N_PARTITIONS;
    Partition* partition = reinterpret_cast<Partition*>(&partitions_[index]);
    return partition->process(func, std::forward<Callback>(callback),
                              std::forward<Args>(args)...);
  }

  /// Enqueues a task to be processed asynchronously on each partition. This
  /// function will block until space in the queue becomes available. If one
  /// partition's queue is full, the function will block, potentially delaying
  /// task distribution to subsequent partitions. This function is thread-safe
  /// and can be called concurrently from multiple threads. Calling this
  /// function after calling 'stop()' is undefined behavior.
  ///
  /// \param[in] func
  ///     A member function pointer of COMPUTABLE to be executed
  ///     asynchronously on the selected partition.
  /// \param[in] callback
  ///     A function to be called with the result of func (if any).
  /// \param[in] args
  ///     Arguments to be passed to func.
  /// \return
  ///     Returns true if the task was successfully enqueued on all
  ///     partitions, false if enqueuing failed for any partition.
  template <typename MemberFunc, typename Callback, typename... Args>
  bool process(MemberFunc func, Callback&& callback, Args&&... args) {
    bool success = true;
    for (int i = 0; i < N_PARTITIONS; ++i) {
      Partition* partition = reinterpret_cast<Partition*>(&partitions_[i]);
      success &= partition->process(func, std::forward<Callback>(callback),
                                    std::forward<Args>(args)...);
    }
    return success;
  }

  /// Stops all processing threads and prevents further task enqueuing.
  /// This function is thread-safe and can be called multiple times safely.
  /// After calling this function, calling any other function on this object
  /// results in undefined behavior.
  void stop() noexcept {
    for (std::size_t i = 0; i < N_PARTITIONS; ++i) {
      reinterpret_cast<Partition*>(&partitions_[i])->stop();
    }
  }

 private:
  HASH_POLICY hash_policy;
  // Use 'char' array for raw storage to allow placement new initialization.
  // This approach avoids potential issues with Partition's possible lack of a
  // default constructor, while still enabling proper alignment and efficient
  // memory usage.
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

}  // namespace mbucko

#endif  // PROACTOR_H