cmake_minimum_required(VERSION 3.16)

project(M3uVideoPlayerPc VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Qml Quick QuickControls2 Sql Network Concurrent REQUIRED)

# Try to find libmpv
# Note: On Windows, you might need to specify the path to mpv headers and lib
# e.g., via CMAKE_PREFIX_PATH or MPV_DIR
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(MPV QUIET mpv)
endif()

if(NOT MPV_FOUND)
    # Hint CMake to look in a local 3rdparty folder first
    set(LOCAL_MPV_DIR "${CMAKE_SOURCE_DIR}/3rdparty/libmpv")
    
    # Clear cache if it previously failed
    if(MPV_INCLUDE_DIR MATCHES "NOTFOUND")
        unset(MPV_INCLUDE_DIR CACHE)
    endif()
    if(MPV_LIBRARY MATCHES "NOTFOUND")
        unset(MPV_LIBRARY CACHE)
    endif()
    
    find_path(MPV_INCLUDE_DIR 
        NAMES mpv/client.h
        PATHS ${LOCAL_MPV_DIR}/include
    )
    find_library(MPV_LIBRARY 
        NAMES mpv mpv-2 libmpv
        PATHS ${LOCAL_MPV_DIR}/lib ${LOCAL_MPV_DIR}
    )
    if(MPV_INCLUDE_DIR AND MPV_LIBRARY)
        set(MPV_FOUND TRUE)
        set(MPV_LIBRARIES ${MPV_LIBRARY})
        set(MPV_INCLUDE_DIRS ${MPV_INCLUDE_DIR})
    endif()
endif()

if(MPV_FOUND)
    message(STATUS "Found mpv: ${MPV_LIBRARIES}")
else()
    message(WARNING "libmpv not found! Please set MPV_INCLUDE_DIR and MPV_LIBRARY or install libmpv. Features relying on it will fail to compile or link.")
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/domain/FuzzySearch.cpp
    src/data/DatabaseManager.cpp
    src/data/PlaylistRepository.cpp
    src/data/ChannelRepository.cpp
    src/data/M3uParser.cpp
    src/data/ContentTypeDetector.cpp
    src/data/ResumeRepository.cpp
    src/data/XtreamApiClient.cpp
    src/infrastructure/TrustedTimeSource.cpp
    src/infrastructure/TimeZoneManager.cpp
    src/infrastructure/SettingsManager.cpp
    src/infrastructure/PosterFetchService.h
    src/infrastructure/PosterFetchService.cpp
    src/infrastructure/player/MpvVideoItem.cpp
    src/ui/PlaylistViewModel.h
    src/ui/PlaylistViewModel.cpp
    src/ui/GroupViewModel.h
    src/ui/GroupViewModel.cpp
    src/ui/ChannelViewModel.h
    src/ui/ChannelViewModel.cpp
    src/ui/AppController.h
    src/ui/AppController.cpp
)

# Application executable
if(WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

# Qt QML resource
qt_add_qml_module(${PROJECT_NAME}
    URI M3uVideoPlayer
    VERSION 1.0
    QML_FILES
        src/ui/main.qml
        src/ui/components/Sidebar.qml
        src/ui/components/SidebarButton.qml
        src/ui/components/AddPlaylistDialog.qml
        src/ui/components/SortMenuButton.qml
        src/ui/components/ViewModeToggle.qml
        src/ui/components/FolderIcon.qml
        src/ui/components/ChannelLogo.qml
        src/ui/screens/PlaylistListView.qml
        src/ui/screens/DirectLinkView.qml
        src/ui/screens/GroupListView.qml
        src/ui/screens/ChannelListView.qml
        src/ui/screens/PlayerView.qml
        src/ui/screens/SettingsView.qml
        src/ui/screens/SettingsPlaylistsView.qml
        src/ui/components/EditPlaylistDialog.qml
        src/ui/components/TimeZoneDialog.qml
        src/ui/components/ResumeDialog.qml
        src/ui/components/ResumeFab.qml
    RESOURCES
        src/assets/icon-512.png
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/ui
    ${MPV_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Network
    Qt6::Concurrent
)

if(MPV_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${MPV_LIBRARIES})
endif()

# Deployment (basic setup for Qt on Windows)
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    # Deploy Qt libraries (requires windeployqt in PATH or Qt environment)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E echo "Please run windeployqt manually or configure cmake deployment."
    )
endif()
