#include <math.h>
#include <stdint.h>

#include <vector>

#include <GL/gl.h>
#include <SDL.h>

#include "grid.hpp"

const int WINDOW_WIDTH = 800, WINDOW_HEIGHT = 800;

void fill_hex(float shade) {
    glBegin(GL_POLYGON);

    glColor4f(shade, shade, shade, 1.0f);

    for (int i = 0; i < 6; i++) {
        float theta = (M_PI / 3.0f) * i;

        glVertex2f(cosf(theta), sinf(theta));
    }

    glEnd();
}

void stroke_hex(int stroke_width) {
    glLineWidth(stroke_width);

    glBegin(GL_LINE_LOOP);

    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);

    for (int i = 0; i < 6; i++) {
        float theta = (M_PI / 3.0f) * i;

        glVertex2f(cosf(theta), sinf(theta));
    }

    glEnd();
}

void render_grid(Grid* grid, float stroke_width) {
    for (int q = -grid->offset; q <= grid->offset; q++) {
        for (int r = -grid->offset; r <= grid->offset; r++) {
            glPushMatrix();

            glTranslatef((3.0f/2.0f) * q, (sqrtf(3.0f)/2.0f) * q + sqrtf(3.0f) * r, 0.0f);

            fill_hex(1.0f - grid->get(q, r));
            stroke_hex(stroke_width);

            glPopMatrix();
        }
    }
}

void outline_origin(int stroke_width) {
    glLineWidth(stroke_width);

    glBegin(GL_LINE_LOOP);

    glColor4f(0.0f, 1.0f, 0.0f, 1.0f);

    for (int i = 0; i < 6; i++) {
        float theta = (M_PI / 3.0f) * i;

        glVertex2f(cosf(theta), sinf(theta));
    }

    glEnd();
}

void render_rover(float rover_x, float rover_y, const float rover_width, const float rover_height, float rover_angle) {
    glPushMatrix();

    glTranslatef(rover_x, rover_y, 0.0f);
    glRotatef(rover_angle, 0.0f, 0.0f, -1.0f);
    glScalef(rover_width, rover_height, 1.0f);

    glBegin(GL_QUADS);

    glColor4f(0.0f, 0.0f, 1.0f, 1.0f);

    glVertex2f(0.5f, 0.5f);
    glVertex2f(0.5f, -0.5f);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f(-0.5f, 0.5f);

    glEnd();

    // Draw the "arrow".

    glBegin(GL_TRIANGLES);

    glColor4f(0.0f, 1.0f, 0.0f, 1.0f);

    glVertex2f(0.0f, 0.25f);
    glVertex2f(-0.25f, 0.0f);
    glVertex2f(0.25f, 0.0f);

    glEnd();

    glPopMatrix();
}

struct Obstacle {
    float x, y, w, h;
};

void render_obstacle(Obstacle* obstacle) {
    glPushMatrix();

    glTranslatef(obstacle->x, obstacle->y, 0.0f);

    glScalef(obstacle->w, obstacle->h, 1.0f);

    glColor4f(1.0f, 0.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);

    glVertex2f(0.5f, 0.5f);
    glVertex2f(0.5f, -0.5f);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f(-0.5f, 0.5f);

    glEnd();

    glPopMatrix();
}

float lerp(float a, float b, float t) {
    return (1.0f - t) * a + t * b;
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

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

    glDisable(GL_DEPTH_TEST);

    float grid_size = 1.0f; // Length of each hexagon side in meters.

    const float MIN_PPM = 10.0f;
    const float MAX_PPM = 200.0f;
    float pixels_per_meter = 30.0f;

    Grid* grid = create_grid(15);

    const float ROVER_WIDTH = 1.0f;
    const float ROVER_HEIGHT = 1.5f;

    const float rover_angle = -180.0f;

    float rover_x = 0, rover_y = 0;

    float translate_x = WINDOW_WIDTH/2.0f, translate_y = WINDOW_HEIGHT/2.0f;

    std::vector<Obstacle> obstacles;

    obstacles.push_back({ 0, -5, 2, 1 });

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
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    mouse_down = true;
                }
            }

            if (event.type == SDL_MOUSEBUTTONUP) {
                if (event.button.button == SDL_BUTTON_RIGHT) {
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

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(translate_x, translate_y, 0.0f);
        glScalef(pixels_per_meter, pixels_per_meter, 1.0f);

        {
            // Grid display.
            
            glPushMatrix();
            glScalef(grid_size, grid_size, 1.0f);

            float line_thickness = 20.0f * ((pixels_per_meter - MIN_PPM) / (MAX_PPM - MIN_PPM)) + 2.0f;

            render_grid(grid, line_thickness);
            outline_origin(line_thickness);

            glPopMatrix();
        }

        for (Obstacle obs : obstacles) {
            render_obstacle(&obs);
        }

        render_rover(rover_x, rover_y, ROVER_WIDTH, ROVER_HEIGHT, rover_angle);

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}