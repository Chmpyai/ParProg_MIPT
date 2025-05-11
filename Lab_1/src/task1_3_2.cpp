#include <mpi.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <cmath>
#ifdef _WIN32
#include <windows.h>
#endif

// Параметры задачи
constexpr double a = 1.0;  // скорость переноса
constexpr double h = 0.01; // шаг по пространству
constexpr double tau = 0.005; // шаг по времени
constexpr double T = 1.0;  // конечное время
constexpr double L = 1.0;  // длина области

// Начальное условие
double u0(double x) {
    return std::exp(-100 * (x - 0.5) * (x - 0.5));
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Новые параметры сетки
    double h_ = h;
    double tau_ = tau;
    if (argc > 2) {
        h_ = L / (std::stoi(argv[1]) - 1);
        tau_ = T / (std::stoi(argv[2]) - 1);
    }

    // Расчет параметров сетки
    const int N = static_cast<int>(L / h_) + 1;  // количество точек по пространству
    const int M = static_cast<int>(T / tau_) + 1; // количество точек по времени

    // Распределение точек по процессам
    const int local_N = N / size + (rank == size - 1 ? N % size : 0);
    const int start_i = rank * (N / size);

    // Выделение памяти для решения
    std::vector<std::vector<double>> u(M, std::vector<double>(local_N + 2));  // +2 для граничных точек

    // Заполнение начального условия
    for (int i = 0; i < local_N; ++i) {
        u[0][i + 1] = u0((start_i + i) * h_);
    }

    // Начало измерения времени
    auto start = std::chrono::high_resolution_clock::now();

    // Решение уравнения переноса
    for (int n = 0; n < M - 1; ++n) {
        // Обмен граничными точками
        if (rank > 0) {
            MPI_Sendrecv(&u[n][1], 1, MPI_DOUBLE, rank - 1, 0,
                        &u[n][0], 1, MPI_DOUBLE, rank - 1, 0,
                        MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        if (rank < size - 1) {
            MPI_Sendrecv(&u[n][local_N], 1, MPI_DOUBLE, rank + 1, 0,
                        &u[n][local_N + 1], 1, MPI_DOUBLE, rank + 1, 0,
                        MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Граничное условие на левом конце
        if (rank == 0) {
            u[n + 1][0] = 0.0;
        }

        // Внутренние точки
        for (int i = 1; i <= local_N; ++i) {
            u[n + 1][i] = u[n][i] - a * tau_ / h_ * (u[n][i] - u[n][i - 1]);
        }

        // Граничное условие на правом конце
        if (rank == size - 1) {
            u[n + 1][local_N + 1] = u[n + 1][local_N];
        }
    }

    // Конец измерения времени
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Сбор результатов на нулевом процессе
    if (rank == 0) {
        std::vector<double> global_solution(N);
        
        // Копируем локальное решение
        for (int i = 0; i < local_N; ++i) {
            global_solution[i] = u[M - 1][i + 1];
        }

        // Получаем решения от других процессов
        for (int p = 1; p < size; ++p) {
            int p_local_N = N / size + (p == size - 1 ? N % size : 0);
            MPI_Recv(&global_solution[p * (N / size)], p_local_N, MPI_DOUBLE, p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Вывод результатов
        std::cout << "Сетка: " << N << "x" << M << std::endl;
        std::cout << "Процессы: " << size << std::endl;
        std::cout << "Время: " << duration.count() / 1000.0 << " с" << std::endl;

        // Сохранение результатов
        std::ofstream out("parallel_results.txt");
        out << "x,u\n";
        for (int i = 0; i < N; ++i) {
            out << i * h_ << "," << global_solution[i] << "\n";
        }
    } else {
        // Отправляем локальное решение на нулевой процесс
        MPI_Send(&u[M - 1][1], local_N, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
} 