add_executable(Runtime WIN32
    main.cpp
)

target_link_libraries(Runtime PRIVATE
    GYO::Engine
    GYO::PlatformSDL
    GYO::RenderBackendSDL
)

gyo_copy_runtime_dlls(Runtime)

if(BUILD_TESTING)
    add_test(NAME runtime_smoke COMMAND Runtime --smoke-test)
    set_tests_properties(runtime_smoke PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software"
        TIMEOUT 15
    )
endif()
