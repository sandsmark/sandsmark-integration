add_library(sandsmark MODULE
    sandsmarkstyle.cpp
    sandsmarkstyle.h
    sandsmarkstyleplugin.cpp
    sandsmarkstyleplugin.h
    )

target_link_libraries(sandsmark
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::WidgetsPrivate
    )

install(TARGETS sandsmark DESTINATION ${QT_PLUGIN_INSTALL_DIR}/styles/)
