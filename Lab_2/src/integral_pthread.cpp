
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <pthread.h> // POSIX Threads
#include <chrono> // 
#include <atomic>
#include <algorithm> // std::max

// --- Глобальные переменные ---
double func_to_integrate(double x) { if (x == 0.0) return 0.0; return sin(1.0 / x); }
struct Task {               // под-интервал интегрирования
    double a;               // Начало интервала
    double b;               // Конец интервала
    double target_epsilon;  // Требуемая точность
};

// передача данных в рабочий поток.
struct ThreadData {
    std::vector<Task>* tasks_queue;
    std::atomic<size_t>* next_task_index;
    double* partial_sum;
    int* evaluations_count; // подсчет общего числа вызовов
    double (*func)(double);
    double* execution_time_ms; // Указатель для записи времени выполнения потока (в мс)
};

// ИНТЕГРИРОВАНИЕ
// взял из методички Якобовский КУльков
// Адаптивный метод трапеций 
// Вычисляем интеграл функции f на интервале [a, b] с желаемой точностью.
double adaptive_trapezoidal(double (*f)(double), double a, double b, double S_prev, double tol, int& eval_count, int depth = 0, int max_depth = 20) {
    eval_count++;
    if (depth >= max_depth || a == b) return S_prev;
    double m = (a + b) / 2.0;
    if (m == a || m == b) return S_prev;
    double fm = f(m); eval_count++;
    double S_left = (f(a) + fm) * (m - a) / 2.0;
    double S_right = (fm + f(b)) * (b - m) / 2.0;
    double S_curr = S_left + S_right;
    if (std::abs(S_curr - S_prev) < 3.0 * tol) return S_curr + (S_curr - S_prev) / 3.0;
    return adaptive_trapezoidal(f, a, m, S_left, tol / 2.0, eval_count, depth + 1, max_depth) +
           adaptive_trapezoidal(f, m, b, S_right, tol / 2.0, eval_count, depth + 1, max_depth);
}

// Интегрирование подинтеравала
double integrate_single_task(double (*f)(double), double a, double b, double epsilon_sub, int& eval_count) {
    if (a == b) return 0.0;
    double fa = f(a), fb = f(b); eval_count += 2;
    return adaptive_trapezoidal(f, a, b, (fa + fb) * (b - a) / 2.0, epsilon_sub, eval_count);
}

// --- Логика работы потока ---
void* thread_worker(void* arg) {
    ThreadData* data = static_cast<ThreadData*>(arg);

// Локальные переменные для накопления результата внутри потока.
    double local_sum = 0.0; int local_eval_count = 0;
// Замер времени ЭТОГО потока (начало)
    auto thread_start_time = std::chrono::high_resolution_clock::now();

    // Цикл обработки задач
// ДИНАМИЧЕСКАЯ ОЧЕРЕДЬ
    while (true) {
        size_t task_idx = data->next_task_index->fetch_add(1);
        if (task_idx >= data->tasks_queue->size()) break;
        const Task& task = (*data->tasks_queue)[task_idx];
        local_sum += integrate_single_task(data->func, task.a, task.b, task.target_epsilon, local_eval_count);
    }
// Замер времени ЭТОГО потока (конец)
    auto thread_end_time = std::chrono::high_resolution_clock::now();
   // total время
    auto thread_duration = std::chrono::duration<double, std::milli>(thread_end_time - thread_start_time); // Время в миллисекундах
// Сохраняем результаты
    *(data->partial_sum) = local_sum;
    *(data->evaluations_count) = local_eval_count;
    *(data->execution_time_ms) = thread_duration.count(); // <-- Сохраняем время выполнения

    pthread_exit(NULL);
}


// MAIN

int main(int argc, char* argv[]) {
// Проверки / input переменные
    if (argc != 5) { std::cerr << "Usage: " << argv[0] << " <threads> <epsilon> <A> <B>\n"; return 1; }
    int num_threads = std::stoi(argv[1]); double epsilon_total = std::stod(argv[2]);
    double A = std::stod(argv[3]); double B = std::stod(argv[4]);
    if (num_threads <= 0 || epsilon_total <= 0 || A < 0 || B <= A) { std::cerr << "Invalid arguments.\n"; return 1; }

    std::cout << "Integrating sin(1/x) from " << A << " to " << B << " with "
              << num_threads << " threads, epsilon: " << epsilon_total << std::endl;
    auto overall_start_time = std::chrono::high_resolution_clock::now(); // Общее время выполнения

// Подготовка задач
    // вектор для хранения под-интервалов (задач)
    std::vector<Task> tasks_queue;
// Определяем начальное количество задач
    size_t num_initial_tasks = std::max(100, num_threads * 500);
// Примерная длина одного под-интервала
    double total_len = B - A; double dx_task = (total_len > 1e-9) ? (total_len / num_initial_tasks) : 0;
    if (dx_task > 0) {
        for (size_t i = 0; i < num_initial_tasks; ++i) {
             double task_a = A + i * dx_task;
             double task_b = (i == num_initial_tasks - 1) ? B : (A + (i + 1) * dx_task);
             if (task_b > B) task_b = B;
             if (task_a < task_b) tasks_queue.push_back({task_a, task_b, epsilon_total * ((task_b - task_a) / total_len)});
         }
    } else if (total_len >= 0) { tasks_queue.push_back({A, B, epsilon_total}); }

    // Инициализация данных для потоков
    std::atomic<size_t> next_task_index(0);
    std::vector<pthread_t> threads_arr(num_threads);
    std::vector<ThreadData> thread_data_arr(num_threads);
    std::vector<double> partial_sums(num_threads, 0.0);
    std::vector<int> thread_eval_counts(num_threads, 0);
    std::vector<double> thread_times_ms(num_threads, 0.0); // время потоков

    // Запуск рабочих потоков
    for (int i = 0; i < num_threads; ++i) {
        // Добавляем указатель на thread_times_ms[i] в ThreadData
        thread_data_arr[i] = {&tasks_queue, &next_task_index, &partial_sums[i], &thread_eval_counts[i], func_to_integrate, &thread_times_ms[i]};
        if (pthread_create(&threads_arr[i], NULL, thread_worker, &thread_data_arr[i])) {
            std::cerr << "Error creating thread " << i << std::endl; exit(-1);
        }
    }

    // Завершение потоков и сбор сумм интеграла
    double total_integral = 0.0;
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads_arr[i], NULL);
        total_integral += partial_sums[i];
    }

// Статистика для питона
    // Сбор статистики по ВРЕМЕНИ выполнения потоков
    long long total_evals_sum = 0; // Общее число вычислений все еще считаем
    double sum_thread_time_ms = 0;
    double min_thread_time_ms = 0, max_thread_time_ms = 0;
    if (num_threads > 0 && !thread_times_ms.empty()) { // Инициализация min/max
        min_thread_time_ms = thread_times_ms[0]; max_thread_time_ms = thread_times_ms[0];
    }

    for (int i = 0; i < num_threads; ++i) {
        total_evals_sum += thread_eval_counts[i]; // Суммируем вычисления
        sum_thread_time_ms += thread_times_ms[i];
        if (thread_times_ms[i] < min_thread_time_ms) min_thread_time_ms = thread_times_ms[i];
        if (thread_times_ms[i] > max_thread_time_ms) max_thread_time_ms = thread_times_ms[i];
        // Вывод для парсинга Python'ом (время потока)
        std::cout << "THREAD_TIME_MS: " << i << " " << std::fixed << std::setprecision(3) << thread_times_ms[i] << std::endl;
    }

    // Вывод статистики балансировки по ВРЕМЕНИ
    if (num_threads > 1) {
        double avg_time_ms = (num_threads > 0) ? (sum_thread_time_ms / num_threads) : 0.0;
        double spread_percentage = (avg_time_ms > 1e-6) ? // Используем мс, порог можно меньше
                                   ((max_thread_time_ms - min_thread_time_ms) / avg_time_ms) * 100.0
                                   : 0.0;
        std::cout << "Load Balancing (Time_ms): Min=" << std::fixed << std::setprecision(3) << min_thread_time_ms
                  << ", Max=" << std::fixed << std::setprecision(3) << max_thread_time_ms
                  << ", Avg=" << std::fixed << std::setprecision(3) << avg_time_ms
                  << ", Spread=" << std::fixed << std::setprecision(2) << spread_percentage << "%" << std::endl;
    } else if (num_threads == 1) {
         std::cout << "Load Balancing (Time_ms): N/A for single thread." << std::endl;
    }

    // Завершение общего замера времени и вывод результатов
    auto overall_time_ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - overall_start_time).count();
    std::cout << std::fixed << std::setprecision(10) << "Integral result: " << total_integral << std::endl;
    std::cout << "Total function evaluations: " << total_evals_sum << std::endl; // Оставляем для информации
    std::cout << "Total Wall Time (" << num_threads << " thr): " << std::fixed << std::setprecision(3) << overall_time_ms << " ms" << std::endl; // Переименовал для ясности

    // Вывод данных для Python-скрипта (используем общее время overall_time_ms)
    if (num_threads == 1) std::cout << "DATAPOINT_INTEGRAL_SINGLE: " << overall_time_ms << " " << total_evals_sum << std::endl;
    else std::cout << "DATAPOINT_INTEGRAL_MULTI: " << num_threads << " " << overall_time_ms << " " << total_evals_sum << std::endl;

    return 0;
}