cmake_minimum_required(VERSION 3.16)

project(PCMakerspace3DPKiosk VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick SerialPort Svg)
find_package(Qt6 REQUIRED COMPONENTS Core)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appPCMakerspace3DPKiosk
    src/main.cpp
)

qt_add_qml_module(appPCMakerspace3DPKiosk
    URI PCMakerspace3DPKiosk
    VERSION 1.0
    QML_FILES
        ui/Main.qml
    RESOURCES
        resources/PC-LogoText.svg
        resources/ltx2a_ledoff.svg
        resources/ltx2a_ledon.svg
        resources/card_filled.svg
        resources/PC-Logo.ico
        resources/OrcaSlicer.svg
        resources/help.svg
    SOURCES
        headers/ltx2aQT.h
        src/ltx2aQT.cpp
        headers/qrprovider.h
        src/qrprovider.cpp
        src/qrcodegen.cpp
        headers/qrcodegen.hpp
        headers/qrmanager.h
        src/qrmanager.cpp
        headers/secrets.h
        SOURCES headers/qtbackend.h
        SOURCES src/QTBackend.cpp
        SOURCES headers/gcodeparser.h headers/gcodeparser.cpp
)

qt_import_qml_plugins(appPCMakerspace3DPKiosk)


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appPCMakerspace3DPKiosk PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appPCMakerspace3DPKiosk
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appPCMakerspace3DPKiosk
    PRIVATE Qt6::Quick Qt6::SerialPort Qt6::Svg
)
target_link_libraries(appPCMakerspace3DPKiosk PRIVATE Qt6::Core)

include(GNUInstallDirs)
install(TARGETS appPCMakerspace3DPKiosk
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
