#include <iostream>
#include <vector>
#include <algorithm> // для std::sort,
#include <chrono>    
#include <thread>    
#include <random>    // для ген случ числ

// Ген вектор случ чис
std::vector<int> generate_random_vector(size_t size, int min_val = 0, int max_val = 1000000) {
    std::vector<int> vec(size);
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> distrib(min_val, max_val);
    for (size_t i = 0; i < size; ++i) vec[i] = distrib(gen);
    return vec;
}

// Сортировка в отдельном потоке
void sort_chunk(std::vector<int>::iterator begin, std::vector<int>::iterator end) {
    std::sort(begin, end);
}

// Параллельная сортировка слиянием
void parallel_merge_sort(std::vector<int>& vec, int num_threads) {
    if (num_threads <= 0 || vec.size() < static_cast<size_t>(num_threads * 2)) {
        std::sort(vec.begin(), vec.end()); // Если мало элементов или потоков, используем std::sort
        return;
    }
 // хранение потоков + хранение начало/конец
    size_t chunk_size = vec.size() / num_threads; 
    std::vector<std::thread> threads;
    std::vector<std::pair<std::vector<int>::iterator, std::vector<int>::iterator>> chunks;
// делим вектор на части
    auto it = vec.begin();
    for (int i = 0; i < num_threads; ++i) {
        auto chunk_end = (i == num_threads - 1) ? vec.end() : (it + chunk_size);
// Сохраняем пару итераторов (начало 'it', конец 'chunk_end') в вектор 'chunks'.        
        chunks.push_back({it, chunk_end});
        threads.emplace_back(sort_chunk, it, chunk_end); // Запускаем сортировку частей в потоках
        it = chunk_end;
    }

    for (auto& t : threads) t.join(); // Ожидаем завершения всех потоков

    // Последовательно сливаем отсортированные части в ОСНОВНОМ потоке
    for (size_t i = 1; i < chunks.size(); ++i) {
        std::inplace_merge(vec.begin(), chunks[i].first, chunks[i].second);
    }
}

// Функция сравнения для qsort
int compare_ints_qsort(const void* a, const void* b) {
    int arg1 = *static_cast<const int*>(a);
    int arg2 = *static_cast<const int*>(b);
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

// MAIN
// первый аргумент (argv[1]) размер вектора.
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <vector_size> [num_threads]" << std::endl;
        return 1;
    }
// Кол-во потоков для сортировки (или равно колыу ядер)
    size_t vector_size = std::stoul(argv[1]);
    int num_threads = (argc > 2) ? std::stoi(argv[2]) : std::thread::hardware_concurrency();
    if (num_threads <= 0) num_threads = 1;

    std::cout << "Vector size: " << vector_size << ", Parallel sort threads: " << num_threads << std::endl;

    std::vector<int> v_orig = generate_random_vector(vector_size);
    std::vector<int> v_parallel = v_orig;
    std::vector<int> v_qsort = v_orig;
    std::vector<int> v_stdsort = v_orig;
// Генерируем исходный вектор со случайными данными + копируем его для 
    // std::qsort
    auto start_q = std::chrono::high_resolution_clock::now();
    qsort(v_qsort.data(), v_qsort.size(), sizeof(int), compare_ints_qsort);
    auto qsort_time = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_q).count();
    std::cout << "qsort time: " << qsort_time << " ms" << std::endl;

    // std::sort (однопоточный)
    auto start_std = std::chrono::high_resolution_clock::now();
    std::sort(v_stdsort.begin(), v_stdsort.end());
    auto stdsort_time = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_std).count();
    std::cout << "std::sort time: " << stdsort_time << " ms" << std::endl;

    // parallel_merge_sort
    auto start_p = std::chrono::high_resolution_clock::now();
    parallel_merge_sort(v_parallel, num_threads);
    auto parallel_time = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_p).count();
    std::cout << "Parallel sort time (" << num_threads << " threads): " << parallel_time << " ms" << std::endl;

    // Проверка
    if (!std::is_sorted(v_parallel.begin(), v_parallel.end())) std::cerr << "Parallel sort FAILED!" << std::endl;
    if (!std::is_sorted(v_qsort.begin(), v_qsort.end())) std::cerr << "qsort FAILED!" << std::endl;

    // Для Python скрипта
    std::cout << "DATAPOINT: " << vector_size << " " << num_threads << " "
              << qsort_time << " " << stdsort_time << " " << parallel_time << std::endl;
    return 0;
}