#pragma once

#include <functional>

class QWidget;
class QJsonObject;
class QString;

/**
 * @brief Interface for scene objects that can provide custom property widgets
 * and serialization.
 *
 * All scene objects (shapes, substrate) implement this interface to expose
 * their editable properties in the PropertiesBar and support JSON save/load.
 */
class ISceneObject {
 public:
  virtual ~ISceneObject() = default;

  /**
   * @brief Create a widget with controls for this object's properties.
   * @param parent Parent widget for the created widget.
   * @return QWidget* containing property controls (spinboxes, color pickers,
   * etc.)
   *
   * The returned widget is owned by the caller and should be deleted when no
   * longer needed. Property changes in the widget should directly update the
   * object.
   */
  virtual QWidget* create_properties_widget(QWidget* parent) = 0;

  /**
   * @brief Serialize object to JSON.
   * @return QJsonObject containing all object properties.
   */
  virtual QJsonObject to_json() const = 0;

  /**
   * @brief Restore object state from JSON.
   * @param json JSON object with properties.
   */
  virtual void from_json(const QJsonObject& json) = 0;

  /**
   * @brief Get type identifier for serialization.
   * @return QString type name (e.g. "rectangle", "ellipse", "circle", "stick",
   * "substrate")
   */
  virtual QString type_name() const = 0;

  /**
   * @brief Get object name.
   */
  virtual QString name() const = 0;

  /**
   * @brief Set object name.
   */
  virtual void set_name(const QString& name) = 0;

  /**
   * @brief Register callback invoked when geometry changes
   * (position/size/rotation).
   */
  virtual void set_geometry_changed_callback(
    std::function<void()> callback) = 0;

  /**
   * @brief Remove previously registered geometry callback.
   */
  virtual void clear_geometry_changed_callback() = 0;

  /**
   * @brief Set material model for grid rendering.
   * @param material Pointer to material model (can be nullptr for no material).
   */
  virtual void set_material_model(class MaterialModel* material) = 0;
};
