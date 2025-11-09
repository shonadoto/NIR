#pragma once

class QWidget;

/**
 * @brief Interface for scene objects that can provide custom property widgets.
 *
 * All scene objects (shapes, substrate) implement this interface to expose
 * their editable properties in the PropertiesBar.
 */
class ISceneObject {
public:
    virtual ~ISceneObject() = default;

    /**
     * @brief Create a widget with controls for this object's properties.
     * @param parent Parent widget for the created widget.
     * @return QWidget* containing property controls (spinboxes, color pickers, etc.)
     *
     * The returned widget is owned by the caller and should be deleted when no longer needed.
     * Property changes in the widget should directly update the object.
     */
    virtual QWidget* create_properties_widget(QWidget *parent) = 0;
};

