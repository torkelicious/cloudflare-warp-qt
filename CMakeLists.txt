cmake_minimum_required(VERSION 3.16)

project(CloudflareWarpQt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        src/main.cpp
        src/widget.cpp
        src/widget.h
        src/widget.ui
        src/mainfunctions.cpp
        src/mainfunctions.h
        src/systray.cpp
        src/systray.h
        resources/resources.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(CloudflareWarpQt MANUAL_FINALIZATION ${PROJECT_SOURCES}
        src/settingsdiag.h src/settingsdiag.cpp)
else()
    add_executable(CloudflareWarpQt ${PROJECT_SOURCES})
endif()

target_link_libraries(CloudflareWarpQt PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_include_directories(CloudflareWarpQt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# INSTALLATION
option(ENABLE_INSTALL "Enable installation rules" OFF)

if(ENABLE_INSTALL)
    message(STATUS "Installation rules ENABLED")
    include(GNUInstallDirs)

    # Install Binary
    install(TARGETS CloudflareWarpQt
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Install Desktop File
    install(FILES warp-qt.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
        RENAME CloudflareWarpQt.desktop
    )

     # Install Icon for the Launcher
     install(FILES resources/app_icon.png
         DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps
         RENAME cloudflare-warp-qt.png
     )
else()
    message(STATUS "Installation rules DISABLED")
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(CloudflareWarpQt)
endif()
