#include "grid.hpp"

Grid* create_grid(int radius) {
    int size = radius * 2 + 1;
    float* grid_memory = new float[size * size];

    for (int i = 0; i < size * size; i++) {
        grid_memory[i] = 0.0f;
    }

    return new Grid{ size, radius, grid_memory };
}