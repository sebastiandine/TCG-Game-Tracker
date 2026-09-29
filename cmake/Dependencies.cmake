# Third-party dependencies pulled via FetchContent. Pinning to known-good tags
# is intentional - bump deliberately, never use `master`.

include(FetchContent)

# Avoid hammering the network on every reconfigure once content is downloaded.
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)

# CMake 4 dropped compatibility with cmake_minimum_required(VERSION < 3.5).
# Some pinned tags below (doctest 2.4.11, nlohmann/json 3.11.3, wxWidgets 3.2.5)
# still ship a `cmake_minimum_required(VERSION 3.0/3.1)` and would otherwise
# fail to configure. Override the floor for all FetchContent'd subprojects.
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

# ---------------------------------------------------------------------------
# nlohmann/json - JSON (de)serialization. Header-only.
# ---------------------------------------------------------------------------
message(STATUS "[tracker] Fetching nlohmann/json ...")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------------------------------------------
# SQLite - embedded SQL database engine via the official amalgamation.
# The zip contains sqlite3.c + sqlite3.h but no CMakeLists.txt, so we use
# FetchContent_Populate and build our own static library target.
# Public domain / blessing license — compatible with MIT.
# ---------------------------------------------------------------------------
message(STATUS "[tracker] Fetching SQLite amalgamation ...")
FetchContent_Declare(
    sqlite_amalgamation
    URL                    https://www.sqlite.org/2026/sqlite-amalgamation-3530400.zip
    URL_HASH               SHA3_256=628a44cfe82c66aed1ccbbe85a562d2e33ebe64b3288981ed76285612227934e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(sqlite_amalgamation)

add_library(sqlite3 STATIC
    ${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c
)
target_include_directories(sqlite3
    PUBLIC $<BUILD_INTERFACE:${sqlite_amalgamation_SOURCE_DIR}>
)
target_compile_definitions(sqlite3 PUBLIC
    SQLITE_THREADSAFE=1
    SQLITE_OMIT_LOAD_EXTENSION=1
    SQLITE_DQS=0
)
# Suppress all warnings for third-party C code.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(sqlite3 PRIVATE -w)
endif()
# Linux needs pthreads and dl for SQLite.
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(sqlite3 PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()

# ---------------------------------------------------------------------------
# miniz - ZIP inflate for in-memory XLSX import. MIT license (compatible).
# Built as a static lib from source; examples/install are disabled.
# ---------------------------------------------------------------------------
message(STATUS "[tracker] Fetching miniz ...")
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_FUZZERS OFF CACHE BOOL "" FORCE)
set(INSTALL_PROJECT OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG        3.0.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(miniz)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(miniz PRIVATE -w)
endif()

# ---------------------------------------------------------------------------
# doctest - lightweight, header-only unit testing framework. Only fetched when
# tests are enabled, so a default release build doesn't pull it.
# ---------------------------------------------------------------------------
if(TRACKER_BUILD_TESTS)
    message(STATUS "[tracker] Fetching doctest ...")
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        v2.4.11
        GIT_SHALLOW    TRUE
    )
    set(DOCTEST_WITH_TESTS OFF CACHE INTERNAL "")
    set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(doctest)
endif()

# ---------------------------------------------------------------------------
# wxWidgets - cross-platform UI toolkit.
# ---------------------------------------------------------------------------
if(TRACKER_USE_SYSTEM_WX)
    message(STATUS "[tracker] Using system wxWidgets via find_package")
    find_package(wxWidgets REQUIRED COMPONENTS core base)
    include(${wxWidgets_USE_FILE})
    add_library(wx::wx INTERFACE IMPORTED)
    target_include_directories(wx::wx INTERFACE ${wxWidgets_INCLUDE_DIRS})
    target_compile_definitions(wx::wx INTERFACE ${wxWidgets_DEFINITIONS})
    target_link_libraries(wx::wx INTERFACE ${wxWidgets_LIBRARIES})
else()
    message(STATUS "[tracker] Fetching wxWidgets (slow on first configure) ...")
    set(wxBUILD_SHARED OFF CACHE INTERNAL "")
    set(wxBUILD_PRECOMP OFF CACHE INTERNAL "")
    set(wxBUILD_INSTALL OFF CACHE INTERNAL "")
    set(wxUSE_STL ON CACHE INTERNAL "")
    set(wxUSE_GUI ON CACHE INTERNAL "")
    FetchContent_Declare(
        wxWidgets
        GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
        GIT_TAG        v3.2.5
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(wxWidgets)
    if(NOT TARGET wx::wx)
        add_library(wx::wx INTERFACE IMPORTED)
        target_link_libraries(wx::wx INTERFACE wx::core wx::base)
    endif()
endif()
