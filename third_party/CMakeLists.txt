# =========================
# [third_party 導入位置] ここで各 3rd party を add_subdirectory
# =========================

# SDL3（submodule等で third_party/SDL3 に置いた）
add_subdirectory(sdl3)

# nlohmann/json（header-onlyだが CMake target を出してくれる）
add_subdirectory(nlohmann_json)

# ImGui（third_party/imgui/CMakeLists.txt を使う）
add_subdirectory(imgui)

# doctest（third_party/doctest/CMakeLists.txt を使う）
add_subdirectory(doctest)