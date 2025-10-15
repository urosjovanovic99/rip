//#include <stdio.h>
//#include <stdlib.h>
//#include <stdint.h>
//#include <MaxSLiCInterface.h>
//#include "Maxfiles.h"
//
//#define N 7
//
//int main(void) {
//
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
//    uint8_t* closure_in = (uint8_t*)malloc(N * N * sizeof(uint8_t));
//    uint8_t* closure_out = (uint8_t*)malloc(N * N * sizeof(uint8_t));
//
//    for (int i = 0; i < N; i++) {
//        for (int j = 0; j < N; j++) {
//            closure_in[i * N + j] = closure[i][j];
//        }
//    }
//
//    max_run(engine, N, closure_out);
//
//    for (int i = 0; i < N; i++) {
//        for (int j = 0; j < N; j++) {
//            closure[i][j] = closure_out[i * N + j];
//        }
//    }
//
//    free(closure_in);
//    free(closure_out);
//
//    return 0;
//}