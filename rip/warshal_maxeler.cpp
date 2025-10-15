//#include <stdio.h>
//#include <stdlib.h>
//#include <stdint.h>
//#include <string.h>
//
//#define N 7  /* Matrix size */
//
///* Warshall algorithm on Maxeler DFE - Pointer Optimized */
//void warshall_dfe(uint8_t closure[N][N]) {
//    int total_size = N * N;
//
//    /* Allocate buffers */
//    uint8_t* closure_flat = (uint8_t*)malloc(total_size * sizeof(uint8_t));
//    uint8_t* row_stream = (uint8_t*)malloc(total_size * sizeof(uint8_t));
//    uint8_t* col_stream = (uint8_t*)malloc(total_size * sizeof(uint8_t));
//    uint8_t* closure_out = (uint8_t*)malloc(total_size * sizeof(uint8_t));
//
//    for (int i = 0; i < N; i++) {
//        memcpy(&closure_flat[i * N], closure[i], N * sizeof(uint8_t));
//    }
//
//    for (int k = 0; k < N; k++) {
//        uint8_t* row_k = &closure_flat[k * N];
//
//        uint8_t col_k[N];
//        for (int i = 0; i < N; i++) {
//            col_k[i] = closure_flat[i * N + k];
//        }
//        for (int i = 0; i < N; i++) {
//            memset(&row_stream[i * N], col_k[i], N);
//            memcpy(&col_stream[i * N], row_k, N);
//        }
//
//        Warshall_run(engine, total_size, closure_flat, row_stream, col_stream, closure_out);
//
//        memcpy(closure_flat, closure_out, total_size * sizeof(uint8_t));
//    }
//
//    for (int i = 0; i < N; i++) {
//        memcpy(closure[i], &closure_flat[i * N], N * sizeof(uint8_t));
//    }
//
//    free(closure_flat);
//    free(row_stream);
//    free(col_stream);
//    free(closure_out);
//}
//
//int main() {
//    uint8_t closure[N][N] = {
//        {1, 1, 0, 0, 0, 0, 0},
//        {0, 1, 1, 0, 0, 0, 0},
//        {0, 0, 1, 1, 0, 0, 0},
//        {0, 0, 0, 1, 1, 0, 0},
//        {0, 0, 0, 0, 1, 1, 0},
//        {0, 0, 0, 0, 0, 1, 1},
//        {0, 0, 0, 0, 0, 0, 1}
//    };
//
//    warshall_dfe(closure);
//
//    return 0;
//}