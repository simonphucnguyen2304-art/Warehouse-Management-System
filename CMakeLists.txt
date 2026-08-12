# =============================================================================
# @file   CMakeLists.txt
# @brief  Build configuration and dependency management for the project.
#
# @author Duong Anh Hao
# @date   2026-06-15
#
# @update
# @author Do Minh Khang
# @date   2026-07-14
# @changelog
#   - Removed the hardcoded personal Qt path. Replaced with a fallback chain
#     (see "Qt setup" below) so a fresh clone builds without every developer
#     needing to hand-edit this file.
#   - Left Qt6::Sql / domain-queries / schema.sql wiring OUT for now, since
#     none of those files exist on this branch yet. Re-adding them later is a
#     3-line change - see the commented markers below.
#
# @update
# @author Do Minh Khang
# @date   2026-07-24
# @changelog Update CMake min version to 3.23
# =============================================================================

cmake_minimum_required(VERSION 3.23)
project(WarehouseMS VERSION 1.0 LANGUAGES CXX)


# --C++ standard--
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Force MSVC to report the standard __cplusplus value instead of its legacy
# default - without this, C++20 feature checks based on __cplusplus can be
# wrong under MSVC even with /std:c++20 set.
if (MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()


# --Qt setup--
# CMAKE_PREFIX_PATH (where CMake looks for your local Qt install) is NOT
# hardcoded - a path like "C:/Qt/6.11.1/msvc2022_64" is specific to one
# machine and breaks every other teammate's build the moment it's committed.
# Instead, Qt is located in this order:
#
#   1. CMAKE_PREFIX_PATH already set externally (CMakeSettings.json,
#      CMakeUserPresets.json, or -DCMAKE_PREFIX_PATH=... on the command
#      line) - always wins if present.
#   2. The QT_BIN_DIR environment variable, if set on this machine
#      (see CMakeSettings.json - each developer sets this once, outside git).
#   3. The default Windows Qt installer location, C:/Qt/<version>/msvc2022_64
#      - auto-detected by globbing, newest version picked if several are
#      installed. Covers the common case with zero per-developer setup.
#
# If none of these find Qt, configuration fails with a clear message instead
# of a cryptic "Qt6Config.cmake not found" error with no next step.

if (NOT CMAKE_PREFIX_PATH AND DEFINED ENV{QT_BIN_DIR})
    get_filename_component(_qt_prefix_from_env "$ENV{QT_BIN_DIR}/.." ABSOLUTE)
    set(CMAKE_PREFIX_PATH "${_qt_prefix_from_env}" CACHE PATH "Qt6 install (from QT_BIN_DIR)")
    message(STATUS "Using Qt6 from QT_BIN_DIR environment variable: ${CMAKE_PREFIX_PATH}")
endif()

if (NOT CMAKE_PREFIX_PATH AND WIN32)
    file(GLOB _qt_candidates "C:/Qt/*/msvc2022_64")
    if (_qt_candidates)
        list(SORT _qt_candidates COMPARE NATURAL ORDER DESCENDING)
        list(GET _qt_candidates 0 _qt_latest)
        set(CMAKE_PREFIX_PATH "${_qt_latest}" CACHE PATH "Auto-detected Qt6 install")
        message(STATUS "Auto-detected Qt6 at: ${_qt_latest}")
    endif()
endif()

if (NOT CMAKE_PREFIX_PATH)
    message(STATUS
        "Qt6 was not found automatically. Set it via one of:\n"
        "  - A QT_BIN_DIR environment variable pointing at your Qt 'bin' folder\n"
        "  - CMakeSettings.json's cmakeCommandArgs\n"
        "  - CMakeUserPresets.json (see CMakeUserPresets.json.example)\n"
        "  - -DCMAKE_PREFIX_PATH=<path-to-Qt>/<kit> on the command line")
endif()

# Enable Qt's build-time code generators:
#   AUTOMOC - processes Q_OBJECT classes for the signal/slot system
#   AUTORCC - compiles .qrc resource files (icons, images, etc.)
#   AUTOUIC - compiles .ui files from Qt Designer into C++ headers
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Locate the required Qt6 libraries including UI components
find_package(Qt6 COMPONENTS Core Widgets Gui Charts Sql Svg REQUIRED)


# --Include directories--
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/entities
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/queries
    ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/states
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repository
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
)

# --Source files--
# Note on GLOB: CMake does not automatically notice new files matching these
# patterns - after adding a new .cpp file, re-run "cmake .." (or reconfigure
# in your IDE) once before building, or the new file will be silently skipped.
file(GLOB_RECURSE DOMAIN_SOURCES
    "src/domain/entities/*.cpp"
    "src/domain/queries/*.cpp"
    "src/domain/states/*.cpp"
)
file(GLOB_RECURSE REPOSITORY_SOURCES
    "src/repository/*.cpp"
)
file(GLOB_RECURSE SERVICE_SOURCES
    "src/service/*.cpp"
)
file(GLOB_RECURSE GUI_SOURCES 
    "src/gui/*.cpp"
)

set(SOURCES
    ${DOMAIN_SOURCES}
    ${REPOSITORY_SOURCES}
    ${SERVICE_SOURCES}
    ${GUI_SOURCES}
    main.cpp
)

# --Executable target--
add_executable(${PROJECT_NAME} ${SOURCES})

# --Embed icon resources (sort arrows, etc.)--
qt_add_resources(${PROJECT_NAME} "app_icons"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    FILES
        resources/icons/sort_asc.svg
        resources/icons/sort_desc.svg
        resources/icons/chevron_left.svg
        resources/icons/chevron_right.svg
)

# --Embed centralized QSS theme (GUI modernization - Phase 1 infra)--
qt_add_resources(${PROJECT_NAME} "app_theme"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    FILES
        resources/theme.qss
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
    Qt6::Svg
)

if (WIN32)
    get_target_property(_qt_qmake_location Qt6::qmake IMPORTED_LOCATION)
    if (_qt_qmake_location)
        get_filename_component(_qt_bin_dir "${_qt_qmake_location}" DIRECTORY)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    endif()
 
    if (WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --no-translations
                    --no-compiler-runtime
                    $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "Deploying Qt runtime DLLs next to ${PROJECT_NAME}.exe via windeployqt"
        )
    else()
        message(WARNING
            "windeployqt not found - Qt DLLs (e.g. Qt6Sqld.dll) may be missing at runtime.\n"
            "Add your Qt 'bin' folder to PATH, or run windeployqt manually on the built .exe.")
    endif()
endif()

# Once resources/db/schema.sql exists (SQL work merged), this copies it next
# to the built executable so DatabaseConnection can find it at runtime.
# Guarded with if(EXISTS ...) so this file works both before and after that
# merge without needing to be edited again.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/db/schema.sql")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources/db
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/db/schema.sql
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources/db/schema.sql
    )
endif()

# --Tests (opt-in, currently empty)--
option(BUILD_TESTS "Build the WarehouseMS test suite" OFF)
if (BUILD_TESTS)
    enable_testing()
    # add_subdirectory(tests)
endif()
