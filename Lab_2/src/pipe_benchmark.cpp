#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <numeric>   // std::iota
#include <cstdio>    // perror
#include <cstdlib>   // exit, EXIT_FAILURE
#include <fcntl.h>   // _O_BINARY

#ifdef _WIN32
    #include <io.h>
    // Используем макросы для краткости и совместимости имен
    #define pipe(fds, size, mode) _pipe(fds, size, mode)
    #define read _read
    #define write _write
    #define close _close
    #define STDIN_FILENO _fileno(stdin) // Обычно 0, но для ясности
    #define STDOUT_FILENO _fileno(stdout) // Обычно 1
#else // POSIX
    #include <unistd.h>
#endif

const size_t MESSAGE_SIZE = 4096;
const int NUM_MESSAGES = 10000;
int pipe_fds[2]; // [0] for read, [1] for write

void die(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void reader_thread_func() {
    std::vector<char> buffer(MESSAGE_SIZE);
    for (int i = 0; i < NUM_MESSAGES; ++i) {
        int bytes_read = read(pipe_fds[0], buffer.data(), MESSAGE_SIZE);
        if (bytes_read <= 0) { // Ошибка или EOF
            if (bytes_read < 0) perror("read from pipe in reader_thread");
            break;
        }
    }
    close(pipe_fds[0]);
}

void writer_thread_func() {
    std::vector<char> message(MESSAGE_SIZE, 'X'); // Заполняем данными
    for (int i = 0; i < NUM_MESSAGES; ++i) {
        int bytes_written = write(pipe_fds[1], message.data(), MESSAGE_SIZE);
        if (bytes_written < static_cast<int>(MESSAGE_SIZE)) { // Ошибка или частичная запись
            if (bytes_written < 0) perror("write to pipe in writer_thread");
            else std::cerr << "Writer: Partial write occurred." << std::endl;
            break;
        }
    }
    close(pipe_fds[1]); // Важно для EOF у читателя
}

int main() {
#ifdef _WIN32
    if (pipe(pipe_fds, 0, _O_BINARY) == -1) die("_pipe creation failed"); // 0 для размера буфера по умолчанию
#else
    if (pipe(pipe_fds) == -1) die("pipe creation failed");
#endif

    auto start_time = std::chrono::high_resolution_clock::now();

    std::thread reader(reader_thread_func);
    std::thread writer(writer_thread_func);

    writer.join(); // Убедимся, что писатель закончил и закрыл свой конец
    reader.join(); // Затем читатель должен увидеть EOF и закончить

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration<double, std::micro>(end_time - start_time).count();

    double total_data_mb = static_cast<double>(NUM_MESSAGES * MESSAGE_SIZE) / (1024.0 * 1024.0);
    double duration_s = duration_us / 1e6;
    double throughput_mbps = (duration_s > 0) ? (total_data_mb / duration_s) : 0;
    double latency_us_msg = (NUM_MESSAGES > 0) ? (duration_us / NUM_MESSAGES) : 0;

    std::cout << "Pipe-like Benchmark (Threads):" << std::endl;
    std::cout << "  Messages: " << NUM_MESSAGES << ", Size: " << MESSAGE_SIZE << " B" << std::endl;
    std::cout << "  Total time: " << duration_us << " us (" << duration_s << " s)" << std::endl;
    std::cout << "  Throughput: " << throughput_mbps << " MB/s" << std::endl;
    std::cout << "  Avg latency/msg: " << latency_us_msg << " us" << std::endl;

    return 0;
}