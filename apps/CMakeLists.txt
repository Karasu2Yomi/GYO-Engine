function(gyo_copy_runtime_dlls target)
    if(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${target}>
                $<TARGET_FILE_DIR:${target}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endfunction()

if(GYO_BUILD_RUNTIME)
    add_subdirectory(runtime)
endif()

if(GYO_BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()

if(GYO_BUILD_OBJECT_FPS)
    add_subdirectory(object_fps)
endif()
