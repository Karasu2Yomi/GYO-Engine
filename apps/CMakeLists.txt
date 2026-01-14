function(copy_sdl3_dll target)
    if(WIN32)
        set(_SDL3_RUNTIME_TGT SDL3::SDL3)
        if(TARGET SDL3::SDL3-shared)
            set(_SDL3_RUNTIME_TGT SDL3::SDL3-shared)
        endif()
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${_SDL3_RUNTIME_TGT}>
                $<TARGET_FILE_DIR:${target}>
        )
    endif()
endfunction()

#=====================
# Runtime
#=====================
add_executable(Runtime WIN32 runtime/main.cpp)
target_compile_definitions(Runtime PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(Runtime PRIVATE
    SDL3::SDL3
    imgui::imgui
    nlohmann_json::nlohmann_json
    engine
)
copy_sdl3_dll(Runtime)

#=====================
# Editor
#=====================
add_executable(Editor WIN32 editor/main.cpp)
target_compile_definitions(Editor PRIVATE SDL_MAIN_HANDLED)
# target_link_libraries(Editor PRIVATE Apps::EditorCore)
copy_sdl3_dll(Editor)

add_dependencies(Editor Runtime)