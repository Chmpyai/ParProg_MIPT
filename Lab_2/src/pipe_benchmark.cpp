#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <numeric>
#include <cstdio> // ввод/вывод (perror)
#include <cstdlib> // (exit, EXIT_FAILURE)
#include <fcntl.h> // Чисто под WIN Содержит определения для управления файловыми дескрипторами
#include <atomic>

// Чисто для WIN
// Переопределение системныхPOSIX на аналоги в Windows (_pipe, _read, _write, _close)
#ifdef _WIN32
    #include <io.h>
    #define pipe(fds, size, mode) _pipe(fds, size, mode)
    #define read _read
    #define write _write
    #define close _close
#else
    #include <unistd.h>
#endif

const size_t MESSAGE_SIZE = 4096;  // Размер одного сообщения
const int NUM_MESSAGES = 10000; // Количество сообщений для теста.
int pipe_fds[2]; // дискрипторы[0] read, [1] write
std::atomic<bool> reader_ready(false); // Сигнал, что читатель готов

// msg error
void die(const char* msg) { perror(msg); exit(EXIT_FAILURE); }

// Поток-чтение из pipe
void reader_thread_func() {
    std::vector<char> buffer(MESSAGE_SIZE);
    reader_ready.store(true); // Сообщаем о готовности

    for (int i = 0; i < NUM_MESSAGES; ++i) {
        int bytes_read = read(pipe_fds[0], buffer.data(), MESSAGE_SIZE);
    }

    // пайп закроется при выходе из программы или закроем в main.
}

// Поток-запись в отдельном потоке (измеряет время своей работы)
// длительность записи в мкс

double writer_thread_func_timed() {

// переменные
    std::vector<char> message(MESSAGE_SIZE, 'X');
    long total_bytes_written = 0;
    std::chrono::high_resolution_clock::time_point start_time, end_time;

    // Ожидание готовности чтения
    while (!reader_ready.load()) {
        std::this_thread::yield(); // Уступить
    }

    start_time = std::chrono::high_resolution_clock::now(); // <-- Запускаем таймер

    for (int i = 0; i < NUM_MESSAGES; ++i) {
        int bytes_written = write(pipe_fds[1], message.data(), MESSAGE_SIZE);
        if (bytes_written < static_cast<int>(MESSAGE_SIZE)) {
            if (bytes_written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) perror("write to pipe in writer");
            else std::cerr << "Writer: Partial write occurred." << std::endl;
            // Если произошла ошибка, останавливаем таймер и выходим
             end_time = std::chrono::high_resolution_clock::now(); // <-- Остановка по ошибке
             goto end_loop; // Используем goto для простоты выхода из цикла и замера
        }
        total_bytes_written += bytes_written;
    }

    end_time = std::chrono::high_resolution_clock::now(); // <-- Останавливаем таймер

end_loop:
    close(pipe_fds[1]); // Закрываем пишущий конец, чтобы читатель получил EOF

    return std::chrono::duration<double, std::micro>(end_time - start_time).count();
}

int main() {
    // 1. Создание пайпа (не входит в измеряемое время)
#ifdef _WIN32
    if (pipe(pipe_fds, 0, _O_BINARY) == -1) die("_pipe creation failed");
#else
    if (pipe(pipe_fds) == -1) die("pipe creation failed");
#endif

    // 2. Создание потоков (не входит в измеряемое время)
    std::thread reader(reader_thread_func);

    // Запускаем писателя и получаем время выполнения
    double writer_duration_us = 0;

    // Обертка лямбда, чтобы передать ссылку на результат
    std::thread writer([&writer_duration_us](){
        writer_duration_us = writer_thread_func_timed();
    });


    // 3. Ожидание завершения потоков (не входит в итогоговое время)
    writer.join();
    reader.join();

    // 4. Закрытие оставшегося дескриптора пайпа (если читатель не закрыл)
    close(pipe_fds[0]);

    // 5. Вывод результатов на основе времени работы писателя
    double total_data_mb = static_cast<double>(NUM_MESSAGES * MESSAGE_SIZE) / (1024.0 * 1024.0);
    double duration_s = writer_duration_us / 1e6;
    double throughput_mbps = (duration_s > 0) ? (total_data_mb / duration_s) : 0;
    double latency_us_msg = (NUM_MESSAGES > 0) ? (writer_duration_us / NUM_MESSAGES) : 0;

    std::cout << "Pipe-like Benchmark (Threads - Writer Loop Time):" << std::endl;
    std::cout << "  Messages: " << NUM_MESSAGES << ", Size: " << MESSAGE_SIZE << " B" << std::endl;
    std::cout << "  Writer loop time: " << writer_duration_us << " us (" << duration_s << " s)" << std::endl;
    std::cout << "  Approx Throughput (based on writer time): " << throughput_mbps << " MB/s" << std::endl;
    std::cout << "  Avg Latency/msg (based on writer time): " << latency_us_msg << " us" << std::endl;

    return 0;
}