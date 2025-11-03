set(SOURCES
    app/main.cpp
    ui/MainWindow.cpp
    ui/EditorView.cpp
    ui/PanelArea.cpp
    )

set(HEADERS
    ui/MainWindow.h
    ui/EditorView.h
    ui/PanelArea.h
    )

add_executable(NIRMaterialEditor
    ${SOURCES}
    ${HEADERS}
    ${CMAKE_SOURCE_DIR}/resources/app.qrc
)

target_include_directories(NIRMaterialEditor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(NIRMaterialEditor PRIVATE Qt6::Widgets Qt6::Svg)

if (WIN32)
    target_compile_definitions(NIRMaterialEditor PRIVATE -DUNICODE -D_UNICODE)
endif()

if (APPLE)
    set_target_properties(NIRMaterialEditor PROPERTIES MACOSX_BUNDLE TRUE)
endif()

install(TARGETS NIRMaterialEditor RUNTIME DESTINATION bin)


