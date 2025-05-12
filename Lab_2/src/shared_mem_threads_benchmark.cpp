
#include <iostream>
#include <vector>
#include <thread> // поток
#include <mutex> // синхро
#include <condition_variable> // условные переменных для ожидания/сигнализации между потоками
#include <chrono>
#include <numeric>
#include <atomic> // Для барьера
// чтобы исключить время создания потоков

// Барьер, чтобы синхронизировать потоки
class Barrier {
    std::mutex mtx;
    std::condition_variable cv;
    size_t count;
    const size_t initial_count;
    int generation = 0;

public:
// init барьер
    explicit Barrier(size_t initial_count_) : count(initial_count_), initial_count(initial_count_) {}

    void wait() {
        std::unique_lock<std::mutex> lock(mtx);
        int current_generation = generation;
        if (--count == 0) {
            generation++; // Сменить поколение
            count = initial_count; // Сбросить счетчик для nest step
            cv.notify_all();
        } else {
            cv.wait(lock, [&] { return generation != current_generation; });
        }
    }
};
// конц барьера
// ПАРАМЕТРЫ

const size_t DATA_SIZE = 4096; // Размер одного блока.
const int NUM_ROUND_TRIPS = 10000; // Количество полных циклов

std::vector<char> shared_buffer(DATA_SIZE); // разделяемая память
std::mutex mtx_comm; // Мьютекс для коммуникации
std::condition_variable cv_comm; // CV для коммуникации
// флаги
bool ping_turn = true;
bool data_ready = false;

Barrier setup_barrier(3); // Барьер для 3 потоков: main, ping, pong

// Поток "Ping" отправка в общий буфер 
void ping_thread_func() {
    std::vector<char> local_data(DATA_SIZE, 'P');
    setup_barrier.wait(); // Синхронизация перед началом работы

// Выполняем заданное количество циклов отправки.
    for (int i = 0; i < NUM_ROUND_TRIPS; ++i) {
        std::unique_lock<std::mutex> lock(mtx_comm);
        cv_comm.wait(lock, [] { return ping_turn; });
        if (DATA_SIZE > 0) shared_buffer = local_data;
        ping_turn = false; data_ready = true;
        lock.unlock();
        cv_comm.notify_one();
    }
}

// Поток "Pong"
void pong_thread_func() {
    std::vector<char> local_recv_buffer(DATA_SIZE);
    setup_barrier.wait(); // Синхронизация перед началом работы

// Выполняем заданное количество циклов приема.
    for (int i = 0; i < NUM_ROUND_TRIPS; ++i) {
        std::unique_lock<std::mutex> lock(mtx_comm);
        cv_comm.wait(lock, [] { return !ping_turn && data_ready; });
        if (DATA_SIZE > 0) local_recv_buffer = shared_buffer;
        ping_turn = true; data_ready = false;
        lock.unlock();
        cv_comm.notify_one();
    }
}

int main() {
    // 1. Создание потоков (не входит в итог время)
    std::thread t_ping(ping_thread_func);
    std::thread t_pong(pong_thread_func);

    // 2. Синхронизация:  пока все потоки будут готовы
    setup_barrier.wait();

    // 3. Запуск таймера и выполнение "пинг-понга"
    // Старт таймера
    auto start_time = std::chrono::high_resolution_clock::now();

    // 4. Ожидание завершения потоков (не входит в итог время)
    t_ping.join();
    t_pong.join();

    auto end_time = std::chrono::high_resolution_clock::now(); // <-- Таймер СТОП
    auto duration_us = std::chrono::duration<double, std::micro>(end_time - start_time).count();

    // 5. Вывод результатов
    double latency_us_round_trip = (NUM_ROUND_TRIPS > 0) ? (duration_us / NUM_ROUND_TRIPS) : 0;
    double total_data_mb = static_cast<double>(NUM_ROUND_TRIPS * 2 * DATA_SIZE) / (1024.0 * 1024.0);
    double throughput_mbps = (duration_us > 0 && DATA_SIZE > 0) ? (total_data_mb / (duration_us / 1e6)) : 0;

    std::cout << "Shared Memory (Threads) Benchmark (Communication Time):" << std::endl;
    std::cout << "  Round trips: " << NUM_ROUND_TRIPS << ", Data size/transfer: " << DATA_SIZE << " B" << std::endl;
    std::cout << "  Total communication time: " << duration_us << " us" << std::endl;
    std::cout << "  Avg latency/round_trip: " << latency_us_round_trip << " us" << std::endl;
    if (DATA_SIZE > 0) {
         std::cout << "  Approx Throughput: " << throughput_mbps << " MB/s" << std::endl;
    }

    return 0;
}