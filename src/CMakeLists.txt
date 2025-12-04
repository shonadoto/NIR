set(SOURCES
    app/main.cpp
    ui/MainWindow.cpp
    ui/EditorView.cpp
    ui/activity/ActivityBar.cpp
    ui/activity/ActivityButton.cpp
    ui/panels/ObjectsBar.cpp
    ui/panels/PropertiesBar.cpp
    ui/editor/EditorArea.cpp
    ui/bindings/ShapeModelBinder.cpp
    ui/editor/SubstrateItem.cpp
    ui/editor/SubstrateDialog.cpp
    ui/sidebar/SideBarWidget.cpp
    model/ObjectTreeModel.cpp
    model/DocumentModel.cpp
    model/core/ModelObject.cpp
    model/MaterialModel.cpp
    model/ShapeModel.cpp
    model/ShapeSizeConverter.cpp
    model/SubstrateModel.cpp
    scene/items/RectangleItem.cpp
    scene/items/EllipseItem.cpp
    scene/items/CircleItem.cpp
    scene/items/StickItem.cpp
    serialization/ProjectSerializer.cpp
    )

set(HEADERS
    ui/MainWindow.h
    ui/EditorView.h
    ui/activity/ActivityBar.h
    ui/activity/ActivityButton.h
    ui/panels/ObjectsBar.h
    ui/panels/PropertiesBar.h
    ui/editor/EditorArea.h
    ui/bindings/ShapeModelBinder.h
    ui/utils/ColorUtils.h
    ui/editor/SubstrateItem.h
    ui/editor/SubstrateDialog.h
    ui/sidebar/SideBarWidget.h
    model/ObjectTreeModel.h
    model/DocumentModel.h
    model/core/Signal.h
    model/core/ModelTypes.h
    model/core/ModelObject.h
    model/MaterialModel.h
    model/ShapeModel.h
    model/ShapeSizeConverter.h
    model/SubstrateModel.h
    scene/ISceneObject.h
    scene/items/RectangleItem.h
    scene/items/EllipseItem.h
    scene/items/CircleItem.h
    scene/items/StickItem.h
    serialization/ProjectSerializer.h
    utils/Logging.h
    )

add_executable(NIRMaterialEditor
    ${SOURCES}
    ${HEADERS}
    ${CMAKE_SOURCE_DIR}/resources/app.qrc
)

target_include_directories(NIRMaterialEditor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(NIRMaterialEditor PRIVATE Qt6::Widgets Qt6::Svg spdlog::spdlog)

# Add debug flags for better crash diagnostics
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT CMAKE_BUILD_TYPE)
    target_compile_options(NIRMaterialEditor PRIVATE
        -g                    # Debug symbols
        -O0                   # No optimization for easier debugging
        -fno-omit-frame-pointer  # Better stack traces
        -Wall                 # Enable all warnings
        -Wextra               # Extra warnings
    )
endif()

if (WIN32)
    target_compile_definitions(NIRMaterialEditor PRIVATE -DUNICODE -D_UNICODE)
endif()

if (APPLE)
    set_target_properties(NIRMaterialEditor PROPERTIES MACOSX_BUNDLE TRUE)
endif()

if (APPLE)
    install(TARGETS NIRMaterialEditor BUNDLE DESTINATION .)
else()
    install(TARGETS NIRMaterialEditor RUNTIME DESTINATION bin)
endif()


