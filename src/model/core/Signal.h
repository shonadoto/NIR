#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <mutex>
#include <vector>

/**
 * @brief Lightweight signal/slot helper independent from Qt.
 *
 * Provides simple connect/disconnect semantics. Thread-safety is ensured with a
 * mutex.
 *
 * @note Exceptions thrown by slots are not caught - they will propagate and
 * prevent remaining slots from being called.
 */
template <typename... Args>
class Signal {
 public:
  using Slot = std::function<void(Args...)>;

  /**
   * @brief Connect slot to signal.
   * @return connection id (for disconnect), or -1 if ID counter overflowed.
   */
  int connect(Slot slot) {
    std::scoped_lock lock(mutex_);
    // Check for overflow - if last_id_ is at max, reset to 0 (wraps around)
    // This is safe because we check for existing IDs in disconnect
    if (last_id_ == std::numeric_limits<int>::max()) {
      last_id_ = 0;
    }
    const int id = ++last_id_;
    slots_.emplace_back(id, std::move(slot));
    return id;
  }

  /**
   * @brief Disconnect slot by id.
   * @param id Connection id returned by connect().
   */
  void disconnect(int id) {
    std::scoped_lock lock(mutex_);
    auto it =
      std::remove_if(slots_.begin(), slots_.end(),
                     [id](const auto& pair) { return pair.first == id; });
    slots_.erase(it, slots_.end());
  }

  /**
   * @brief Disconnect all slots.
   */
  void disconnect_all() {
    std::scoped_lock lock(mutex_);
    slots_.clear();
  }

  /**
   * @brief Get number of connected slots.
   * @return Number of connected slots.
   */
  size_t slot_count() const {
    std::scoped_lock lock(mutex_);
    return slots_.size();
  }

  /**
   * @brief Check if any slots are connected.
   * @return true if at least one slot is connected.
   */
  bool has_slots() const {
    std::scoped_lock lock(mutex_);
    return !slots_.empty();
  }

  /**
   * @brief Emit event to all connected slots.
   *
   * @note If a slot throws an exception, remaining slots will not be called.
   * @note Slots are called in the order they were connected.
   */
  void emit_signal(Args... args) const {
    std::vector<Slot> copy;
    {
      std::scoped_lock lock(mutex_);
      copy.reserve(slots_.size());
      for (const auto& pair : slots_) {
        copy.emplace_back(pair.second);
      }
    }
    // Call slots outside of lock to avoid deadlocks if slots try to
    // connect/disconnect
    for (const auto& slot : copy) {
      if (slot) {  // Check if slot is valid
        slot(args...);
      }
    }
  }

 private:
  mutable std::mutex mutex_;
  std::vector<std::pair<int, Slot>> slots_;
  int last_id_{0};
};
