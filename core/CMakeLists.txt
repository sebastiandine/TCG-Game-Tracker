# tracker_core: UI-agnostic domain, services, ports, and infra adapters.
# This target MUST NOT depend on wxWidgets or any UI toolkit.

add_library(tracker_core STATIC
    src/domain/Enums.cpp
    src/domain/Configuration.cpp
    src/domain/DeckGroup.cpp
    src/domain/DeckStatistics.cpp

    src/services/ConfigService.cpp
    src/services/GameTitleService.cpp
    src/services/FormatService.cpp
    src/services/DeckService.cpp
    src/services/GameTypeService.cpp
    src/services/GameService.cpp
    src/services/GameImportService.cpp

    src/import/Csv.cpp
    src/import/Xlsx.cpp
    src/import/GameImportMapper.cpp

    src/infra/StdFileSystem.cpp
    src/infra/SqliteDatabase.cpp
    src/infra/SqliteGameTitleRepository.cpp
    src/infra/SqliteFormatRepository.cpp
    src/infra/SqliteDeckRepository.cpp
    src/infra/SqliteGameTypeRepository.cpp
    src/infra/SqliteGameRepository.cpp
)

target_include_directories(tracker_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(tracker_core
    PUBLIC
        nlohmann_json::nlohmann_json
    PRIVATE
        sqlite3
        miniz
        tracker_warnings
)

target_compile_features(tracker_core PUBLIC cxx_std_20)
