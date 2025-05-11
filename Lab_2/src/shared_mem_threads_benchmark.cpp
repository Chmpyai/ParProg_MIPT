#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

// Для чистого замера синхронизации, DATA_SIZE можно сделать 0 или малым.
// Для имитации передачи данных, оставляем его > 0.
const size_t DATA_SIZE = 4096; // Размер данных (может быть 0 для чистого теста сигнала)
const int NUM_ROUND_TRIPS = 10000;

std::vector<char> shared_buffer(DATA_SIZE); // Общий буфер
std::mutex mtx;
std::condition_variable cv;
bool ping_turn = true;    // true: очередь ping, false: очередь pong
bool data_ready = false;  // Флаг готовности данных

// Поток "Ping"
void ping_thread_func() {
    std::vector<char> local_data(DATA_SIZE, 'P'); // Локальные данные для "отправки"
    for (int i = 0; i < NUM_ROUND_TRIPS; ++i) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return ping_turn; }); // Ждем своей очереди

        if (DATA_SIZE > 0) shared_buffer = local_data; // "Отправка" данных (копирование)
        
        ping_turn = false; // Передаем очередь pong
        data_ready = true; // Данные готовы для pong
        lock.unlock();
        cv.notify_one();   // Уведомляем pong
    }
}

// Поток "Pong"
void pong_thread_func() {
    std::vector<char> local_recv_buffer(DATA_SIZE); // Локальный буфер для "приема"
    for (int i = 0; i < NUM_ROUND_TRIPS; ++i) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return !ping_turn && data_ready; }); // Ждем своей очереди и готовности данных

        if (DATA_SIZE > 0) local_recv_buffer = shared_buffer; // "Прием" данных (копирование)

        ping_turn = true;  // Передаем очередь ping
        data_ready = false; // Данные "обработаны"
        lock.unlock();
        cv.notify_one();   // Уведомляем ping
    }
}

int main() {
    auto start_time = std::chrono::high_resolution_clock::now();

    // Pong должен быть готов ждать перед первым notify от Ping
    // Установим начальное состояние так, чтобы Ping начал первым
    ping_turn = true;
    data_ready = false;

    std::thread t_ping(ping_thread_func);
    std::thread t_pong(pong_thread_func);

    t_ping.join();
    t_pong.join();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration<double, std::micro>(end_time - start_time).count();

    double latency_us_round_trip = duration_us / NUM_ROUND_TRIPS;

    std::cout << "Shared Memory (Threads) Benchmark (Simplified):" << std::endl;
    std::cout << "  Round trips: " << NUM_ROUND_TRIPS << ", Data size/transfer: " << DATA_SIZE << " B" << std::endl;
    std::cout << "  Total time: " << duration_us << " us" << std::endl;
    std::cout << "  Avg latency/round_trip: " << latency_us_round_trip << " us" << std::endl;
    if (DATA_SIZE > 0 && duration_us > 0) {
         double total_data_mb = static_cast<double>(NUM_ROUND_TRIPS * 2 * DATA_SIZE) / (1024 * 1024);
         double throughput_mbps = total_data_mb / (duration_us / 1e6);
         std::cout << "  Approx Throughput: " << throughput_mbps << " MB/s" << std::endl;
    }
    return 0;
}