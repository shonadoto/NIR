#pragma once

#include "model/core/ModelObject.h"
#include "model/ShapeModel.h"
#include "model/MaterialModel.h"
#include "model/SubstrateModel.h"
#include "model/core/Signal.h"
#include <vector>
#include <memory>

class DocumentModel {
public:
    DocumentModel();

    std::shared_ptr<ShapeModel> create_shape(ShapeModel::ShapeType type, const std::string &name = {});
    void remove_shape(const std::shared_ptr<ShapeModel> &shape);
    void clear_shapes();
    const std::vector<std::shared_ptr<ShapeModel>>& shapes() const { return shapes_; }

    std::shared_ptr<MaterialModel> create_material(const Color &color = {}, const std::string &name = {});
    void remove_material(const std::shared_ptr<MaterialModel> &material);
    void clear_materials();
    const std::vector<std::shared_ptr<MaterialModel>>& materials() const { return materials_; }

    std::shared_ptr<SubstrateModel> substrate() const { return substrate_; }
    void set_substrate(const std::shared_ptr<SubstrateModel> &substrate);

    using DocumentSignal = Signal<const ModelChange&>;
    DocumentSignal& on_changed() { return changed_signal_; }

private:
    void notify_all(const ModelChange &change);

    std::vector<std::shared_ptr<ShapeModel>> shapes_;
    std::vector<std::shared_ptr<MaterialModel>> materials_;
    std::shared_ptr<SubstrateModel> substrate_;
    DocumentSignal changed_signal_;
};

