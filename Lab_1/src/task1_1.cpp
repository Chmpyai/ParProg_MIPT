// task1_1.cpp
#include <mpi.h>       // MPI
#include <iostream>    // std::cout, std::cerr
#include <random>      // генерация случайных чисел
#include <chrono>      // измерение времени
#include <fstream>     // write res to file
#include <string>      // Для std::string, std::stoll

#ifdef _WIN32
#include <windows.h>   // Для SetConsoleOutputCP на Windows
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // Корректное отображение UTF-8 в Win
#endif
    //  MPI
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        std::cerr << "Ошибка инициализации MPI." << std::endl;
        return 1;
    }

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // ID текущего процесса
    MPI_Comm_size(MPI_COMM_WORLD, &size); // кол-во процессов

    // Общее кол-во точек для алгоритма
    const long long total_points = (argc > 1) ? std::stoll(argv[1]) : 1000000;

    // Распределение точек по процессам
    long long points_per_process = total_points / size;
    long long remainder_points = total_points % size;
    long long local_num_points = points_per_process + (rank < remainder_points ? 1 : 0);

    // Генерация случайных точек(Вихрь Мерсенна)
    std::mt19937_64 rng(static_cast<unsigned long long>(rank) + 12345ULL + static_cast<unsigned long long>(std::chrono::system_clock::now().time_since_epoch().count()));
    std::uniform_real_distribution<double> dist(0.0, 1.0); // Равномерное распределение [0.0, 1.0)

    // Замер времени
    auto start_time = std::chrono::high_resolution_clock::now();

    // Подсчет точек
    long long local_inside_circle_count = 0;
    for (long long i = 0; i < local_num_points; ++i) {
        double x = dist(rng);
        double y = dist(rng);
        if (x * x + y * y <= 1.0) {
            ++local_inside_circle_count;
        }
    }

    // Сбор резов с рангом 0
    long long global_inside_circle_count = 0;
    MPI_Reduce(&local_inside_circle_count, &global_inside_circle_count, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    // вывод
    if (rank == 0) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        double duration_s = duration_ms.count() / 1000.0;

        // Пи: 4 * (точки в круге / всего точек)
        double pi_estimate = (total_points > 0) ? (4.0 * global_inside_circle_count / total_points) : 0.0;

        std::cout << "π ≈ " << pi_estimate << std::endl;
        std::cout << "Точки: " << total_points << std::endl;
        std::cout << "Процессы: " << size << std::endl;
        std::cout << "Время: " << duration_s << " с" << std::endl;

        //  + write to file pi_result.csv
        std::ofstream fout("pi_result.csv");
        if (fout.is_open()) {
            fout << "Процессы,Точки,π,Время(с)\n";
            fout << size << "," << total_points << "," << pi_estimate << "," << duration_s << std::endl;
            fout.close();
        } else {
            std::cerr << "Ошибка: не удалось открыть файл pi_result.csv для записи." << std::endl;
        }
    }

    MPI_Finalize();
    return 0;
}