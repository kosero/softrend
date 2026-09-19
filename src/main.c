#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

enum {
  WIDTH = 800,
  HEIGHT = 600,
};

typedef struct Vec2 {
  float x, y;
} Vec2;

typedef struct Color {
  float r, g, b, a;
} Color;

#define min(a, b) ((a) < (b) ? (a) : (b))
#define min3(a, b, c) min(min(a, b), c)

#define max(a, b) ((a) > (b) ? (a) : (b))
#define max3(a, b, c) max(max(a, b), c)

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static uint32_t framebuffer[WIDTH * HEIGHT];
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static uint32_t framebuffer_depth[WIDTH * HEIGHT];

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

static inline uint32_t color_to_uint32(Color color) {
  uint8_t r = (uint8_t)(min(max(color.r, 0.0f), 1.0f) * 255.0f);
  uint8_t g = (uint8_t)(min(max(color.g, 0.0f), 1.0f) * 255.0f);
  uint8_t b = (uint8_t)(min(max(color.b, 0.0f), 1.0f) * 255.0f);
  uint8_t a = (uint8_t)(min(max(color.a, 0.0f), 1.0f) * 255.0f);

  return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) |
         (uint32_t)r;
}

static void draw_pixel(Vec2 point, Color color) {
  if (point.x >= 0 && point.x < WIDTH && point.y >= 0 && point.y < HEIGHT) {
    framebuffer[(int)((point.y * WIDTH) + point.x)] = color_to_uint32(color);
  }
}

static void framebuffer_clear(Color color) {
  uint32_t pixel = color_to_uint32(color);
  for (int i = 0; i < WIDTH * HEIGHT; i++) {
    framebuffer[i] = pixel;
  }
}

static inline Color color_lerp(Color c0, Color c1, Color c2, float u, float v,
                               float w) {
  return (Color){
      .r = (c0.r * w) + (c1.r * u) + (c2.r * v),
      .g = (c0.g * w) + (c1.g * u) + (c2.g * v),
      .b = (c0.b * w) + (c1.b * u) + (c2.b * v),
      .a = (c0.a * w) + (c1.a * u) + (c2.a * v),
  };
}

static inline void process_triangle_pixel(Vec2 p0, Vec2 p1, Vec2 p2, Color c0,
                                          Color c1, Color c2, int x, int y,
                                          float det) {
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
    Color pixel_color = color_lerp(c0, c1, c2, u, v, w);
    draw_pixel((Vec2){(float)x, (float)y}, pixel_color);
  }
}

static void draw_triangle_interpolated(Vec2 p0, Vec2 p1, Vec2 p2, Color c0,
                                       Color c1, Color c2) {
  int min_x = min3((int)p0.x, (int)p1.x, (int)p2.x);
  int max_x = max3((int)p0.x, (int)p1.x, (int)p2.x);
  int min_y = min3((int)p0.y, (int)p1.y, (int)p2.y);
  int max_y = max3((int)p0.y, (int)p1.y, (int)p2.y);

  float det = ((p1.y - p2.y) * (p0.x - p2.x)) + ((p2.x - p1.x) * (p0.y - p2.y));

  for (int y = min_y; y <= max_y; y++) {
    for (int x = min_x; x <= max_x; x++) {
      process_triangle_pixel(p0, p1, p2, c0, c1, c2, x, y, det);
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

  Color col_bg = {0.0f, 0.0f, 0.0f, 1.0f};

  Color col_red = {1.0f, 0.0f, 0.0f, 1.0f};
  Color col_green = {0.0f, 1.0f, 0.0f, 1.0f};
  Color col_blue = {0.0f, 0.0f, 1.0f, 1.0f};

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

    framebuffer_clear(col_bg);

    draw_triangle_interpolated(v0, v1, v2, col_red, col_green, col_blue);

    SDL_UpdateTexture(texture, NULL, framebuffer,
                      WIDTH * (int)sizeof(uint32_t));

    SDL_RenderClear(ctx.r);
    SDL_RenderTexture(ctx.r, texture, NULL, NULL);
    SDL_RenderPresent(ctx.r);
  }

  sdl_clean_up(ctx.r, ctx.w);
  return 0;
}
