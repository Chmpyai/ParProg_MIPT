#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <cmath>
#ifdef _WIN32
#include <windows.h>
#endif


// Решение одномерного линейного уравнения переноса u_t + a * u_x = 0
//       последовательным методом.
// Область: Пространство x от 0 до L, время t от 0 до T.
// Граничные условия (ГУ):
//   - На левой границе (x=0): u(0, t) = 0 .
//   - На правой границе (x=L): 
//     (u(L, t) = u(L-h, t)), что примерно соответствует нулевой производной.
// Начальное условие (НУ):
//   - В момент времени t=0: u(x, 0) задается функцией Гауссова импульса.

// Параметры задачи
constexpr double a = 1.0;  // скорость переноса
constexpr double h = 0.001; // шаг по пространству
constexpr double tau = 0.0005; // шаг по времени
constexpr double T = 1.0;  // конечное время
constexpr double L = 1.0;  // длина области


// Необходимо выполнение условия:
// |a| * tau / h <= 1
// В нашем случае: 1.0 * 0.0005 / 0.001 = 0.5. Условие выполняется (0.5 <= 1), схема устойчива.

// Начальное условие
double u0(double x) {
    return std::exp(-100 * (x - 0.5) * (x - 0.5));
}

// MAIN
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // Параметры сетки сетки
    const int N = static_cast<int>(L / h) + 1;  // точки по пространству
    const int M = static_cast<int>(T / tau) + 1; // точки по времени

    // Выделение памяти для решения
    std::vector<std::vector<double>> u(M, std::vector<double>(N));

    // Начальное условие
    for (int i = 0; i < N; ++i) {
        u[0][i] = u0(i * h);
    }
    // Start time
    auto start = std::chrono::high_resolution_clock::now();

    // Решение уравнения переноса
    for (int n = 0; n < M - 1; ++n) {
        u[n + 1][0] = 0.0;  // ГУ

        for (int i = 1; i < N - 1; ++i) {
            u[n + 1][i] = u[n][i] - a * tau / h * (u[n][i] - u[n][i - 1]);
        }

        u[n + 1][N - 1] = u[n + 1][N - 2];  // ГУ
    }

    // Finish time
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