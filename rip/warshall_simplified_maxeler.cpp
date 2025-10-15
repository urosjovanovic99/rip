//#include <stdio.h>
//#include <stdlib.h>
//#include <stdint.h>
//
//#define N 7
//
//void warshall_dfe(uint8_t closure[N][N]) {
//    int total_size = N * N;
//
//    /* Allocate buffers */
//    uint8_t* closure_in = (uint8_t*)malloc(total_size * sizeof(uint8_t));
//    uint8_t* row_elems = (uint8_t*)malloc(total_size * sizeof(uint8_t));
//    uint8_t* col_elems = (uint8_t*)malloc(total_size * sizeof(uint8_t));
//    uint8_t* closure_out = (uint8_t*)malloc(total_size * sizeof(uint8_t));
//
//    max_file_t* maxfile = Warshall_init();
//    max_engine_t* engine = max_load(maxfile, "*");
//
//    for (int k = 0; k < N; k++) {
//        for (int i = 0; i < N; i++) {
//            for (int j = 0; j < N; j++) {
//                int idx = i * N + j;
//                closure_in[idx] = closure[i][j];
//                row_elems[idx] = closure[i][k];
//                col_elems[idx] = closure[k][j];
//            }
//        }
//
//        Warshall_run(maxeler, row_elems, col_elems);
//
//        for (int i = 0; i < N; i++) {
//            for (int j = 0; j < N; j++) {
//                int idx = i * N + j;
//                closure[i][j] = closure_out[idx];
//            }
//        }
//    }
//
//    /* Cleanup */
//    free(closure_in);
//    free(row_elems);
//    free(col_elems);
//    free(closure_out);
//}
//int main() {
//    
//    uint8_t adjacency[N][N] = {
//        {1, 1, 0, 0, 0, 0, 0},
//        {0, 1, 1, 0, 0, 0, 0},
//        {0, 0, 1, 1, 0, 0, 0},
//        {0, 0, 0, 1, 1, 0, 0},
//        {0, 0, 0, 0, 1, 1, 0},
//        {0, 0, 0, 0, 0, 1, 1},
//        {0, 0, 0, 0, 0, 0, 1}
//    };
//
//    /* Copy for DFE computation */
//    uint8_t closure_dfe[N][N];
//    for (int i = 0; i < N; i++) {
//        for (int j = 0; j < N; j++) {
//            closure_dfe[i][j] = adjacency[i][j];
//        }
//    }
//    warshall_dfe(closure_dfe);
//
//    return 0;
//}
