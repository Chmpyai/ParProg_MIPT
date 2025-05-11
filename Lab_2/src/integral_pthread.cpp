#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <pthread.h>
#include <chrono>
#include <atomic>
#include <algorithm> // std::max

// --- Глобальные определения и структуры ---
// Интегрируемая функция
double func_to_integrate(double x) { if (x == 0.0) return 0.0; return sin(1.0 / x); }
// Описание задачи (подынтервала) для интегрирования
struct Task { double a, b, target_epsilon; };
// Данные, передаваемые каждому потоку
struct ThreadData {
    std::vector<Task>* tasks_queue; std::atomic<size_t>* next_task_index;
    double* partial_sum; int* evaluations_count; double (*func)(double);
};

// --- Численный метод интегрирования ---
// Адаптивный метод трапеций: рекурсивно делит интервал для достижения нужной точности
double adaptive_trapezoidal(double (*f)(double), double a, double b, double S_prev, double tol, int& eval_count, int depth = 0, int max_depth = 20) {
    eval_count++;
    if (depth >= max_depth || a == b) return S_prev; // Условие остановки рекурсии
    double m = (a + b) / 2.0;
    if (m == a || m == b) return S_prev; // Интервал слишком мал
    double fm = f(m); eval_count++;
    double S_left = (f(a) + fm) * (m - a) / 2.0;
    double S_right = (fm + f(b)) * (b - m) / 2.0;
    double S_curr = S_left + S_right;
    // Проверка точности и возможное дальнейшее деление или возврат результата
    if (std::abs(S_curr - S_prev) < 3.0 * tol) return S_curr + (S_curr - S_prev) / 3.0; // Уточнение Ричардсона
    return adaptive_trapezoidal(f, a, m, S_left, tol / 2.0, eval_count, depth + 1, max_depth) +
           adaptive_trapezoidal(f, m, b, S_right, tol / 2.0, eval_count, depth + 1, max_depth);
}

// Вычисляет интеграл для одной задачи (подынтервала)
double integrate_single_task(double (*f)(double), double a, double b, double epsilon_sub, int& eval_count) {
    if (a == b) return 0.0;
    double fa = f(a), fb = f(b); eval_count += 2; // Начальные вычисления функции
    return adaptive_trapezoidal(f, a, b, (fa + fb) * (b - a) / 2.0, epsilon_sub, eval_count); // Запуск адаптивного метода
}

// --- Логика работы потока ---
// Функция, выполняемая каждым потоком: берет задачи из очереди и обрабатывает их
void* thread_worker(void* arg) {
    ThreadData* data = static_cast<ThreadData*>(arg);
    double local_sum = 0.0; int local_eval_count = 0;
    // Цикл обработки задач из общей очереди
    while (true) {
        size_t task_idx = data->next_task_index->fetch_add(1); // Атомарно получаем индекс следующей задачи
        if (task_idx >= data->tasks_queue->size()) break;      // Если задачи кончились, выходим
        const Task& task = (*data->tasks_queue)[task_idx];     // Получаем задачу
        local_sum += integrate_single_task(data->func, task.a, task.b, task.target_epsilon, local_eval_count); // Вычисляем интеграл для задачи
    }
    *(data->partial_sum) = local_sum;                 // Сохраняем частичную сумму
    *(data->evaluations_count) = local_eval_count;    // Сохраняем количество вычислений
    pthread_exit(NULL);
}

// --- Основная программа ---
int main(int argc, char* argv[]) {
    // Парсинг аргументов командной строки
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <threads> <epsilon> <A> <B>\n"; return 1;
    }
    int num_threads = std::stoi(argv[1]); double epsilon_total = std::stod(argv[2]);
    double A = std::stod(argv[3]); double B = std::stod(argv[4]);

    // Валидация входных данных
    if (num_threads <= 0 || epsilon_total <= 0 || A < 0 || B <= A) { // A может быть 0, если не строгий интеграл, но для sin(1/x) обычно A > 0
        std::cerr << "Invalid arguments.\n"; return 1;
    }

    std::cout << "Integrating sin(1/x) from " << A << " to " << B << " with "
              << num_threads << " threads, epsilon: " << epsilon_total << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now(); // Засекаем время начала

    // Подготовка задач для параллельной обработки
    std::vector<Task> tasks_queue;
    size_t num_initial_tasks = std::max(100, num_threads * 500); // Количество мелких задач для лучшей балансировки
    double total_len = B - A; double dx_task = (total_len > 1e-9) ? (total_len / num_initial_tasks) : 0; // Дельта для каждой задачи
    if (dx_task > 0) { // Только если интервал не нулевой
        for (size_t i = 0; i < num_initial_tasks; ++i) {
            double task_a = A + i * dx_task;
            double task_b = (i == num_initial_tasks - 1) ? B : (A + (i + 1) * dx_task);
            if (task_b > B) task_b = B; // Коррекция последнего интервала
            if (task_a < task_b) // Добавляем только непустые интервалы
                tasks_queue.push_back({task_a, task_b, epsilon_total * ((task_b - task_a) / total_len)});
        }
    } else if (total_len <= 1e-9 && total_len >= 0) { // Если интервал нулевой или очень мал
         tasks_queue.push_back({A, B, epsilon_total}); // Одна задача для всего интервала
    }


    // Инициализация данных для потоков
    std::atomic<size_t> next_task_index(0); // Атомарный счетчик для выбора задач
    std::vector<pthread_t> threads_arr(num_threads);
    std::vector<ThreadData> thread_data_arr(num_threads);
    std::vector<double> partial_sums(num_threads, 0.0);
    std::vector<int> thread_eval_counts(num_threads, 0);

    // Запуск рабочих потоков
    for (int i = 0; i < num_threads; ++i) {
        thread_data_arr[i] = {&tasks_queue, &next_task_index, &partial_sums[i], &thread_eval_counts[i], func_to_integrate};
        if (pthread_create(&threads_arr[i], NULL, thread_worker, &thread_data_arr[i])) {
            std::cerr << "Error creating thread " << i << std::endl; exit(-1);
        }
    }

    // Ожидание завершения всех потоков и сбор частичных сумм интеграла
    double total_integral = 0.0;
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads_arr[i], NULL);
        total_integral += partial_sums[i];
    }
    
    // Сбор статистики по количеству вычислений функции для анализа балансировки
    long long total_evals_sum = 0; double sum_evals_avg = 0;
    long long min_evals = 0, max_evals = 0;
    if (num_threads > 0 && !thread_eval_counts.empty()) { // Инициализация min/max
        min_evals = thread_eval_counts[0]; max_evals = thread_eval_counts[0];
    }

    for (int i = 0; i < num_threads; ++i) {
        total_evals_sum += thread_eval_counts[i]; sum_evals_avg += thread_eval_counts[i];
        if (thread_eval_counts[i] < min_evals) min_evals = thread_eval_counts[i];
        if (thread_eval_counts[i] > max_evals) max_evals = thread_eval_counts[i];
        std::cout << "THREAD_EVALS: " << i << " " << thread_eval_counts[i] << std::endl; // Для парсинга Python
    }

    // Вывод статистики балансировки
    if (num_threads > 1) {
        double avg = (sum_evals_avg / num_threads);
        double spread = (avg > 1e-9) ? ((double)(max_evals - min_evals) / avg) * 100.0 : 0.0;
        std::cout << "Load Balancing (Evals): Min=" << min_evals << ", Max=" << max_evals
                  << ", Avg=" << std::fixed << std::setprecision(2) << avg
                  << ", Spread=" << spread << "%" << std::endl;
    } else if (num_threads == 1) {
        std::cout << "Load Balancing (Evals): N/A for single thread." << std::endl;
    }

    // Завершение замера времени и вывод результатов
    auto time_ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time).count();
    std::cout << std::fixed << std::setprecision(10) << "Integral result: " << total_integral << std::endl;
    std::cout << "Total function evaluations: " << total_evals_sum << std::endl;
    std::cout << "Time (" << num_threads << " thr): " << time_ms << " ms" << std::endl;

    // Вывод данных для Python-скрипта (для построения графиков ускорения)
    if (num_threads == 1) std::cout << "DATAPOINT_INTEGRAL_SINGLE: " << time_ms << " " << total_evals_sum << std::endl;
    else std::cout << "DATAPOINT_INTEGRAL_MULTI: " << num_threads << " " << time_ms << " " << total_evals_sum << std::endl;
    
    return 0;
}