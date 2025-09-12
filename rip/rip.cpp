// rip.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <omp.h>
#include <vector>
#include <chrono>
#include <thread>
#include <conio.h>

const int M = 50;
const int N = 50;
const int NUMBER_OF_ITERATIONS = 100;
const bool ENABLE_VISUAL = true;

std::vector<std::vector<int>> generate_start_values() {
    std::vector<std::vector<int>> matrix(M, std::vector<int>(N));
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            matrix[i][j] = rand() % 2 == 1 ? 0 : 1;
        }
    }

    return matrix;
}

int calculate_live_neighbours(std::vector<std::vector<int>> matrix, int i, int j) {
    int live_neighbours = 0;
    for (int x = i - 1; x < i + 2; x++) {
        for (int y = j - 1; y < j + 2; y++) {
            if (x < 0 || x >= M || y < 0 || y >= N || (x == i && y == j)) {
                live_neighbours += 0;
            }
            else {
                live_neighbours += matrix[x][y];
            }
        }
    }
    return live_neighbours;
}

int calculate_next_generation_for_single_cell(std::vector<std::vector<int>> matrix, int i, int j) {
    int cell = matrix[i][j];
    int live_neighbours = calculate_live_neighbours(matrix, i, j);
    if (live_neighbours > 8)
        std::cout << "";
    if (cell == 1 && live_neighbours < 2) {
        return 0; // underpopulation
    }
    else if (cell == 1 && live_neighbours > 3) {
        return 0; // overpopulation
    }
    else if (cell == 1 && (live_neighbours == 2 || live_neighbours == 3)) {
        return 1; // stay alive
    }
    else if (cell == 0 && live_neighbours == 3) {
        return 1; // reproduction
    }
    return cell;
}

std::vector<std::vector<int>> calculate_next_generation(std::vector<std::vector<int>> matrix) {
    std::vector<std::vector<int>> next_gen_matrix(M, std::vector<int>(N));
#pragma omp parallel for schedule(static)
    for (int x = 0; x < M * N; x++) {
        int i = x / N;
        int j = x % N;
        next_gen_matrix[i][j] = calculate_next_generation_for_single_cell(matrix, i, j);
    }
    //for (int i = 0; i < M; i++) {
    //    for (int j = 0; j < N; j++) {
    //        next_gen_matrix[i][j] = calculate_next_generation_for_single_cell(matrix, i, j);
    //    }
    //}
    return next_gen_matrix;
}

void show(std::vector<std::vector<int>> matrix) {
    std::cout << "\x1b[H\x1b[2J";
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            std::cout << (matrix[i][j] ? "\033[07m  \033[m" : "  ");
        }
        std::cout << std::endl;
    }
}

int main()
{
    std::vector<std::vector<int>> matrix = generate_start_values();
    int iteration = 0;
    auto start = std::chrono::high_resolution_clock::now();
    while (iteration++ < NUMBER_OF_ITERATIONS) {
        if(ENABLE_VISUAL)
            show(matrix);
        matrix = calculate_next_generation(matrix);
        if(ENABLE_VISUAL)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "Time elapsed: " << duration.count() << std::endl;
    return 0;
}