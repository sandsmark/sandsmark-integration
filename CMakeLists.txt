cmake_minimum_required(VERSION 2.8.12)

project(SandsmarkIntegration)
set(PROJECT_VERSION "1.0")
set(PROJECT_VERSION_MAJOR 5)

include(FeatureSummary)
find_package(ECM 5.17.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://projects.kde.org/projects/kdesupport/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(GenerateExportHeader)
include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(ECMGenerateHeaders)

include(KDEInstallDirs)
include(KDECMakeSettings)

find_package(Qt5 5.5.0 CONFIG REQUIRED
    Widgets
    DBus
    X11Extras
    QuickControls2
    )

find_package(KF5 REQUIRED
    Config
    ConfigWidgets
    I18n
    IconThemes
    KIO
    CoreAddons
    Notifications
    WidgetsAddons
    WindowSystem
    Wayland
    )

add_definitions(-DTRANSLATION_DOMAIN=\"sandsmarkintegration5\")
if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/po")
    ki18n_install(po)
endif()
add_subdirectory(src)
add_subdirectory(autotests)
add_subdirectory(tests)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
