#define WIN32_LEAN_AND_MEAN

// windows
#include <windows.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
// imgui
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
// json
#include <nlohmann/json.hpp>
// lib
#include <string>



int GameMain(int, char**)
{
    // --- SDL init ---
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "SDL3 + ImGui Demo",
        1280,
        720,
        SDL_WINDOW_RESIZABLE
    );
    if (!window)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // VSYNC（無ければ無視される実装もある）
    SDL_SetRenderVSync(renderer, 1);

    // --- ImGui init ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    // Platform / Renderer backends
    // ※ ここが SDL3 連動の肝
    if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer))
    {
        SDL_Log("ImGui_ImplSDL3_InitForSDLRenderer failed");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!ImGui_ImplSDLRenderer3_Init(renderer))
    {
        SDL_Log("ImGui_ImplSDLRenderer3_Init failed");
        ImGui_ImplSDL3_Shutdown();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // --- Demo state ---
    bool running = true;
    bool show_demo_window = true;
    float value = 0.5f;

    // nlohmann/json demo data
    nlohmann::json j;
    j["app"] = "sdl3_imgui_demo";
    j["window"] = {{"w", 1280}, {"h", 720}};
    j["value"] = value;

    while (running)
    {
        // --- Events ---
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            ImGui_ImplSDL3_ProcessEvent(&e);

            if (e.type == SDL_EVENT_QUIT)
                running = false;

            // ウィンドウ閉じる
            if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                running = false;
        }

        // --- ImGui new frame ---
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // --- UI ---
        {
            ImGui::Begin("Control");

            ImGui::Checkbox("Show ImGui Demo Window", &show_demo_window);
            ImGui::SliderFloat("value", &value, 0.0f, 1.0f);

            // JSONを更新して表示
            j["value"] = value;
            std::string jsonText = j.dump(2);

            ImGui::Separator();
            ImGui::TextUnformatted("nlohmann/json dump:");
            ImGui::BeginChild("json", ImVec2(0, 200), true);
            ImGui::TextUnformatted(jsonText.c_str());
            ImGui::EndChild();

            ImGui::End();
        }

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // --- Render ---
        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 20, 20, 22, 255);
        SDL_RenderClear(renderer);

        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);
    }

    // --- Shutdown ---
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // SDLのmainラッパを使わない場合に推奨
    SDL_SetMainReady();

    return GameMain(0, nullptr);
}