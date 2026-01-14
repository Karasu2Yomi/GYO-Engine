cmake_minimum_required(VERSION 3.30)
project(Object_Time LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# (Optional) CLion向け: compile_commands.json を出す
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TOOLS "Build tools (editor, etc.)" ON)
option(BUILD_GAME  "Build game executable" ON)
option(RUN_TEST_CASE  "Test case executable" ON)


# =========================
# [third_party 導入位置] ここで third_party を読み込む（依存は third_party 側で完結）
# =========================
add_subdirectory(third_party)

# =========================
# ここから自作サブプロジェクト（internal）
# =========================

add_subdirectory(engine)
#add_subdirectory(systems)
#add_subdirectory(game)
#add_subdirectory(game)



# ----------------------------
# App
# ----------------------------
# App 実装（main なし）
if(BUILD_GAME)
    add_subdirectory(game)
endif()

if(BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(RUN_TEST_CASE)
    add_subdirectory(tests)
endif ()

# 実行入口（main あり）
add_subdirectory(apps)
