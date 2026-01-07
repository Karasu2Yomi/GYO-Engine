# =========================
# [third_party 導入位置] ここで各 3rd party を add_subdirectory
# =========================

# SDL3（submodule等で third_party/SDL3 に置いた想定）
# ※ 置き場所が違うならパスを変更してください
add_subdirectory(sdl3)

# nlohmann/json（header-onlyだが CMake target を出してくれる）
add_subdirectory(nlohmann_json)

# ImGui（この回答内の third_party/imgui/CMakeLists.txt を使う想定）
add_subdirectory(imgui)