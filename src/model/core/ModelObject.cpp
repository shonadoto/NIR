#include "model/core/ModelObject.h"

#include <atomic>
#include <cstdint>
#include <sstream>
#include <string>

#include "model/core/ModelTypes.h"

namespace {
// Global atomic counter for generating unique IDs - must be mutable
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic<uint64_t> g_id_counter{0};
}  // namespace

ModelObject::ModelObject() : id_(GenerateId()) {}

void ModelObject::set_name(const std::string& name) {
  // Trim whitespace from name
  std::string trimmed = name;
  trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
  trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

  if (trimmed == name_) {
    return;
  }
  name_ = trimmed;
  notify_change(ModelChange{ModelChange::Type::NameChanged, "name"});
}

void ModelObject::notify_change(const ModelChange& change) const {
  changed_signal_.emit_signal(change);
}

auto ModelObject::GenerateId() -> std::string {
  const uint64_t value = ++g_id_counter;
  std::ostringstream oss;
  oss << "obj-" << value;
  return oss.str();
}
