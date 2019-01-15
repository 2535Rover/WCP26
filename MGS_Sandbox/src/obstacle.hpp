#include <vector>

struct Obstacle {
    float x, y, w, h;
};

void lidar_scan(float x, float y, float angle, std::vector<Obstacle>& obstacles, float out_points[270]);