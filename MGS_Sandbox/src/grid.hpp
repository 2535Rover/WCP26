/*
    Contains routines for dealing with a hexagon grid.
    Nearly everything to do with hexagon grids comes from https://www.redblobgames.com/grids/hexagons/.

    Our grid uses the axial coordinate system.
*/

struct Grid {
    int size;

    int offset;

    float* grid;
};

// Creates a rhombus large enough to support a circle with the given radius.
// With axial coordinates, the size of the rhombus when stored in a rectangular array is equivalent to twice the radius plus 1.
// The offset is just the radius, in q and r.
Grid* create_grid(int radius);