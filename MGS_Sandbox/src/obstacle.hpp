#include <vector>

struct Obstacle {
    // x and y are the position of the center.
    float x, y, w, h;
};

void lidar_scan(float x, float y, float angle, std::vector<Obstacle>& obstacles, float out_points[271], float max_scan_distance);