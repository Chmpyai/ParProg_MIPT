#include <mpi.h>
#include <iostream>
#include <vector>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        if (rank == 0) {
            std::cerr << "Требуется ровно 2 процесса" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    // Размеры сообщений для тестирования
    const std::vector<int> sizes = {1, 10, 100, 1000, 10000, 100000, 1000000};
    const int iterations = 1000;

    if (rank == 0) {
        std::cout << "Размер (байт),Время (мкс)\n";
    }

    for (int size : sizes) {
        std::vector<char> buffer(size);
        double total_time = 0.0;

        for (int i = 0; i < iterations; ++i) {
            MPI_Barrier(MPI_COMM_WORLD);
            auto start = std::chrono::high_resolution_clock::now();

            if (rank == 0) {
                MPI_Send(buffer.data(), size, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
                MPI_Recv(buffer.data(), size, MPI_BYTE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                MPI_Recv(buffer.data(), size, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(buffer.data(), size, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
            }

            total_time += std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - start).count();
        }

        if (rank == 0) {
            std::cout << size << "," << total_time / (2 * iterations) << std::endl;
        }
    }

    MPI_Finalize();
    return 0;
} 