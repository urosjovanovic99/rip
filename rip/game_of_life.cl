__kernel void game_of_life(__global const int* grid_in,
                           __global int* grid_out,
                           const int width,
                           const int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    int idx = y * width + x;
    int live_neighbors = 0;

    // Check 8 neighbors with wrap-around (toroidal grid)
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;

            int nx = (x + dx + width) % width;
            int ny = (y + dy + height) % height;
            int nidx = ny * width + nx;

            live_neighbors += grid_in[nidx];
        }
    }

    int cell = grid_in[idx];
    int new_state = 0;

    // Conway's rules
    if (cell == 1 && (live_neighbors == 2 || live_neighbors == 3)) {
        new_state = 1; // survives
    } else if (cell == 0 && live_neighbors == 3) {
        new_state = 1; // birth
    }

    grid_out[idx] = new_state;
}