#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define WIDTH 800
#define HEIGHT 600

typedef struct Vec2 {
  float x, y;
} Vec2;

#define min(a, b) ((a) < (b) ? (a) : (b))
#define min3(a, b, c) min(min(a, b), c)

#define max(a, b) ((a) > (b) ? (a) : (b))
#define max3(a, b, c) max(max(a, b), c)

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

static void draw_pixel(Vec2 point, uint32_t color) {
  if (point.x >= 0 && point.x < WIDTH && point.y >= 0 && point.y < HEIGHT) {
    framebuffer[(int)((point.y * WIDTH) + point.x)] = color;
  }
}

static void framebuffer_clear(uint32_t color) {
  for (int i = 0; i < WIDTH * HEIGHT; i++) {
    framebuffer[i] = color;
  }
}

static void draw_line(Vec2 p0, Vec2 p1, uint32_t color) {
  int dx = abs((int)(p1.x - p0.x));
  int dy = abs((int)(p1.y - p0.y));
  int sx = (p0.x < p1.x) ? 1 : -1;
  int sy = (p0.y < p1.y) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    draw_pixel(p0, color);

    if (p0.x == p1.x && p0.y == p1.y) {
      break;
    }

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      p0.x += (float)sx;
    }
    if (e2 < dx) {
      err += dx;
      p0.y += (float)sy;
    }
  }
}

static inline void process_triangle_pixel(Vec2 p0, Vec2 p1, Vec2 p2, int x,
                                          int y, float det, uint32_t color) {
  if (det == 0.0f) {
    return;
  }

  float u = (((p1.y - p2.y) * ((float)x - p2.x)) +
             ((p2.x - p1.x) * ((float)y - p2.y))) /
            det;
  float v = (((p2.y - p0.y) * ((float)x - p2.x)) +
             ((p0.x - p2.x) * ((float)y - p2.y))) /
            det;
  float w = 1.0f - u - v;

  if (u >= 0.0f && v >= 0.0f && w >= 0.0f) {
    draw_pixel((Vec2){(float)x, (float)y}, color);
  }
}

static void draw_triangle(Vec2 p0, Vec2 p1, Vec2 p2, uint32_t color) {
  int min_x = min3((int)p0.x, (int)p1.x, (int)p2.x);
  int max_x = max3((int)p0.x, (int)p1.x, (int)p2.x);
  int min_y = min3((int)p0.y, (int)p1.y, (int)p2.y);
  int max_y = max3((int)p0.y, (int)p1.y, (int)p2.y);

  float det = ((p1.y - p2.y) * (p0.x - p2.x)) + ((p2.x - p1.x) * (p0.y - p2.y));

  for (int y = min_y; y <= max_y; y++) {
    for (int x = min_x; x <= max_x; x++) {
      process_triangle_pixel(p0, p1, p2, x, y, det, color);
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

  Vec2 v0 = (Vec2){400, 100};
  Vec2 v1 = (Vec2){200, 500};
  Vec2 v2 = (Vec2){600, 500};

  bool running = true;
  SDL_Event event;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    framebuffer_clear(0x000000FF);

    draw_triangle(v0, v1, v2, col_red);

    SDL_UpdateTexture(texture, NULL, framebuffer,
                      WIDTH * (int)sizeof(uint32_t));

    SDL_RenderClear(ctx.r);
    SDL_RenderTexture(ctx.r, texture, NULL, NULL);
    SDL_RenderPresent(ctx.r);
  }

  sdl_clean_up(ctx.r, ctx.w);
  return 0;
}
