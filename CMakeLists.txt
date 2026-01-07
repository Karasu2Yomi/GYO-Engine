cmake_minimum_required(VERSION 3.30)
project(Object_Time LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# (Optional) CLion向け: compile_commands.json を出す
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TOOLS "Build tools (editor, etc.)" ON)
option(BUILD_GAME  "Build game executable" ON)


# =========================
# [third_party 導入位置] ここで third_party を読み込む（依存は third_party 側で完結）
# =========================
add_subdirectory(third_party)

# =========================
# ここから自作サブプロジェクト（internal）
# =========================

#add_subdirectory(engine)
#add_subdirectory(systems)
#add_subdirectory(game)
#add_subdirectory(game)

if(BUILD_GAME)
    add_subdirectory(game)
endif()

if(BUILD_TOOLS)
    add_subdirectory(tools/editor)
endif()

# ----------------------------
# App
# ----------------------------
add_executable(${PROJECT_NAME} WIN32
#        apps/editor/EditorMain.cpp
        apps/runtime/main.cpp
)

# SDLの「SDL_main」を使わず自前 main() にする（保険）
target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_HANDLED)

target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL3::SDL3
        imgui_lib
        nlohmann_json::nlohmann_json
)

# Windows: 実行時にSDL3.dllが必要な場合があるので、exeの隣にコピー
if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()