#!/bin/bash

# Выход при ошибке
set -e

# Директории
BIN_DIR="./bin"
DATA_DIR="./data"
PLOTS_DIR="./plots" # Хотя этот скрипт не создает графики напрямую

# Имена файлов для результатов
SORT_RESULTS_FILE="$DATA_DIR/sort_results.txt"
PIPE_RESULTS_FILE="$DATA_DIR/pipe_results.txt"
SHARED_MEM_RESULTS_FILE="$DATA_DIR/shared_mem_results.txt"
INTEGRAL_RESULTS_FILE="$DATA_DIR/integral_results.txt"

# 1. Сборка проекта
echo "Building C++ projects..."
make clean # Очистка перед сборкой (опционально)
make all
echo "Build complete."
echo ""

# 2. Создание директорий для данных и графиков (если их нет)
mkdir -p $DATA_DIR
mkdir -p $PLOTS_DIR

# 3. Очистка старых файлов результатов
rm -f $SORT_RESULTS_FILE $PIPE_RESULTS_FILE $SHARED_MEM_RESULTS_FILE $INTEGRAL_RESULTS_FILE

echo "Running experiments..."
echo ""

# --- Эксперименты для сортировки ---
echo "--- Running Sort Benchmarks ---"
SIZES_SORT=(100000 500000 1000000 5000000) # Размеры массивов
THREADS_SORT=(1 2 4 8) # Количество потоков (можно взять реальное число ядер `nproc`)
                                 # Для 1 потока параллельная сортировка часто работает как std::sort

for size in "${SIZES_SORT[@]}"; do
    for threads in "${THREADS_SORT[@]}"; do
        echo "Sorting: size=$size, threads=$threads"
        $BIN_DIR/parallel_sort_s $size $threads >> $SORT_RESULTS_FILE
    done
done
echo "Sort benchmarks finished. Results in $SORT_RESULTS_FILE"
echo ""

# --- Эксперимент для pipe ---
echo "--- Running Pipe Benchmark ---"
$BIN_DIR/pipe_benchmark_s > $PIPE_RESULTS_FILE # Перенаправляем весь вывод
echo "Pipe benchmark finished. Results in $PIPE_RESULTS_FILE"
echo ""

# --- Эксперимент для shared memory (threads) ---
echo "--- Running Shared Memory (Threads) Benchmark ---"
$BIN_DIR/shared_mem_benchmark_s > $SHARED_MEM_RESULTS_FILE # Перенаправляем весь вывод
echo "Shared memory benchmark finished. Results in $SHARED_MEM_RESULTS_FILE"
echo ""

# --- Эксперименты для интегрирования ---
echo "--- Running Integral Benchmarks ---"
EPSILON="1e-8"
A_LIMIT="0.01"
B_LIMIT="2.0"
THREADS_INTEGRAL=(1 2 4 8) # Добавьте больше значений, если нужно (например, nproc)
                            # Убедитесь, что есть замер для 1 потока (базовый)

echo "Integral params: Epsilon=$EPSILON, A=$A_LIMIT, B=$B_LIMIT"
for threads in "${THREADS_INTEGRAL[@]}"; do
    echo "Integrating: threads=$threads"
    $BIN_DIR/integral_pthread_s $threads $EPSILON $A_LIMIT $B_LIMIT >> $INTEGRAL_RESULTS_FILE
done
echo "Integral benchmarks finished. Results in $INTEGRAL_RESULTS_FILE"
echo ""

echo "All experiments complete."
echo "To generate plots, run: python python/plot_results.py"