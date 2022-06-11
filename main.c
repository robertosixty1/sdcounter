#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "image.h"

void scc(int code)
{
    if (code < 0)
    {
        fprintf(stderr, "ERROR: SDL pooped itself: %s\n",
                SDL_GetError());
        exit(1);
    }
}

void *sccp(void *ptr)
{
    if (ptr == NULL) {
        fprintf(stderr, "SDL pooped itself: %s\n", SDL_GetError());
        abort();
    }

    return ptr;
}

typedef enum {
    MODE_ASCENDING,
    MODE_COUNTDOWN,
    MODE_CLOCK
} Mode;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* digits;
    int char_x;
    int char_y;
    int sprite;
    float displayed_time;
    bool quit;
    bool paused;
    Mode mode;
} Context;

#define COLON_INDEX         10
#define FPS                 60
#define DELTA_TIME         (1.0f / FPS)
#define MAIN_COLOR_R        220
#define MAIN_COLOR_G        220
#define MAIN_COLOR_B        220
#define BACKGROUND_COLOR_R  24
#define BACKGROUND_COLOR_G  24
#define BACKGROUND_COLOR_B  24
#define WIDTH               800
#define HEIGHT              600
#define SPRITE_CHAR_WIDTH  (300 / 2)
#define SPRITE_CHAR_HEIGHT (380 / 2)
#define CHAR_WIDTH         (300 / 2)
#define CHAR_HEIGHT        (380 / 2)
#define CHARS_COUNT         8
#define TEXT_WIDTH         (CHAR_WIDTH * CHARS_COUNT)
#define TEXT_HEIGHT        (CHAR_HEIGHT)
#define TEXT_SCALE          1
#define TEXT_FIT_SCALE      1
#define SPRITE_DURATION    (0.40f / SPRITE_COUNT)
#define SPRITE_COUNT        3

SDL_Surface* load_png_file_as_surface()
{
    SDL_Surface* image_surface =
        sccp(SDL_CreateRGBSurfaceFrom(
                 png,
                 (int) png_width,
                 (int) png_height,
                 32,
                 (int) png_width * 4,
                 0x000000FF,
                 0x0000FF00,
                 0x00FF0000,
                 0xFF000000));
    return image_surface;
}

SDL_Texture* load_png_file_as_texture(Context* context)
{
    SDL_Surface* image_surface = load_png_file_as_surface();
    return sccp(SDL_CreateTextureFromSurface(context->renderer, image_surface));
}

void context_init_sdl(Context* context)
{
    scc(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear"));
    scc(SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0"));

    context->window = SDL_CreateWindow("Counter", 0, 0,
                                       WIDTH,
                                       HEIGHT,
                                       SDL_WINDOW_RESIZABLE);
    context->renderer = SDL_CreateRenderer(
        context->window, -1,
        SDL_RENDERER_PRESENTVSYNC
        | SDL_RENDERER_ACCELERATED);

    context->digits = load_png_file_as_texture(context);
    scc(SDL_SetTextureColorMod(context->digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));

    context->char_x = 1;
    context->char_y = 1;
}

void context_render_char(Context* context, int number,
                         float user_scale, float fit_scale)
{
    if (context->sprite >= 3) context->sprite = 0;

    const int effective_digit_width = (int) floorf((float) CHAR_WIDTH * user_scale * fit_scale);
    const int effective_digit_height = (int) floorf((float) CHAR_HEIGHT * user_scale * fit_scale);

    const SDL_Rect src = {
        (int) (number * SPRITE_CHAR_WIDTH),
        (int) (context->sprite * SPRITE_CHAR_HEIGHT),
        SPRITE_CHAR_WIDTH,
        SPRITE_CHAR_HEIGHT
    };
    const SDL_Rect dst = {
        context->char_x,
        context->char_y,
        effective_digit_width,
        effective_digit_height
    };
    SDL_RenderCopy(context->renderer, context->digits, &src, &dst);
    context->char_x += effective_digit_width;
}

int main(void)
{
    scc(SDL_Init(SDL_INIT_EVERYTHING));

    Context context = {0};
    context_init_sdl(&context);

    SDL_SetRenderDrawColor(context.renderer,
                           BACKGROUND_COLOR_R,
                           BACKGROUND_COLOR_G,
                           BACKGROUND_COLOR_B,
                           255);

    float sprite_cooldown = SPRITE_DURATION;
    while (!context.quit)
    {
        SDL_Event event = {0};
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                context.quit = true;
                break;
            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                case SDLK_F11: {
                    Uint32 window_flags;
                    scc(window_flags = SDL_GetWindowFlags(context.window));
                    if (window_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                        scc(SDL_SetWindowFullscreen(context.window, 0));
                    } else {
                        scc(SDL_SetWindowFullscreen(context.window, SDL_WINDOW_FULLSCREEN_DESKTOP));
                    }
                } break;
                }
            } break;
            }
        }

        SDL_RenderClear(context.renderer);
        {
            context.char_x = 0;
            context.char_y = 0;

            size_t t = (size_t) ceilf(fmaxf(context.displayed_time, 0.0f));

            const size_t hours = t / 60 / 60;
            context_render_char(&context, hours / 10,  TEXT_SCALE, TEXT_FIT_SCALE);
            context_render_char(&context, hours % 10,  TEXT_SCALE, TEXT_FIT_SCALE);
            context_render_char(&context, COLON_INDEX, TEXT_SCALE, TEXT_FIT_SCALE);

            const size_t minutes = t / 60 % 60;
            context_render_char(&context, minutes / 10, TEXT_SCALE, TEXT_FIT_SCALE);
            context_render_char(&context, minutes % 10, TEXT_SCALE, TEXT_FIT_SCALE);
            context_render_char(&context, COLON_INDEX,  TEXT_SCALE, TEXT_FIT_SCALE);

            const size_t seconds = t % 60;
            context_render_char(&context, seconds / 10, TEXT_SCALE, TEXT_FIT_SCALE);
            context_render_char(&context, seconds % 10, TEXT_SCALE, TEXT_FIT_SCALE);
        }
        SDL_RenderPresent(context.renderer);

        if (sprite_cooldown <= 0.0f) {
            context.sprite++;
            sprite_cooldown = SPRITE_DURATION;
        }
        sprite_cooldown -= DELTA_TIME;

        if (!context.paused) {
            switch (context.mode) {
            case MODE_ASCENDING: {
                context.displayed_time += DELTA_TIME;
            } break;
            case MODE_COUNTDOWN: {
                if (context.displayed_time > 1e-6) {
                    context.displayed_time -= DELTA_TIME;
                } else {
                    context.displayed_time = 0.0f;
                }
            } break;
            case MODE_CLOCK: {
                time_t t = time(NULL);
                struct tm *tm = localtime(&t);
                context.displayed_time = tm->tm_sec
                               + tm->tm_min  * 60.0f
                               + tm->tm_hour * 60.0f * 60.0f;
                break;
            }
            }
        }

        SDL_Delay((int) floorf(DELTA_TIME * 1000.0f));
    }

    SDL_Quit();
    return 0;
}