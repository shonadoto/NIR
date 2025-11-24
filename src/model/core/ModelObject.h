#pragma once

#include "model/core/Signal.h"
#include "model/core/ModelTypes.h"
#include <string>
#include <atomic>

/**
 * @brief Base class for all pure models. Provides id, name and change signals.
 */
class ModelObject {
public:
    ModelObject();
    virtual ~ModelObject() = default;

    const std::string& id() const { return id_; }

    const std::string& name() const { return name_; }
    void set_name(const std::string &name);

    using ChangeSignal = Signal<const ModelChange&>;
    ChangeSignal& on_changed() { return changed_signal_; }

protected:
    void notify_change(ModelChange change) const;

private:
    static std::string generate_id();

    std::string id_;
    std::string name_ {"Unnamed"};
    mutable ChangeSignal changed_signal_;
};

