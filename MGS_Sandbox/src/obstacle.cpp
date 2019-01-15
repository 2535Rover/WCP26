#include <assert.h>
#include <math.h>

#include "obstacle.hpp"

// Adapted from http://www.jeffreythompson.org/collision-detection/line-line.php.
static bool line_line_collision(float a0x, float a0y, float a1x, float a1y, float b0x, float b0y, float b1x, float b1y, float* out_intersection_x, float* out_intersection_y) {
    float u0 = ((b1x-b0x)*(a0y-b0y) - (b1y-b0y)*(a0x-b0x)) / ((b1y-b0y)*(a1x-a0x) - (b1x-b0x)*(a1y-a0y));
    float u1 = ((a1x-a0x)*(a0y-b0y) - (a1y-a0y)*(a0x-b0x)) / ((b1y-b0y)*(a1x-a0x) - (b1x-b0x)*(a1y-a0y));

    if (u0 < 0 || u0 > 1 || u1 < 0 || u1 > 1) {
        return false;
    }

    *out_intersection_x = a0x + (u0 * (a1x - a0x));
    *out_intersection_y = a0y + (u0 * (a1y - a0y));
    
    return true;
}

void lidar_scan(float x, float y, float angle, std::vector<Obstacle>& obstacles, float out_points[271], float max_scan_distance) {
    for (int i = -45; i <= 225; i++) {
        float theta = ((float)i + angle) * M_PI / 180.0f;

        float x1 = max_scan_distance * cosf(theta);
        float y1 = max_scan_distance * sinf(theta);

        float min_sq = max_scan_distance * max_scan_distance;

        for (Obstacle obs : obstacles) {
            float isect_x, isect_y, isect0_x = NAN, isect0_y = NAN, isect1_x = NAN, isect1_y = NAN;

            // Top.
            if (line_line_collision(x, y, x1, y1, obs.x - obs.w/2.0f, obs.y - obs.h/2.0f, obs.x + obs.w/2.0f, obs.y - obs.h/2.0f, &isect_x, &isect_y)) {
                if (isnan(isect0_x)) {
                    isect0_x = isect_x;
                    isect0_y = isect_y;
                } else {
                    isect1_x = isect_x;
                    isect1_y = isect_y;
                }
            }

            // Right.
            if (line_line_collision(x, y, x1, y1, obs.x + obs.w/2.0f, obs.y - obs.h/2.0f, obs.x + obs.w/2.0f, obs.y + obs.h/2.0f, &isect_x, &isect_y)) {
                if (isnan(isect0_x)) {
                    isect0_x = isect_x;
                    isect0_y = isect_y;
                } else {
                    isect1_x = isect_x;
                    isect1_y = isect_y;
                }
            }

            // Bottom.
            if (line_line_collision(x, y, x1, y1, obs.x + obs.w/2.0f, obs.y + obs.h/2.0f, obs.x - obs.w/2.0f, obs.y + obs.h/2.0f, &isect_x, &isect_y)) {
                if (isnan(isect0_x)) {
                    isect0_x = isect_x;
                    isect0_y = isect_y;
                } else {
                    isect1_x = isect_x;
                    isect1_y = isect_y;
                }
            }

            // Left.
            if (line_line_collision(x, y, x1, y1, obs.x - obs.w/2.0f, obs.y + obs.h/2.0f, obs.x - obs.w/2.0f, obs.y - obs.h/2.0f, &isect_x, &isect_y)) {
                if (isnan(isect0_x)) {
                    isect0_x = isect_x;
                    isect0_y = isect_y;
                } else {
                    isect1_x = isect_x;
                    isect1_y = isect_y;
                }
            }

            if (isnan(isect0_x)) {
                // nothing.
            } else {
                assert(!isnan(isect0_x) && !isnan(isect0_y) && !isnan(isect1_x) && !isnan(isect1_y));

                float d0_sq = (x - isect0_x)*(x - isect0_x) + (y - isect0_y)*(y - isect0_y);
                float d1_sq = (x - isect1_x)*(x - isect1_x) + (y - isect1_y)*(y - isect1_y);

                assert(d0_sq >= 0);
                assert(d1_sq >= 0);

                if (d0_sq < d1_sq) {
                    if (d0_sq < min_sq) min_sq = d0_sq;
                } else {
                    if (d1_sq < min_sq) min_sq = d1_sq;
                }
            }
        }

        out_points[i + 45] = sqrtf(min_sq);
    }
}