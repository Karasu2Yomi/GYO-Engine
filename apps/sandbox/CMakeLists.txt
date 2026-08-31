add_executable(Sandbox WIN32
    main.cpp
)

target_link_libraries(Sandbox PRIVATE
    GYO::Engine
    GYO::PlatformSDL
    GYO::RenderBackendSDL
    imgui::core
    imgui::sdl3_sdlrenderer3
    imgui::demo
    nlohmann_json::nlohmann_json
)

gyo_copy_runtime_dlls(Sandbox)
