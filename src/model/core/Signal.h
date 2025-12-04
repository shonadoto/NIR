#pragma once

#include <algorithm>
#include <functional>
#include <mutex>
#include <vector>

/**
 * @brief Lightweight signal/slot helper independent from Qt.
 *
 * Provides simple connect/disconnect semantics. Thread-safety is ensured with a
 * mutex.
 */
template <typename... Args>
class Signal {
 public:
  using Slot = std::function<void(Args...)>;

  /**
   * @brief Connect slot to signal.
   * @return connection id (for disconnect).
   */
  int connect(Slot slot) {
    std::scoped_lock lock(mutex_);
    const int id = ++last_id_;
    slots_.emplace_back(id, std::move(slot));
    return id;
  }

  /**
   * @brief Disconnect slot by id.
   */
  void disconnect(int id) {
    std::scoped_lock lock(mutex_);
    auto it =
      std::remove_if(slots_.begin(), slots_.end(),
                     [id](const auto& pair) { return pair.first == id; });
    slots_.erase(it, slots_.end());
  }

  /**
   * @brief Emit event to all connected slots.
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
    for (const auto& slot : copy) {
      slot(args...);
    }
  }

 private:
  mutable std::mutex mutex_;
  std::vector<std::pair<int, Slot>> slots_;
  int last_id_{0};
};
