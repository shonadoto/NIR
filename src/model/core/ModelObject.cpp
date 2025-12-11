#include "model/core/ModelObject.h"

#include <atomic>
#include <cstdint>
#include <sstream>
#include <string>

#include "model/core/ModelTypes.h"

namespace {
// Global atomic counter for generating unique IDs - must be mutable
std::atomic<uint64_t> g_id_counter{0};
}  // namespace

ModelObject::ModelObject() : id_(GenerateId()) {}

void ModelObject::set_name(const std::string& name) {
  // Trim whitespace from name
  std::string trimmed = name;
  const size_t first = trimmed.find_first_not_of(" \t\n\r");
  if (first != std::string::npos) {
    trimmed.erase(0, first);
    const size_t last = trimmed.find_last_not_of(" \t\n\r");
    if (last != std::string::npos) {
      trimmed.erase(last + 1);
    }
  } else {
    // String contains only whitespace
    trimmed.clear();
  }

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
