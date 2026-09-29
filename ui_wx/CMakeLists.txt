# tracker_ui_wx: wxWidgets adapter. The only target that depends on wx::wx.
# Replace this directory with a different toolkit (Qt, Dear ImGui, ...) without
# touching tracker_core.

add_library(tracker_ui_wx STATIC
    src/MainFrame.cpp
    src/SettingsDialog.cpp
    src/CreateFormatDialog.cpp
    src/CreateGameTitleDialog.cpp
    src/CreateGameTypeDialog.cpp
    src/CreateArchetypeDialog.cpp
    src/FormatWorkspace.cpp
    src/DecksPanel.cpp
    src/EditDeckDialog.cpp
    src/GamesPanel.cpp
    src/GameDialog.cpp
    src/StatisticsPanel.cpp
    src/DeckStatisticsPanel.cpp
    src/StatsWidgets.cpp
    src/Theme.cpp
    src/ImportGamesDialog.cpp
    src/ExportGamesDialog.cpp
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/tracker/ui/AppVersion.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/tracker/ui/AppVersion.hpp
    @ONLY
)

target_include_directories(tracker_ui_wx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

target_link_libraries(tracker_ui_wx
    PUBLIC
        tracker_core
        wx::wx
    # Intentionally NOT linking tracker_warnings here: wxWidgets headers raise
    # spurious diagnostics under -Wpedantic / -Wshadow that we'd have to
    # suppress per-call. The strict warning set is reserved for tracker_core.
)

# Theme.cpp subclasses EDIT parents via SetWindowSubclass / DefSubclassProc /
# RemoveWindowSubclass (comctl32); those symbols are not pulled in by wx alone.
if (WIN32)
    target_link_libraries(tracker_ui_wx PRIVATE comctl32)
endif()

target_compile_features(tracker_ui_wx PUBLIC cxx_std_20)
