#pragma once

#include <atomic>
#include <string>

#include "model/core/ModelTypes.h"
#include "model/core/Signal.h"

/**
 * @brief Base class for all pure models. Provides id, name and change signals.
 */
class ModelObject {
 public:
  ModelObject();
  virtual ~ModelObject() = default;

  const std::string& id() const {
    return id_;
  }

  const std::string& name() const {
    return name_;
  }
  void set_name(const std::string& name);

  using ChangeSignal = Signal<const ModelChange&>;
  ChangeSignal& on_changed() {
    return changed_signal_;
  }

 protected:
  void notify_change(const ModelChange& change) const;

 private:
  static auto GenerateId() -> std::string;

  std::string id_;
  std::string name_{"New inclusion"};
  mutable ChangeSignal changed_signal_;
};
