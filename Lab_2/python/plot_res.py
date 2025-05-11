import matplotlib.pyplot as plt
import pandas as pd
import io
import os
import re # Для парсинга pipe и shared_mem

# Директории
DATA_DIR = "../data"  # Относительно python/
PLOTS_DIR = "../plots" # Относительно python/

# Убедимся, что директория для графиков существует
os.makedirs(PLOTS_DIR, exist_ok=True)

def parse_datapoints(filepath, prefix="DATAPOINT: ", num_cols=5, col_names=None):
    """Общая функция для парсинга строк DATAPOINT."""
    data_lines = []
    if not os.path.exists(filepath):
        print(f"Warning: File not found {filepath}")
        return pd.DataFrame(columns=col_names if col_names else [f"col{i}" for i in range(num_cols)])

    with open(filepath, "r") as f:
        content = f.readlines()
    
    for line in content:
        if line.startswith(prefix):
            data_lines.append(line.replace(prefix, "").strip())
    
    if not data_lines:
        print(f"Warning: No data points found with prefix '{prefix}' in {filepath}")
        return pd.DataFrame(columns=col_names if col_names else [f"col{i}" for i in range(num_cols)])

    data_str = "\n".join(data_lines)
    df = pd.read_csv(io.StringIO(data_str), sep=" ", header=None, names=col_names)
    return df

def plot_sort_performance():
    print("Plotting sort performance...")
    filepath = os.path.join(DATA_DIR, "sort_results.txt")
    df = parse_datapoints(filepath, 
                          col_names=["size", "threads", "qsort_ms", "stdsort_ms", "parallel_ms"])

    if df.empty:
        print("No data for sort plots.")
        return

    # График 1: Время от размера массива (для макс. числа потоков в данных)
    max_threads = df['threads'].max()
    df_fixed_threads = df[df['threads'] == max_threads]

    if not df_fixed_threads.empty:
        plt.figure(figsize=(10, 6))
        plt.plot(df_fixed_threads['size'], df_fixed_threads['qsort_ms'], marker='o', label='qsort')
        plt.plot(df_fixed_threads['size'], df_fixed_threads['stdsort_ms'], marker='s', label='std::sort (1 thr)')
        plt.plot(df_fixed_threads['size'], df_fixed_threads['parallel_ms'], marker='^', label=f'Parallel Sort ({max_threads} thr)')
        plt.xlabel('Размер массива')
        plt.ylabel('Время (мс)')
        plt.title(f'Сортировка: время от размера (паралл. для {max_threads} потоков)')
        plt.legend()
        plt.grid(True)
        plt.xscale('log')
        plt.yscale('log')
        plt.tight_layout()
        plt.savefig(os.path.join(PLOTS_DIR, "sort_vs_size.png"))
        plt.close()

    # График 2: Время от числа потоков (для макс. размера массива в данных)
    max_size = df['size'].max()
    df_fixed_size = df[df['size'] == max_size]

    if not df_fixed_size.empty:
        plt.figure(figsize=(10, 6))
        if not df_fixed_size.empty:
            plt.axhline(y=df_fixed_size['qsort_ms'].iloc[0], color='r', linestyle='--', label=f'qsort (size {max_size})')
            plt.axhline(y=df_fixed_size['stdsort_ms'].iloc[0], color='g', linestyle='--', label=f'std::sort (size {max_size})')
        
        plt.plot(df_fixed_size['threads'], df_fixed_size['parallel_ms'], marker='o', label=f'Parallel Sort (size {max_size})')
        plt.xlabel('Число потоков')
        plt.ylabel('Время (мс)')
        plt.title(f'Сортировка: время от числа потоков (размер {max_size})')
        plt.legend()
        plt.grid(True)
        if not df_fixed_size['threads'].empty:
             plt.xticks(sorted(df_fixed_size['threads'].unique()))
        plt.tight_layout()
        plt.savefig(os.path.join(PLOTS_DIR, "sort_vs_threads.png"))
        plt.close()
    print("Sort plots generated.")


def plot_integral_performance():
    print("Plotting integral performance...")
    filepath = os.path.join(DATA_DIR, "integral_results.txt")
    
    single_thread_data = parse_datapoints(filepath, prefix="DATAPOINT_INTEGRAL_SINGLE: ", num_cols=2, col_names=["time_ms", "evals"])
    multi_thread_data = parse_datapoints(filepath, prefix="DATAPOINT_INTEGRAL_MULTI: ", num_cols=3, col_names=["threads", "time_ms", "evals"])

    if single_thread_data.empty or multi_thread_data.empty:
        print("Not enough data for integral plots (need single and multi-thread results).")
        return

    single_thread_time = single_thread_data['time_ms'].iloc[0]
    
    # Добавляем данные для 1 потока в multi_thread_data для удобства построения
    # Используем concat, если single_thread_data не пустой
    df_integral = pd.concat([
        pd.DataFrame([{'threads': 1, 'time_ms': single_thread_time, 'evals': single_thread_data['evals'].iloc[0]}]),
        multi_thread_data
    ]).sort_values(by='threads').reset_index(drop=True)


    df_integral['speedup'] = single_thread_time / df_integral['time_ms']
    df_integral['efficiency'] = df_integral['speedup'] / df_integral['threads']

    plt.figure(figsize=(12, 5))
    plt.subplot(1, 2, 1)
    plt.plot(df_integral['threads'], df_integral['speedup'], marker='o', label='Ускорение (Speedup)')
    plt.plot(df_integral['threads'], df_integral['threads'], linestyle='--', color='gray', label='Идеальное ускорение')
    plt.xlabel('Число потоков')
    plt.ylabel('Ускорение')
    plt.title('Интегрирование: Ускорение')
    plt.legend()
    plt.grid(True)
    if not df_integral['threads'].empty: plt.xticks(sorted(df_integral['threads'].unique()))

    plt.subplot(1, 2, 2)
    plt.plot(df_integral['threads'], df_integral['efficiency'] * 100, marker='s', label='Эффективность (Efficiency)')
    plt.axhline(y=100, linestyle='--', color='gray', label='Идеальная эффективность (100%)')
    plt.xlabel('Число потоков')
    plt.ylabel('Эффективность (%)')
    plt.title('Интегрирование: Эффективность')
    plt.ylim(0, 110)
    plt.legend()
    plt.grid(True)
    if not df_integral['threads'].empty: plt.xticks(sorted(df_integral['threads'].unique()))

    plt.tight_layout()
    plt.savefig(os.path.join(PLOTS_DIR, "integral_performance.png"))
    plt.close()
    print("Integral plots generated.")

def display_communication_benchmarks():
    print("\n--- Communication Benchmarks ---")
    
    pipe_file = os.path.join(DATA_DIR, "pipe_results.txt")
    if os.path.exists(pipe_file):
        print("\nPipe Benchmark Results:")
        with open(pipe_file, "r") as f:
            print(f.read())
    else:
        print(f"\n{pipe_file} not found.")

    shared_mem_file = os.path.join(DATA_DIR, "shared_mem_results.txt")
    if os.path.exists(shared_mem_file):
        print("\nShared Memory (Threads) Benchmark Results:")
        with open(shared_mem_file, "r") as f:
            print(f.read())
    else:
        print(f"\n{shared_mem_file} not found.")
    print("------------------------------")


if __name__ == "__main__":
    # Создание директорий, если их нет
    os.makedirs(DATA_DIR, exist_ok=True)
    os.makedirs(PLOTS_DIR, exist_ok=True)

    plot_sort_performance()
    plot_integral_performance()
    display_communication_benchmarks() # Просто выводим текстовые результаты

    print(f"\nAll plots saved to: {os.path.abspath(PLOTS_DIR)}")
    print(f"All raw data saved in: {os.path.abspath(DATA_DIR)}")