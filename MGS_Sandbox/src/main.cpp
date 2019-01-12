#include <math.h>
#include <stdint.h>

#include <GL/gl.h>
#include <SDL.h>

#include "grid.hpp"

const int WINDOW_WIDTH = 600, WINDOW_HEIGHT = 600;

void fill_hex(int center_x, int center_y, int size) {
    glBegin(GL_POLYGON);

    for (int i = 0; i < 6; i++) {
        float theta = (M_PI / 3.0f) * i;

        glVertex2f(center_x + size * cosf(theta), center_y + size * sinf(theta));
    }

    glEnd();
}

void stroke_hex(int center_x, int center_y, int size, int stroke_width) {
    glLineWidth(stroke_width);

    glBegin(GL_LINE_LOOP);

    for (int i = 0; i < 6; i++) {
        float theta = (M_PI / 3.0f) * i;

        glVertex2f(center_x + size * cosf(theta), center_y + size * sinf(theta));
    }

    glEnd();
}

void stroke_grid_outline(Grid* grid, float grid_size, float pixels_per_meter, float stroke_width) {
    // We calculate the pixel size of a hexagon side as grid_size * pixels_per_meter.

    float pixel_size = grid_size * pixels_per_meter;

    for (int q = -grid->offset; q <= grid->offset; q++) {
        for (int r = -grid->offset; r <= grid->offset; r++) {
            float center_x = pixel_size * (3.0f/2.0f) * q;
            float center_y = pixel_size * ((sqrtf(3.0f)/2.0f) * q + sqrtf(3.0f) * r);

            stroke_hex(center_x, center_y, pixel_size, stroke_width);
        }
    }
}

enum class MovementDirection {
    UP, DOWN, LEFT, RIGHT
};

struct MovementStatus {
    bool pressed;

    uint32_t time_pressed;
};

float lerp(float a, float b, float t) {
    return (1.0f - t) * a + t * b;
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

    SDL_Window* window = SDL_CreateWindow("MGS Sandbox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);

    SDL_GL_SetSwapInterval(1);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0.0f, 0.5f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_MULTISAMPLE);

    float grid_size = 1.0f; // Length of each hexagon side in meters.

    const float MIN_PPM = 10.0f;
    const float MAX_PPM = 200.0f;
    float pixels_per_meter = 30.0f;

    Grid* grid = create_grid(15);

    float translate_x = WINDOW_WIDTH/2.0f, translate_y = WINDOW_HEIGHT/2.0f;

    float movement_speed = 5.0f;

    MovementStatus movement[4]{};

    bool mouse_down = false;

    for (;;) {
        bool should_quit = false;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                should_quit = true;
                break;
            }

            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    should_quit = true;
                    break;
                }

                switch (event.key.keysym.sym) {
                case SDLK_s:
                    movement[(int)MovementDirection::UP] = { true, SDL_GetTicks() };
                    break;
                 case SDLK_w:
                    movement[(int)MovementDirection::DOWN] = { true, SDL_GetTicks() };
                    break;
                case SDLK_d:
                    movement[(int)MovementDirection::LEFT] = { true, SDL_GetTicks() };
                    break;
                case SDLK_a:
                    movement[(int)MovementDirection::RIGHT] = { true, SDL_GetTicks() };
                    break;
                }
            }

            if (event.type == SDL_KEYUP) {
                switch (event.key.keysym.sym) {
                case SDLK_s:
                    movement[(int)MovementDirection::UP] = { false, 0 };
                    break;
                case SDLK_w:
                    movement[(int)MovementDirection::DOWN] = { false, 0 };
                    break;
                case SDLK_d:
                    movement[(int)MovementDirection::LEFT] = { false, 0 };
                    break;
                case SDLK_a:
                    movement[(int)MovementDirection::RIGHT] = { false, 0 };
                    break;
                }
            }

            if (event.type == SDL_MOUSEWHEEL) {
                //float old_ppm = pixels_per_meter;

                if (event.wheel.y > 0) {
                    // Zoom in.

                    pixels_per_meter *= 1.1f;
                } else if (event.wheel.y < 0) {
                    // Zoom out.

                    pixels_per_meter /= 1.1f;
                }

                if (pixels_per_meter < MIN_PPM) {
                    pixels_per_meter = MIN_PPM;
                }

                if (pixels_per_meter > MAX_PPM) {
                    pixels_per_meter = MAX_PPM;
                }
            }

            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouse_down = true;
                }
            }

            if (event.type == SDL_MOUSEBUTTONUP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouse_down = false;
                }
            }

            if (event.type == SDL_MOUSEMOTION) {
                if (mouse_down) {
                    translate_x += event.motion.xrel;
                    translate_y += event.motion.yrel;
                }
            }
        }

        if (should_quit) break;

        {
            MovementStatus up_status = movement[(int)MovementDirection::UP];
            MovementStatus down_status = movement[(int)MovementDirection::DOWN];
            MovementStatus left_status = movement[(int)MovementDirection::LEFT];
            MovementStatus right_status = movement[(int)MovementDirection::RIGHT];

            if (up_status.pressed) {
                if (down_status.pressed) {
                    translate_y += up_status.time_pressed > down_status.time_pressed ? -movement_speed : movement_speed;
                } else {
                    translate_y += -movement_speed;
                }
            } else if (down_status.pressed) {
                if (up_status.pressed) {
                    translate_y += up_status.time_pressed > down_status.time_pressed ? -movement_speed : movement_speed;
                } else {
                    translate_y += movement_speed;
                }
            }

            if (left_status.pressed) {
                if (right_status.pressed) {
                    translate_x += left_status.time_pressed > right_status.time_pressed ? -movement_speed : movement_speed;
                } else {
                    translate_x += -movement_speed;
                }
            } else if (right_status.pressed) {
                if (left_status.pressed) {
                    translate_x += left_status.time_pressed > right_status.time_pressed ? -movement_speed : movement_speed;
                } else {
                    translate_x += movement_speed;
                }
            }
        }

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(translate_x, translate_y, 0.0f);

        float line_thickness = 20.0f * ((pixels_per_meter - MIN_PPM) / (MAX_PPM - MIN_PPM)) + 1.0f;

        glColor3f(1.0f, 0.0f, 0.0f);
        stroke_grid_outline(grid, grid_size, pixels_per_meter, line_thickness);

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}