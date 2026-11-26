#include <SDL3/SDL.h>

#include <iostream>

namespace
{
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
}

int main(int, char**)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "EMBER: SDL initialization failed: " << SDL_GetError() << '\n';
        return 1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!SDL_CreateWindowAndRenderer(
            "EMBER Engine",
            kWindowWidth,
            kWindowHeight,
            SDL_WINDOW_RESIZABLE,
            &window,
            &renderer))
    {
        std::cerr << "EMBER: Window creation failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    std::cout << "EMBER Engine " << "0.1.0" << " started.\n";

    bool isRunning = true;

    while (isRunning)
    {
        SDL_Event event{};

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                isRunning = false;
            }
        }

        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);

        SDL_SetRenderDrawColor(renderer, 24, 27, 36, 255);
        SDL_RenderClear(renderer);

        // Temporary render test: a centered rectangle.
        const SDL_FRect testRect{
            static_cast<float>(windowWidth) * 0.5F - 48.0F,
            static_cast<float>(windowHeight) * 0.5F - 48.0F,
            96.0F,
            96.0F
        };

        SDL_SetRenderDrawColor(renderer, 91, 160, 255, 255);
        SDL_RenderFillRect(renderer, &testRect);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "EMBER Engine shut down cleanly.\n";
    return 0;
}
