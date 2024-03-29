find_package(X11)
set_package_properties(X11 PROPERTIES DESCRIPTION "Required for updating the Cursor theme on X11"
                        URL "http://www.x.org"
                        TYPE REQUIRED
                        )
set(HAVE_X11 ${X11_FOUND})
if(HAVE_X11)
  find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED X11Extras)
endif()

configure_file(config-platformtheme.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-platformtheme.h )

set(platformtheme_SRCS
    kdeplatformtheme.cpp
    kfontsettingsdata.cpp
    khintssettings.cpp
    kdeplatformfiledialoghelper.cpp
    kdeplatformfiledialogbase.cpp
    kdeplatformsystemtrayicon.cpp
    kfiletreeview.cpp
    kdirselectdialog.cpp
    sfilemetapreview.cpp
    x11integration.cpp
    main.cpp

    sfileplacesmodel.cpp
    sfileplacesview.cpp
    sfileplacesitem.cpp
)

add_library(SandsmarkPlatformTheme MODULE ${platformtheme_SRCS})

target_link_libraries(SandsmarkPlatformTheme
    PRIVATE
        Qt5::GuiPrivate
        Qt5::DBus
        Qt5::X11Extras
        Qt5::QuickControls2
        KF5::ConfigWidgets
        KF5::ConfigCore
        KF5::CoreAddons
        KF5::IconThemes
        KF5::KIOFileWidgets # KFileFilterCombo, KDirSortFilterProxyModel, KRecentDirs
        KF5::KIOWidgets
        KF5::XmlGui
        KF5::GuiAddons
        KF5::I18n
        KF5::Notifications
        KF5::WindowSystem
        ${QT5PLATFORMSUPPORT_LIBS}

)

if(HAVE_X11)
  target_link_libraries(SandsmarkPlatformTheme PRIVATE Qt5::X11Extras ${X11_Xcursor_LIB})
endif()

install(TARGETS SandsmarkPlatformTheme DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/platformthemes)
