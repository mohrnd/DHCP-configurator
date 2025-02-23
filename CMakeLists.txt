cmake_minimum_required(VERSION 3.16)
project(Linux-DHCP-Configurator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories for libssh
include_directories("/usr/include/libssh")

# Link directories for libssh
link_directories("/lib/x86_64-linux-gnu")

# Find Qt
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Add sources
set(PROJECT_SOURCES
    main.cpp
    widget.cpp
    widget.h
    widget.ui
    sshcmdexecfiletransfer.h
    sshcmdexecfiletransfer.cpp
)

# Handle Qt versions and platform-specific configurations
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Linux-DHCP-Configurator
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(Linux-DHCP-Configurator SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(Linux-DHCP-Configurator
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# Link the Qt Widgets module
target_link_libraries(Linux-DHCP-Configurator PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Link libssh for SSH functionality
target_link_libraries(Linux-DHCP-Configurator PRIVATE ssh)

# Bundle and macOS/Windows specific settings
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Linux-DHCP-Configurator)
endif()

set_target_properties(Linux-DHCP-Configurator PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Install the application
include(GNUInstallDirs)
install(TARGETS Linux-DHCP-Configurator
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Finalize Qt6 executable if needed
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Linux-DHCP-Configurator)
endif()
