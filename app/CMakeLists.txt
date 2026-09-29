# tracker: thin executable / composition root.
# Wires concrete adapters into the services and hands them to the UI.

add_executable(tracker
    main.cpp
)
set_target_properties(tracker PROPERTIES OUTPUT_NAME tcg-game-tracker)

# Subsystem WINDOWS on Windows so we don't get a stray console.
if(WIN32)
    set_target_properties(tracker PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_link_libraries(tracker
    PRIVATE
        tracker_core
        tracker_ui_wx
        tracker_warnings
)

# MinGW-w64: ship the toolchain runtime next to the exe so Explorer / IDE
# launches do not pick a mismatched libstdc++ off PATH.
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    get_filename_component(_tracker_mingw_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    foreach(_tracker_rt_dll IN ITEMS libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
        if(EXISTS "${_tracker_mingw_bin}/${_tracker_rt_dll}")
            add_custom_command(TARGET tracker POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_tracker_mingw_bin}/${_tracker_rt_dll}"
                        "$<TARGET_FILE_DIR:tracker>/${_tracker_rt_dll}"
                VERBATIM)
        endif()
    endforeach()
endif()
