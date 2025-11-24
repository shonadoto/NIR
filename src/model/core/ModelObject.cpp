#include "model/core/ModelObject.h"

#include <sstream>

namespace {
std::atomic<uint64_t> g_id_counter {0};
}

ModelObject::ModelObject()
    : id_(generate_id()) {}

void ModelObject::set_name(const std::string &name) {
    if (name == name_) {
        return;
    }
    name_ = name;
    notify_change(ModelChange{ModelChange::Type::NameChanged, "name"});
}

void ModelObject::notify_change(ModelChange change) const {
    changed_signal_.emit_signal(change);
}

std::string ModelObject::generate_id() {
    uint64_t value = ++g_id_counter;
    std::ostringstream oss;
    oss << "obj-" << value;
    return oss.str();
}

