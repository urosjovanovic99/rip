#include <CL/cl.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>


namespace many_core {
    const char* kernelSource = R"CLC(
__kernel void hello(__global char* out) {
    const char msg[] = "Hello, World from OpenCL!";
    int id = get_global_id(0);
    if (id < sizeof(msg)) {
        out[id] = msg[id];
    }
}
)CLC";

    const int M = 90;
    const int N = 90;
    const int NUMBER_OF_ITERATIONS = 100;
    const bool ENABLE_VISUAL = true;

    void show(std::vector<std::vector<int>> matrix) {
        std::cout << "\x1b[H\x1b[2J";
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                if(matrix[i].size() > 0)
                    std::cout << (matrix[i][j] ? "\033[07m  \033[m" : "  ");
            }
            std::cout << std::endl;
        }
    }

    std::vector<std::vector<int>> generate_start_values() {
        std::vector<std::vector<int>> matrix(M, std::vector<int>(N));
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                matrix[i][j] = rand() % 2 == 1 ? 0 : 1;
            }
        }

        return matrix;
    }

    // Load kernel from file
    std::string loadKernel(const char* filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open kernel file");
        }
        std::ostringstream oss;
        oss << file.rdbuf();
        return oss.str();
    }
}
using namespace many_core;

//int main() {
//    std::vector<std::vector<int>> matrix = generate_start_values();
//
//    cl_int err;
//
//    // 1. Get platform & device
//    cl_platform_id platform;
//    cl_device_id device;
//    err = clGetPlatformIDs(1, &platform, nullptr);
//    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
//
//    // 2. Create context & queue
//    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
//    cl_queue_properties props[] = { CL_QUEUE_PROPERTIES, 0, 0 };
//    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, props, &err);
//
//    // 3. Load & build program
//    std::string kernelSource = loadKernel("game_of_life.cl");
//    const char* src = kernelSource.c_str();
//    size_t length = kernelSource.size();
//    cl_program program = clCreateProgramWithSource(context, 1, &src, &length, &err);
//    err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
//
//    if (err != CL_SUCCESS) {
//        // Print build log if compilation fails
//        size_t log_size;
//        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
//        std::vector<char> log(log_size);
//        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
//        std::cerr << "Build log:\n" << log.data() << "\n";
//        return 1;
//    }
//
//    cl_kernel kernel = clCreateKernel(program, "game_of_life", &err);
//
//    // 4. Create buffers
//    cl_mem buf_in = clCreateBuffer(context, CL_MEM_READ_WRITE,
//        sizeof(int) * M * N, nullptr, &err);
//    cl_mem buf_out = clCreateBuffer(context, CL_MEM_READ_WRITE,
//        sizeof(int) * M * N, nullptr, &err);
//
//    // 5. Run multiple steps
//    for (int step = 0; step < NUMBER_OF_ITERATIONS; step++) {
//        // Copy grid to device
//        err = clEnqueueWriteBuffer(queue, buf_in, CL_TRUE, 0,
//            sizeof(int) * matrix.size(), matrix.data(), 0, nullptr, nullptr);
//
//        // Set arguments
//        err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in);
//        err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_out);
//        err = clSetKernelArg(kernel, 2, sizeof(int), &M);
//        err = clSetKernelArg(kernel, 3, sizeof(int), &N);
//
//        // Run kernel
//        size_t globalSize[2] = { M, N };
//        err = clEnqueueNDRangeKernel(queue, kernel, 2, nullptr, globalSize, nullptr, 0, nullptr, nullptr);
//
//        // Read back
//        err = clEnqueueReadBuffer(queue, buf_out, CL_TRUE, 0,
//            sizeof(int) * matrix.size(), matrix.data(), 0, nullptr, nullptr);
//
//        //std::cout << "Step " << step + 1 << ":\n";
//        show(matrix);
//        std::this_thread::sleep_for(std::chrono::milliseconds(200));
//    }
//
//    // 6. Cleanup
//    clReleaseMemObject(buf_in);
//    clReleaseMemObject(buf_out);
//    clReleaseKernel(kernel);
//    clReleaseProgram(program);
//    clReleaseCommandQueue(queue);
//    clReleaseContext(context);
//
//    return 0;
//}