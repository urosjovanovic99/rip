// game_of_life.cpp
// Compile: g++ -std=c++17 -O2 -pthread -o game_of_life game_of_life.cpp
// Run examples:
//   ./game_of_life            (default 40x80 random, 100 gens, wrap mode)
//   ./game_of_life 30 120 200 100 wrap 0.25
//   ./game_of_life 20 60 0 200 finite 0.12   (0 gens => run until Ctrl-C)

#include <bits/stdc++.h>
#include <thread>
#include <chrono>
using namespace std;

using Grid = vector<vector<uint8_t>>; // 0 or 1

// Print usage
void print_usage(const char* prog) {
    cout << "Usage: " << prog << " [rows cols generations delay_ms mode seed_prob]\n"
        << "  rows, cols        : grid size (default 40 80)\n"
        << "  generations       : number of generations to run (0 = infinite) (default 100)\n"
        << "  delay_ms          : milliseconds between frames (default 100)\n"
        << "  mode              : 'wrap' (toroidal) or 'finite' (default wrap)\n"
        << "  seed_prob         : probability 0..1 to start a live cell (default 0.25)\n         << "Example: " << prog << " 30 120 200 100 wrap 0.2\n";
}

// Clear terminal (ANSI)
void clear_screen() {
    // Cursor to top-left and clear screen
    cout << "\x1b[H\x1b[2J";
}

// Print grid to terminal
void print_grid(const Grid& g) {
    size_t r = g.size();
    size_t c = g.empty() ? 0 : g[0].size();
    for (size_t i = 0; i < r; ++i) {
        for (size_t j = 0; j < c; ++j) {
            cout << (g[i][j] ? 'O' : ' ');
        }
        cout << '\n';
    }
}

// Count neighbors for cell (i,j) with chosen mode
int count_neighbors(const Grid& g, int i, int j, bool wrap) {
    int r = (int)g.size();
    int c = (int)g[0].size();
    int cnt = 0;
    for (int di = -1; di <= 1; ++di) {
        for (int dj = -1; dj <= 1; ++dj) {
            if (di == 0 && dj == 0) continue;
            int ni = i + di;
            int nj = j + dj;
            if (wrap) {
                // toroidal wrap
                if (ni < 0) ni += r;
                if (ni >= r) ni -= r;
                if (nj < 0) nj += c;
                if (nj >= c) nj -= c;
                cnt += g[ni][nj];
            }
            else {
                // finite: out-of-bounds treated as dead
                if (ni >= 0 && ni < r && nj >= 0 && nj < c) cnt += g[ni][nj];
            }
        }
    }
    return cnt;
}

// Compute next generation
Grid next_generation(const Grid& curr, bool wrap) {
    int r = (int)curr.size();
    int c = (int)curr[0].size();
    Grid next(r, vector<uint8_t>(c, 0));
    for (int i = 0; i < r; ++i) {
        for (int j = 0; j < c; ++j) {
            int n = count_neighbors(curr, i, j, wrap);
            if (curr[i][j]) {
                // alive
                next[i][j] = (n == 2 || n == 3) ? 1 : 0;
            }
            else {
                // dead
                next[i][j] = (n == 3) ? 1 : 0;
            }
        }
    }
    return next;
}

// Compare grids
bool same_grid(const Grid& a, const Grid& b) {
    return a == b;
}

// Random seed
Grid random_grid(int rows, int cols, double prob, std::mt19937& rng) {
    Grid g(rows, vector<uint8_t>(cols, 0));
    uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            g[i][j] = (dist(rng) < prob) ? 1 : 0;
    return g;
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int rows = 40;
    int cols = 80;
    int generations = 100;
    int delay_ms = 100;
    string mode = "wrap";
    double seed_prob = 0.25;

    if (argc >= 2) {
        if (string(argv[1]) == "-h" || string(argv[1]) == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }
    if (argc >= 3) rows = stoi(argv[1]), cols = stoi(argv[2]);
    if (argc >= 4) generations = stoi(argv[3]);
    if (argc >= 5) delay_ms = stoi(argv[4]);
    if (argc >= 6) mode = argv[5];
    if (argc >= 7) seed_prob = stod(argv[6]);

    if (rows <= 0 || cols <= 0) {
        cerr << "rows and cols must be positive\n";
        return 1;
    }
    bool wrap = (mode == "wrap" || mode == "toroid" || mode == "toroidal");

    // RNG
    random_device rd;
    mt19937 rng(rd());

    // Initialize
    Grid grid = random_grid(rows, cols, seed_prob, rng);

    // Optionally let user paste a pattern from stdin before starting:
    // If there's extra input data on stdin, try to parse ASCII pattern where 'O' is alive.
    // To keep things simple: user can redirect a small text file to stdin; if present, use it.
    if (!cin.rdbuf()->in_avail()) {
        // nothing waiting in stdin
    }
    else {
        // Read lines up to rows
        vector<string> lines;
        string line;
        // consume remaining newline maybe
        while (getline(cin, line)) {
            if (!line.empty() || !lines.empty()) // skip leading empty lines
                lines.push_back(line);
            if ((int)lines.size() >= rows) break;
        }
        if (!lines.empty()) {
            // Replace grid top-left with pattern
            for (size_t i = 0; i < lines.size() && i < (size_t)rows; ++i) {
                for (size_t j = 0; j < lines[i].size() && j < (size_t)cols; ++j) {
                    grid[i][j] = (lines[i][j] == 'O' || lines[i][j] == '1' || lines[i][j] == 'X') ? 1 : 0;
                }
            }
        }
    }

    Grid prev;
    int gen = 0;
    bool infinite = (generations == 0);

    while (infinite || gen < generations) {
        clear_screen();
        cout << "Conway's Game of Life — Generation: " << gen << "  (" << rows << "x" << cols << ")  Mode: " << (wrap ? "wrap" : "finite") << "\n";
        print_grid(grid);

        Grid next = next_generation(grid, wrap);

        if (same_grid(next, grid)) {
            cout << "\nStable (no changes) — stopping at generation " << gen << ".\n";
            break;
        }
        if (!prev.empty() && same_grid(next, prev)) {
            cout << "\nEntered 2-step oscillator (period 2) — stopping at generation " << gen << ".\n";
            break;
        }

        prev = grid;
        grid = move(next);

        ++gen;
        if (!infinite && gen >= generations) break;

        this_thread::sleep_for(chrono::milliseconds(delay_ms));
    }

    cout << "Finished at generation " << gen << ".\n";
    return 0;
}