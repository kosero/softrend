#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define WIDTH 800
#define HEIGHT 600

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static uint32_t framebuffer[WIDTH * HEIGHT];

typedef struct SDL_Context {
  SDL_Renderer *r;
  SDL_Window *w;
} SDL_Context;

static SDL_Context sdl_context_init(void) {
  SDL_Window *w = SDL_CreateWindow("softrend", WIDTH, HEIGHT, 0);
  if (!w) {
    SDL_Log("Window could not be created: %s", SDL_GetError());
    _exit(1);
  }

  SDL_Renderer *r = SDL_CreateRenderer(w, NULL);
  if (!r) {
    SDL_Log("Renderer could not be created: %s", SDL_GetError());
    _exit(1);
  }

  return (SDL_Context){r, w};
}

static void sdl_clean_up(SDL_Renderer *r, SDL_Window *w) {
  SDL_DestroyRenderer(r);
  SDL_DestroyWindow(w);
  SDL_Quit();
}

static void draw_pixel(int x, int y, uint32_t color) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
    framebuffer[(y * WIDTH) + x] = color;
  }
}

static void framebuffer_clear(uint32_t color) {
  for (int i = 0; i < WIDTH * HEIGHT; i++) {
    framebuffer[i] = color;
  }
}

static void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    draw_pixel(x0, y0, color);

    if (x0 == x1 && y0 == y1) {
      break;
    }

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

int main(void) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("SDL could not be initialized: %s", SDL_GetError());
    return 1;
  }

  SDL_Context ctx = sdl_context_init();

  SDL_Texture *texture =
      SDL_CreateTexture(ctx.r, SDL_PIXELFORMAT_RGBA32,
                        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
  if (!texture) {
    SDL_Log("Texture could not be created: %s", SDL_GetError());
    sdl_clean_up(ctx.r, ctx.w);
    return 1;
  }

  const SDL_PixelFormatDetails *fmt_details =
      SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA32);

  uint32_t col_bg = SDL_MapRGBA(fmt_details, NULL, 0, 0, 0, 255);
  uint32_t col_red = SDL_MapRGBA(fmt_details, NULL, 255, 0, 0, 255);
  uint32_t col_green = SDL_MapRGBA(fmt_details, NULL, 0, 255, 0, 255);
  uint32_t col_blue = SDL_MapRGBA(fmt_details, NULL, 0, 0, 255, 255);

  bool running = true;
  SDL_Event event;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    framebuffer_clear(0x000000FF);

    draw_line(400, 100, 200, 500, col_red);
    draw_line(200, 500, 600, 500, col_green);
    draw_line(600, 500, 400, 100, col_blue);

    SDL_UpdateTexture(texture, NULL, framebuffer,
                      WIDTH * (int)sizeof(uint32_t));

    SDL_RenderClear(ctx.r);
    SDL_RenderTexture(ctx.r, texture, NULL, NULL);
    SDL_RenderPresent(ctx.r);
  }

  sdl_clean_up(ctx.r, ctx.w);
  return 0;
}
