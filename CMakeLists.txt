cmake_minimum_required(VERSION 3.16)

project(cloudflare-warp-qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Concurrent)

set(PROJECT_SOURCES
        src/main.cpp
        src/widget.cpp
        src/widget.h
        src/widget.ui
        src/mainfunctions.cpp
        src/mainfunctions.h
        src/systray.cpp
        src/systray.h
        src/settingsdiag.cpp
        src/settingsdiag.h
        resources/resources.qrc
)

if (QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${PROJECT_SOURCES})
else ()
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
endif ()

set_target_properties(${PROJECT_NAME}
        PROPERTIES
        OUTPUT_NAME ${PROJECT_NAME}
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Concurrent
)

target_include_directories(${PROJECT_NAME}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES resources/cloudflare-warp-qt.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
        RENAME ${PROJECT_NAME}.desktop
)

install(FILES resources/app_icon.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps
        RENAME ${PROJECT_NAME}.png
)

if (QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif ()
