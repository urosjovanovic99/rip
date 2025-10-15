//#include <iostream>
//#include <omp.h>
//using namespace std;
//
//int main() {
//    const int n = 7;
//    const int threads = 4;
//    omp_set_num_threads(threads);
//
//    int graph[n][n] = {
//        {0, 1, 0, 1, 0, 0, 0},
//        {0, 0, 1, 0, 0, 0, 0},
//        {0, 0, 0, 0, 0, 1, 0},
//        {0, 0, 0, 0, 1, 0, 0},
//        {0, 0, 0, 0, 0, 0, 1},
//        {0, 0, 0, 0, 0, 0, 0},
//        {0, 0, 0, 0, 0, 1, 0}
//    };
//
//    double start = omp_get_wtime();
//
//    for (int k = 0; k < n; ++k) {
//#pragma omp parallel for collapse(2) shared(graph, n, k)
//        for (int i = 0; i < n; ++i) {
//            for (int j = 0; j < n; ++j) {
//                int thread_id = omp_get_thread_num();
//                // optional: print which thread is working on (i,j)
//#pragma omp critical
//                std::cout << "Thread " << thread_id << " working on (" << i << "," << j << ")\n";
//
//                if (graph[i][k] && graph[k][j])
//                    graph[i][j] = 1;
//            }
//        }
//    }
//
//    double end = omp_get_wtime();
//
//    // Output final transitive closure matrix
//    cout << "Transitive closure matrix:\n";
//    for (int i = 0; i < n; ++i) {
//        for (int j = 0; j < n; ++j) {
//            cout << graph[i][j] << " ";
//        }
//        cout << endl;
//    }
//
//    cout << "\nExecution time: " << (end - start) * 1000 << " ms\n";
//
//    return 0;
//}