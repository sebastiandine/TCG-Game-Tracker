# Unit tests for tracker_core. Wx and infra-bound tests are out of scope here -
# everything in this target operates against the tracker_core ports/services
# using in-memory fakes, so the suite is fast and hermetic.

add_executable(tracker_core_tests
    fakes/InMemoryFileSystem.cpp

    result_tests.cpp
    domain_json_tests.cpp
    config_service_tests.cpp
    format_service_tests.cpp
    sqlite_format_repository_tests.cpp
    game_title_service_tests.cpp
    sqlite_game_title_repository_tests.cpp
    sqlite_schema_migration_tests.cpp
    game_type_service_tests.cpp
    sqlite_game_type_repository_tests.cpp
    deck_service_tests.cpp
    sqlite_deck_repository_tests.cpp
    deck_grouping_tests.cpp
    deck_statistics_tests.cpp
    game_service_tests.cpp
    sqlite_game_repository_tests.cpp
    game_import_parse_tests.cpp
    game_import_service_tests.cpp

    main.cpp
)

target_include_directories(tracker_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(tracker_core_tests
    PRIVATE
        tracker_core
        sqlite3
        doctest::doctest
        tracker_warnings
)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(tracker_core_tests)
