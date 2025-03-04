#include <iostream>
#include <cuda_runtime.h>

// 核函数：在 GPU 上运行，执行向量加法
__global__ void vectorAdd(float *A, float *B, float *C, int N) {
    // 计算全局线程 ID
    int idx = threadIdx.x + blockIdx.x * blockDim.x;

    // 每个线程计算一个元素的加法
    if (idx < N) {
        C[idx] = A[idx] + B[idx];
    }
}

int main() {
    // 定义向量大小
    int N = 1 << 20; // 1M 元素
    size_t size = N * sizeof(float);

    // 在主机上分配内存
    float *h_A = (float *)malloc(size);
    float *h_B = (float *)malloc(size);
    float *h_C = (float *)malloc(size);

    // 初始化向量 A 和 B
    for (int i = 0; i < N; i++) {
        h_A[i] = static_cast<float>(i);
        h_B[i] = static_cast<float>(i * 2);
    }

    // 在设备上分配内存
    float *d_A, *d_B, *d_C;
    cudaMalloc((void **)&d_A, size);
    cudaMalloc((void **)&d_B, size);
    cudaMalloc((void **)&d_C, size);

    // 将数据从主机传输到设备
    cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, size, cudaMemcpyHostToDevice);

    // 定义线程块大小和线程块数量
    int threadsPerBlock = 256;
    int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;

    // 启动核函数，在 GPU 上执行向量加法
    vectorAdd<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, d_C, N);

    // 将结果从设备传输回主机
    cudaMemcpy(h_C, d_C, size, cudaMemcpyDeviceToHost);

    // 验证计算结果
    bool success = true;
    for (int i = 0; i < N; i++) {
        if (h_C[i] != h_A[i] + h_B[i]) {
            success = false;
            break;
        }
    }

    if (success) {
        std::cout << "Vector addition completed successfully!" << std::endl;
    } else {
        std::cout << "Vector addition failed!" << std::endl;
    }

    // 释放主机和设备内存
    free(h_A);
    free(h_B);
    free(h_C);
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    return 0;
}