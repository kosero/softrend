#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

#define WIDTH 800
#define HEIGHT 600

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static uint32_t framebuffer[WIDTH * HEIGHT];

static void sdl_clean_up(SDL_Renderer *r, SDL_Window *w) {
  SDL_DestroyRenderer(r);
  SDL_DestroyWindow(w);
  SDL_Quit();
}

int main(void) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("SDL could not be initialized: %s", SDL_GetError());
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("softrend", WIDTH, HEIGHT, 0);
  if (!window) {
    SDL_Log("Window could not be created: %s", SDL_GetError());
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    SDL_Log("Renderer could not be created: %s", SDL_GetError());
    return 1;
  }

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
  if (!texture) {
    SDL_Log("Texture could not be created: %s", SDL_GetError());
    sdl_clean_up(renderer, window);
    return 1;
  }

  bool running = true;
  SDL_Event event;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {
        uint8_t r = (uint8_t)(x % 256);
        uint8_t g = (uint8_t)(y % 256);
        uint8_t b = 128;
        uint8_t a = 255;

        framebuffer[(y * WIDTH) + x] = ((uint32_t)r << 24) |
                                       ((uint32_t)g << 16) |
                                       ((uint32_t)b << 8) | (uint32_t)a;
      }
    }

    SDL_UpdateTexture(texture, NULL, framebuffer, (int)sizeof(uint32_t));

    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  sdl_clean_up(renderer, window);
  return 0;
}
