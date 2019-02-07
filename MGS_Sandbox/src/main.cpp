#include <math.h>
#include <stdint.h>

#include <vector>

#include <GL/gl.h>
#include <SDL.h>

#include "grid.hpp"
#include "obstacle.hpp"

const int WINDOW_WIDTH = 800, WINDOW_HEIGHT = 800;

struct OccupancyGrid {
	float side_size;

	int size;

	int* data;
};

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

void render_occupancy_grid(OccupancyGrid* occupancy_grid, float rover_x, float rover_y, float rover_angle) {
	int max = 1;
	for (int i = 0; i < occupancy_grid->size * occupancy_grid->size; i++) {
		if (occupancy_grid->data[i] > max) {
			max = occupancy_grid->data[i];
		}
	}

	glPushMatrix();

	int hs = occupancy_grid->size / 2;

	glTranslatef(rover_x, rover_y, 0.0f);
	glRotatef(rover_angle, 0.0f, 0.0f, -1.0f);
	glScalef(occupancy_grid->side_size, occupancy_grid->side_size, 1.0f);
	glTranslatef(-hs, -hs, 0.0f);


	for (int x = 0; x < occupancy_grid->size; x++) {
		for (int y = 0; y < occupancy_grid->size; y++) {
			int idx = y * occupancy_grid->size + x;

			glColor4f(0.0f, 0.0f, 0.0f, (float)occupancy_grid->data[idx] / (float)max);
			//glColor4f(0.0f, 0.0f, 0.0f, 0.25f);

			glBegin(GL_QUADS);

			glVertex2f(x, y);
			glVertex2f(x + 1, y);
			glVertex2f(x + 1, y + 1);
			glVertex2f(x, y + 1);

			glEnd();
		}
	}

	glPopMatrix();
}

void render_lidar_range(float rover_x, float rover_y, float rover_angle) {
    glPushMatrix();

    glTranslatef(rover_x, rover_y, 0.0f);
    glRotatef(rover_angle, 0.0f, 0.0f, -1.0f);
    glScalef(10.0f, 10.0f, 1.0f);

    glColor4f(1.0f, 0.564f, 0.141f, 0.75f);

    glBegin(GL_TRIANGLE_FAN);

    glVertex2f(0.0f, 0.0f);

    for (int i = -45; i <= 225; i++) {
        float theta = i * M_PI / 180.0f;

        glVertex2f(cosf(theta), sinf(theta));
    }

    glEnd();

    glPopMatrix();
}

void render_lidar_point(float rover_x, float rover_y, float rover_angle, float point_angle, float distance, float pixels_per_meter) {
    glPushMatrix();

    glTranslatef(rover_x, rover_y, 0.0f);
    glRotatef(rover_angle, 0.0f, 0.0f, -1.0f);
    glScalef(distance, distance, 1.0f);

    glColor4f(0.0f, 1.0f, 0.0f, 1.0f);

    glBegin(GL_QUADS);

    float theta = point_angle * M_PI / 180.0f;

    float hw = 2.0f / (pixels_per_meter * distance);

    glVertex2f(cosf(theta) + hw, sinf(theta) + hw);
    glVertex2f(cosf(theta) + hw, sinf(theta) - hw);
    glVertex2f(cosf(theta) - hw, sinf(theta) - hw);
    glVertex2f(cosf(theta) - hw, sinf(theta) + hw);

    glEnd();

    glPopMatrix();
}

float lerp(float a, float b, float t) {
    return (1.0f - t) * a + t * b;
}

void save_level(FILE* out_file, std::vector<Obstacle>& obstacles) {
	for (Obstacle obs : obstacles) {
		fprintf(out_file, "obstacle %f %f %f %f\n", obs.x, obs.y, obs.w, obs.h);
	}
}

bool read_line(FILE* in_file, char* line) {
	for (;;) {
		char c;
		fread(&c, 1, 1, in_file);

		if (c == '\n') return true;
		if (feof(in_file)) return false;

		*line = c;
		line++;
	}
}

void load_level(FILE* in_file, std::vector<Obstacle>& obstacles) {
	char line[1024];

	while (true) {
		if (!read_line(in_file, line)) break;

		if (strncmp(line, "obstacle", 8) == 0) {
			char* lptr = line + 8;
			float x = strtof(lptr, &lptr);
			float y = strtof(lptr, &lptr);
			float w = strtof(lptr, &lptr);
			float h = strtof(lptr, &lptr);

			obstacles.push_back({ x, y, w, h });
		}
	}
}


OccupancyGrid* create_occupancy_grid(float side_size, int size) {
	OccupancyGrid* grid = new OccupancyGrid;

	grid->side_size = side_size;
	grid->size = size;

	grid->data = new int[size * size];

	return grid;
}

int main(int argc, char** argv) {
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

    Grid* grid = create_grid(30);

	const float OCC_GRID_SIDE_SIZE = 0.25f;
	OccupancyGrid* occupancy_grid = create_occupancy_grid(OCC_GRID_SIDE_SIZE, ceilf(20.0f / OCC_GRID_SIDE_SIZE));

    const float ROVER_WIDTH = 1.0f;
    const float ROVER_HEIGHT = 1.5f;
	const float ROVER_SPEED = 0.5f;

    float rover_angle = -180.0f;

    float rover_x = 0, rover_y = 0;

	float rover_dangle = 0;
	float rover_speed = 0;

    float translate_x = (WINDOW_WIDTH/2.0f)/pixels_per_meter, translate_y = (WINDOW_HEIGHT/2.0f)/pixels_per_meter;

    std::vector<Obstacle> obstacles;

	if (argc > 1) {
		FILE* in_file = fopen(argv[1], "r");
		printf("> Loading level %s\n", argv[1]);
		load_level(in_file, obstacles);
		fclose(in_file);
	}

    float lidar_points[271];

    bool right_mouse_down = false;

	bool display_grid = true;
	bool display_lidar = true;
	bool display_obstacles = true;
	bool display_occupancy_grid = true;

    Obstacle drag_obstacle;
    bool dragging = false;

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
                } else if (event.key.keysym.sym == SDLK_u) {
                    if (obstacles.size() != 0) {
                        printf("> Undoing last placed obstacle.\n");

                        obstacles.pop_back();
                    }
                } else if (event.key.keysym.sym == SDLK_r) {
                    printf("> Resetting camera position.\n");

                    translate_x = (WINDOW_WIDTH/2.0f)/pixels_per_meter;
                    translate_y = (WINDOW_HEIGHT/2.0f)/pixels_per_meter;
                } else if (event.key.keysym.sym == SDLK_s) {
					printf("> Saving current level.\n");

					FILE* level_file = fopen("level.mgslevel", "w");
					save_level(level_file, obstacles);
					fclose(level_file);
				} else if (event.key.keysym.sym == SDLK_g) {
					auto mod_state = SDL_GetModState();

					if (mod_state & KMOD_SHIFT) {
						display_occupancy_grid = !display_occupancy_grid;
					} else {
						display_grid = !display_grid;
					}
				} else if (event.key.keysym.sym == SDLK_l) {
					display_lidar = !display_lidar;
				} else if (event.key.keysym.sym == SDLK_o) {
					display_obstacles = !display_obstacles;
				} else if (event.key.keysym.sym == SDLK_UP) {
					rover_speed = -ROVER_SPEED / 5.0f;
				} else if (event.key.keysym.sym == SDLK_DOWN) {
					rover_speed = ROVER_SPEED / 5.0f;
				} else if (event.key.keysym.sym == SDLK_LEFT) {
					rover_dangle = ROVER_SPEED;
				} else if (event.key.keysym.sym == SDLK_RIGHT) {
					rover_dangle = -ROVER_SPEED;
				}
            }

			if (event.type == SDL_KEYUP) {
				if (event.key.keysym.sym == SDLK_UP) {
					rover_speed = 0;
				} else if (event.key.keysym.sym == SDLK_DOWN) {
					rover_speed = 0;
				} else if (event.key.keysym.sym == SDLK_LEFT) {
					rover_dangle = 0;
				} else if (event.key.keysym.sym == SDLK_RIGHT) {
					rover_dangle = 0;
				}
			}

            if (event.type == SDL_MOUSEWHEEL) {
                //float old_ppm = pixels_per_meter;  

                int mx, my;
                SDL_GetMouseState(&mx, &my);

                float old_x = (mx/pixels_per_meter) - translate_x;
                float old_y = (my/pixels_per_meter) - translate_y;              

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

                float new_x = (mx/pixels_per_meter) - translate_x;
                float new_y = (my/pixels_per_meter) - translate_y;  

                translate_x += new_x - old_x;
                translate_y += new_y - old_y;
            }

            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    if (dragging) {
                        printf("[!] Cancelling drag.\n");

                        dragging = false;
                    } else {
                        right_mouse_down = true;
                    }
                } else if (event.button.button == SDL_BUTTON_LEFT) {
                    dragging = true;

                    int mx, my;
                    SDL_GetMouseState(&mx, &my);

                    float fmx = mx, fmy = my;

                    drag_obstacle.x = (fmx / pixels_per_meter) - translate_x;
                    drag_obstacle.y = (fmy / pixels_per_meter) - translate_y;

                    drag_obstacle.w = 0;
                    drag_obstacle.h = 0;
                }
            }

            if (event.type == SDL_MOUSEBUTTONUP) {
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    right_mouse_down = false;
                } else if (event.button.button == SDL_BUTTON_LEFT && dragging) {
                    dragging = false;

                    if (drag_obstacle.w < 1e-6 || drag_obstacle.h < 1e-6) {
                        printf("[!] Not adding an obstacle: too small!\n");
                    } else {
                        obstacles.push_back(drag_obstacle);                        
                    }
                }
            }

            if (event.type == SDL_MOUSEMOTION) {
                if (right_mouse_down) {
                    translate_x += event.motion.xrel/pixels_per_meter;
                    translate_y += event.motion.yrel/pixels_per_meter;
                }

                if (dragging) {
                    int mx, my;
                    SDL_GetMouseState(&mx, &my);

                    float fmx = mx, fmy = my;

                    float world_x = (fmx / pixels_per_meter) - translate_x;
                    float world_y = (fmy / pixels_per_meter) - translate_y;

                    drag_obstacle.w = 2.0f * fabsf(world_x - drag_obstacle.x);
                    drag_obstacle.h = 2.0f * fabsf(world_y - drag_obstacle.y);
                }
            }
        }

        if (should_quit) break;

		rover_angle += rover_dangle;
		rover_x += rover_speed * cosf((rover_angle + 90) * M_PI / 180.0f);
		rover_y += rover_speed * sinf((rover_angle + 90) * M_PI / 180.0f);

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
		
        glScalef(pixels_per_meter, pixels_per_meter, 1.0f);
        glTranslatef(translate_x, translate_y, 0.0f);

        if (display_grid) {
            // Grid display.
            
            glPushMatrix();
            glScalef(grid_size, grid_size, 1.0f);

            float line_thickness = 20.0f * ((pixels_per_meter - MIN_PPM) / (MAX_PPM - MIN_PPM)) + 2.0f;

            render_grid(grid, line_thickness);
            outline_origin(line_thickness);

            glPopMatrix();
        }

        if (dragging) {
            render_obstacle(&drag_obstacle);
        }

		if (display_obstacles) {
			for (Obstacle obs : obstacles) {
				render_obstacle(&obs);
			}
		}


		if (display_lidar) render_lidar_range(rover_x, rover_y, rover_angle);

        render_rover(rover_x, rover_y, ROVER_WIDTH, ROVER_HEIGHT, rover_angle);

        lidar_scan(rover_x, rover_y, rover_angle, obstacles, lidar_points, 20.0f);

		// Update the occupancy grid.
		memset(occupancy_grid->data, 0, sizeof(int) * occupancy_grid->size * occupancy_grid->size);

		for (int i = -45; i <= 225; i++) {
			float distance = lidar_points[i + 45];

			if (distance <= 10.0f)  {
				if (display_lidar) render_lidar_point(rover_x, rover_y, rover_angle, i, distance, pixels_per_meter);

				// Add to occupancy grid.
				// First, get position-centric cartesian coordinates.
				float lp_x = distance * cosf((float)i * M_PI / 180.0f);
				float lp_y = distance * sinf((float)i * M_PI / 180.0f);

				lp_x += occupancy_grid->side_size * (occupancy_grid->size / 2);
				lp_y += occupancy_grid->side_size * (occupancy_grid->size / 2);

				// What grid cell?

				int gc_x = lp_x / occupancy_grid->side_size;
				int gc_y = lp_y / occupancy_grid->side_size;

				occupancy_grid->data[gc_y * occupancy_grid->size + gc_x]++;
			}
		}

		if (display_occupancy_grid) render_occupancy_grid(occupancy_grid, rover_x, rover_y, rover_angle);

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
