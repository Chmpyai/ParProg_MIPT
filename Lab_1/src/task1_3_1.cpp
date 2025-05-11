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
constexpr double h = 0.001; // шаг по пространству
constexpr double tau = 0.0005; // шаг по времени
constexpr double T = 1.0;  // конечное время
constexpr double L = 1.0;  // длина области

// Начальное условие
double u0(double x) {
    return std::exp(-100 * (x - 0.5) * (x - 0.5));
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // Расчет параметров сетки
    const int N = static_cast<int>(L / h) + 1;  // количество точек по пространству
    const int M = static_cast<int>(T / tau) + 1; // количество точек по времени

    // Выделение памяти для решения
    std::vector<std::vector<double>> u(M, std::vector<double>(N));

    // Заполнение начального условия
    for (int i = 0; i < N; ++i) {
        u[0][i] = u0(i * h);
    }

    // Начало измерения времени
    auto start = std::chrono::high_resolution_clock::now();

    // Решение уравнения переноса
    for (int n = 0; n < M - 1; ++n) {
        u[n + 1][0] = 0.0;  // граничное условие

        for (int i = 1; i < N - 1; ++i) {
            u[n + 1][i] = u[n][i] - a * tau / h * (u[n][i] - u[n][i - 1]);
        }

        u[n + 1][N - 1] = u[n + 1][N - 2];  // граничное условие
    }

    // Конец измерения времени
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Вывод результатов
    std::cout << "Сетка: " << N << "x" << M << std::endl;
    std::cout << "Время: " << duration.count() / 1000.0 << " с" << std::endl;

    // Сохранение результатов
    std::ofstream out("sequential_results.txt");
    out << "x,t,u\n";
    for (int n = 0; n < M; n += 10) {
        for (int i = 0; i < N; ++i) {
            out << i * h << "," << n * tau << "," << u[n][i] << "\n";
        }
    }

    return 0;
} 