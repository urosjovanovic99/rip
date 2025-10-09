#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <CL/cl.h>

#define NUM_GENERALS 64
#define MAX_ROUNDS 3

// OpenCL kernel code as string
const char* kernel_source = R"(
typedef enum { ATTACK = 0, RETREAT = 1, UNKNOWN = 2 } Action;

typedef struct {
    int id;
    int is_loyal;
    int proposed_action;
} General;

typedef struct {
    int sender_id;
    int order;
    int valid;
} Message;

// Kernel 1: Commander broadcasts to all lieutenants
__kernel void broadcast_phase(
    __global General* commander,
    __global General* lieutenants,
    __global Message* messages,
    __global uint* rng_state,
    int num_lieutenants)
{
    int gid = get_global_id(0);
    if (gid >= num_lieutenants) return;
    
    // Simple LCG random number generator
    uint seed = rng_state[gid];
    seed = seed * 1103515245 + 12345;
    rng_state[gid] = seed;
    
    int order;
    if (commander->is_loyal) {
        order = commander->proposed_action;
    } else {
        // Traitor sends random orders
        order = (seed % 2 == 0) ? ATTACK : RETREAT;
    }
    
    // Store message
    messages[gid].sender_id = commander->id;
    messages[gid].order = order;
    messages[gid].valid = 1;
    
    // Update lieutenant's received action
    lieutenants[gid].proposed_action = order;
}

// Kernel 2: Lieutenants relay messages to each other
__kernel void relay_phase(
    __global General* lieutenants,
    __global Message* input_messages,
    __global Message* output_messages,
    __global uint* rng_state,
    int num_lieutenants,
    int round)
{
    int sender_idx = get_global_id(0);
    int receiver_idx = get_global_id(1);
    
    if (sender_idx >= num_lieutenants || receiver_idx >= num_lieutenants) return;
    if (sender_idx == receiver_idx) return; // Don't send to self
    
    int output_idx = sender_idx * num_lieutenants + receiver_idx;
    
    // Get the message this lieutenant received
    int input_idx = sender_idx;
    if (input_messages[input_idx].valid == 0) {
        output_messages[output_idx].valid = 0;
        return;
    }
    
    // Generate random number for traitor behavior
    uint seed = rng_state[sender_idx * num_lieutenants + receiver_idx];
    seed = seed * 1103515245 + 12345;
    rng_state[sender_idx * num_lieutenants + receiver_idx] = seed;
    
    int relayed_order;
    if (lieutenants[sender_idx].is_loyal) {
        relayed_order = input_messages[input_idx].order;
    } else {
        // Traitor sends conflicting messages
        relayed_order = (seed % 2 == 0) ? ATTACK : RETREAT;
    }
    
    output_messages[output_idx].sender_id = sender_idx;
    output_messages[output_idx].order = relayed_order;
    output_messages[output_idx].valid = 1;
}

// Kernel 3: Each lieutenant computes consensus
__kernel void consensus_phase(
    __global General* lieutenants,
    __global Message* all_messages,
    __global int* decisions,
    int num_lieutenants,
    int total_messages)
{
    int gid = get_global_id(0);
    if (gid >= num_lieutenants) return;
    
    int attack_votes = 0;
    int retreat_votes = 0;
    
    // Count votes from all messages received
    for (int i = 0; i < total_messages; i++) {
        // Check if this message is for this lieutenant
        int receiver_idx = i % num_lieutenants;
        if (receiver_idx == gid && all_messages[i].valid) {
            if (all_messages[i].order == ATTACK) {
                attack_votes++;
            } else if (all_messages[i].order == RETREAT) {
                retreat_votes++;
            }
        }
    }
    
    // Make decision based on majority
    decisions[gid] = (attack_votes > retreat_votes) ? ATTACK : RETREAT;
}

// Kernel 4: Final voting among loyal lieutenants (reduction)
__kernel void final_vote(
    __global General* lieutenants,
    __global int* decisions,
    __global int* vote_counts,
    int num_lieutenants)
{
    int gid = get_global_id(0);
    if (gid >= num_lieutenants) return;
    
    // Only loyal lieutenants vote
    if (lieutenants[gid].is_loyal) {
        if (decisions[gid] == ATTACK) {
            atomic_inc(&vote_counts[0]); // ATTACK counter
        } else {
            atomic_inc(&vote_counts[1]); // RETREAT counter
        }
    }
}
)";

class OpenCLByzantine {
private:
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;

    cl_kernel broadcast_kernel;
    cl_kernel relay_kernel;
    cl_kernel consensus_kernel;
    cl_kernel vote_kernel;

    int num_lieutenants;

public:
    OpenCLByzantine(int num_lts) : num_lieutenants(num_lts) {
        setup_opencl();
    }

    ~OpenCLByzantine() {
        cleanup();
    }

    void setup_opencl() {
        cl_int err;

        // Get Intel GPU platform
        err = clGetPlatformIDs(1, &platform, nullptr);
        if (err != CL_SUCCESS) {
            std::cerr << "Failed to get platform" << std::endl;
            exit(1);
        }

        // Get GPU device
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
        if (err != CL_SUCCESS) {
            std::cerr << "Failed to get GPU device, trying CPU" << std::endl;
            err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, nullptr);
        }

        // Print device info
        char device_name[128];
        clGetDeviceInfo(device, CL_DEVICE_NAME, 128, device_name, nullptr);
        std::cout << "Using device: " << device_name << std::endl;

        // Create context
        context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);

        // Create command queue (OpenCL 2.0+)
#ifdef CL_VERSION_2_0
        queue = clCreateCommandQueueWithProperties(context, device, nullptr, &err);
#else
        queue = clCreateCommandQueue(context, device, 0, &err);
#endif

        // Create program
        program = clCreateProgramWithSource(context, 1, &kernel_source, nullptr, &err);

        // Build program
        err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
        if (err != CL_SUCCESS) {
            size_t log_size;
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
            std::vector<char> log(log_size);
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
            std::cerr << "Build error:\n" << log.data() << std::endl;
            exit(1);
        }

        // Create kernels
        broadcast_kernel = clCreateKernel(program, "broadcast_phase", &err);
        relay_kernel = clCreateKernel(program, "relay_phase", &err);
        consensus_kernel = clCreateKernel(program, "consensus_phase", &err);
        vote_kernel = clCreateKernel(program, "final_vote", &err);
    }

    int run_protocol(std::vector<int>& general_ids, std::vector<int>& is_loyal,
        std::vector<int>& proposed_actions) {
        cl_int err;

        // Prepare host data
        struct General {
            int id;
            int is_loyal;
            int proposed_action;
        };

        General commander;
        commander.id = 0;
        commander.is_loyal = is_loyal[0];
        commander.proposed_action = proposed_actions[0];

        std::vector<General> lieutenants(num_lieutenants);
        for (int i = 0; i < num_lieutenants; i++) {
            lieutenants[i].id = general_ids[i + 1];
            lieutenants[i].is_loyal = is_loyal[i + 1];
            lieutenants[i].proposed_action = 2; // UNKNOWN
        }

        // Create buffers
        cl_mem commander_buf = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            sizeof(General), &commander, &err);

        cl_mem lieutenants_buf = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
            num_lieutenants * sizeof(General), lieutenants.data(), &err);

        // Message buffers
        struct Message {
            int sender_id;
            int order;
            int valid;
        };

        std::vector<Message> messages(num_lieutenants * num_lieutenants);
        cl_mem messages_buf = clCreateBuffer(context, CL_MEM_READ_WRITE,
            num_lieutenants * num_lieutenants * sizeof(Message), nullptr, &err);

        // RNG state buffer
        std::vector<unsigned int> rng_state(num_lieutenants * num_lieutenants);
        for (size_t i = 0; i < rng_state.size(); i++) {
            rng_state[i] = rand();
        }
        cl_mem rng_buf = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
            rng_state.size() * sizeof(unsigned int), rng_state.data(), &err);

        // Phase 1: Broadcast
        clSetKernelArg(broadcast_kernel, 0, sizeof(cl_mem), &commander_buf);
        clSetKernelArg(broadcast_kernel, 1, sizeof(cl_mem), &lieutenants_buf);
        clSetKernelArg(broadcast_kernel, 2, sizeof(cl_mem), &messages_buf);
        clSetKernelArg(broadcast_kernel, 3, sizeof(cl_mem), &rng_buf);
        clSetKernelArg(broadcast_kernel, 4, sizeof(int), &num_lieutenants);

        size_t global_size = num_lieutenants;
        clEnqueueNDRangeKernel(queue, broadcast_kernel, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);
        clFinish(queue);

        // Phase 2: Relay (simplified - one round)
        cl_mem messages_out_buf = clCreateBuffer(context, CL_MEM_READ_WRITE,
            num_lieutenants * num_lieutenants * sizeof(Message), nullptr, &err);

        clSetKernelArg(relay_kernel, 0, sizeof(cl_mem), &lieutenants_buf);
        clSetKernelArg(relay_kernel, 1, sizeof(cl_mem), &messages_buf);
        clSetKernelArg(relay_kernel, 2, sizeof(cl_mem), &messages_out_buf);
        clSetKernelArg(relay_kernel, 3, sizeof(cl_mem), &rng_buf);
        clSetKernelArg(relay_kernel, 4, sizeof(int), &num_lieutenants);
        int round = 1;
        clSetKernelArg(relay_kernel, 5, sizeof(int), &round);

        size_t global_sizes[2] = { (size_t)num_lieutenants, (size_t)num_lieutenants };
        clEnqueueNDRangeKernel(queue, relay_kernel, 2, nullptr, global_sizes, nullptr, 0, nullptr, nullptr);
        clFinish(queue);

        // Phase 3: Consensus
        std::vector<int> decisions(num_lieutenants);
        cl_mem decisions_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
            num_lieutenants * sizeof(int), nullptr, &err);

        int total_msgs = num_lieutenants * num_lieutenants;
        clSetKernelArg(consensus_kernel, 0, sizeof(cl_mem), &lieutenants_buf);
        clSetKernelArg(consensus_kernel, 1, sizeof(cl_mem), &messages_out_buf);
        clSetKernelArg(consensus_kernel, 2, sizeof(cl_mem), &decisions_buf);
        clSetKernelArg(consensus_kernel, 3, sizeof(int), &num_lieutenants);
        clSetKernelArg(consensus_kernel, 4, sizeof(int), &total_msgs);

        clEnqueueNDRangeKernel(queue, consensus_kernel, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);
        clFinish(queue);

        // Phase 4: Final vote
        std::vector<int> vote_counts(2, 0);
        cl_mem votes_buf = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
            2 * sizeof(int), vote_counts.data(), &err);

        clSetKernelArg(vote_kernel, 0, sizeof(cl_mem), &lieutenants_buf);
        clSetKernelArg(vote_kernel, 1, sizeof(cl_mem), &decisions_buf);
        clSetKernelArg(vote_kernel, 2, sizeof(cl_mem), &votes_buf);
        clSetKernelArg(vote_kernel, 3, sizeof(int), &num_lieutenants);

        clEnqueueNDRangeKernel(queue, vote_kernel, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);
        clFinish(queue);

        // Read results
        clEnqueueReadBuffer(queue, votes_buf, CL_TRUE, 0, 2 * sizeof(int), vote_counts.data(), 0, nullptr, nullptr);

        std::cout << "Attack votes: " << vote_counts[0] << ", Retreat votes: " << vote_counts[1] << std::endl;

        // Cleanup buffers
        clReleaseMemObject(commander_buf);
        clReleaseMemObject(lieutenants_buf);
        clReleaseMemObject(messages_buf);
        clReleaseMemObject(messages_out_buf);
        clReleaseMemObject(decisions_buf);
        clReleaseMemObject(votes_buf);
        clReleaseMemObject(rng_buf);

        return (vote_counts[0] > vote_counts[1]) ? 0 : 1; // 0=ATTACK, 1=RETREAT
    }

    void cleanup() {
        clReleaseKernel(broadcast_kernel);
        clReleaseKernel(relay_kernel);
        clReleaseKernel(consensus_kernel);
        clReleaseKernel(vote_kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
    }
};

int main() {
    srand(static_cast<unsigned int>(time(0)));

    int num_lieutenants = NUM_GENERALS - 1;

    std::vector<int> general_ids(NUM_GENERALS);
    std::vector<int> is_loyal(NUM_GENERALS);
    std::vector<int> proposed_actions(NUM_GENERALS);

    // Commander
    general_ids[0] = 0;
    is_loyal[0] = 1;
    proposed_actions[0] = 0; // ATTACK

    int traitor_count = 0;
    for (int i = 1; i < NUM_GENERALS; i++) {
        general_ids[i] = i;
        is_loyal[i] = (rand() % 10) > 2; // 70% loyal
        proposed_actions[i] = 2; // UNKNOWN
        if (!is_loyal[i]) traitor_count++;
    }

    std::cout << "=== Byzantine Generals Problem (OpenCL GPU) ===" << std::endl;
    std::cout << "Total generals: " << NUM_GENERALS << std::endl;
    std::cout << "Traitors: " << traitor_count << std::endl << std::endl;

    OpenCLByzantine byzantine(num_lieutenants);

    int result = byzantine.run_protocol(general_ids, is_loyal, proposed_actions);

    std::cout << "\nFinal Decision: " << (result == 0 ? "ATTACK" : "RETREAT") << std::endl;

    return 0;
}