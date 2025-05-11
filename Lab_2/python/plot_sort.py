import matplotlib.pyplot as plt
import pandas as pd
import io

# Пример данных, если запускать Python скрипт отдельно
# Если читаете из файла, созданного C++ программой:
# with open("data.txt", "r") as f:
#    content = f.readlines()
# data_lines = [line for line in content if line.startswith("DATAPOINT:")]
# data_str = "".join([line.replace("DATAPOINT: ", "") for line in data_lines])
# df = pd.read_csv(io.StringIO(data_str), sep=" ", header=None,
#                  names=["size", "threads", "qsort_ms", "stdsort_ms", "parallel_ms"])


# Вставьте сюда вывод вашей программы (строки DATAPOINT) или прочитайте из файла
data_str = """
1000000 1 200.0 70.0 75.0
1000000 2 201.0 71.0 45.0
1000000 4 202.0 72.0 30.0
1000000 8 203.0 73.0 25.0
5000000 4 1200.0 400.0 150.0
10000000 4 2500.0 850.0 320.0
20000000 4 5300.0 1800.0 680.0
""" # Замените эти данные на ваши!

df = pd.read_csv(io.StringIO(data_str.strip()), sep=" ", header=None,
                 names=["size", "threads", "qsort_ms", "stdsort_ms", "parallel_ms"])

print(df)

# График 1: Время от размера массива (для фиксированного числа потоков, например, 4)
df_fixed_threads = df[df['threads'] == df['threads'].unique()[2 if len(df['threads'].unique()) > 2 else 0]] # Берем 4 потока если есть

if not df_fixed_threads.empty:
    plt.figure(figsize=(10, 6))
    plt.plot(df_fixed_threads['size'], df_fixed_threads['qsort_ms'], marker='o', label='qsort')
    plt.plot(df_fixed_threads['size'], df_fixed_threads['stdsort_ms'], marker='s', label='std::sort (1 thread)')
    plt.plot(df_fixed_threads['size'], df_fixed_threads['parallel_ms'], marker='^', label=f'Parallel Sort ({df_fixed_threads["threads"].iloc[0]} threads)')
    plt.xlabel('Размер массива')
    plt.ylabel('Время (мс)')
    plt.title(f'Производительность сортировки от размера массива ({df_fixed_threads["threads"].iloc[0]} потоков для параллельной)')
    plt.legend()
    plt.grid(True)
    plt.xscale('log') # Размеры могут сильно отличаться
    plt.yscale('log')
    plt.tight_layout()
    plt.savefig("sort_performance_vs_size.png")
    plt.show()

# График 2: Время от числа потоков (для фиксированного размера массива)
# Выберите один из размеров, для которого есть данные с разным числом потоков
fixed_size_data = df[df['size'] == df['size'].unique()[0]] # Берем первый встреченный размер

if not fixed_size_data.empty:
    plt.figure(figsize=(10, 6))
    # qsort и std::sort не зависят от числа потоков, но можно показать их время как базовое
    if not fixed_size_data.empty:
         plt.axhline(y=fixed_size_data['qsort_ms'].iloc[0], color='r', linestyle='--', label=f'qsort (size {fixed_size_data["size"].iloc[0]})')
         plt.axhline(y=fixed_size_data['stdsort_ms'].iloc[0], color='g', linestyle='--', label=f'std::sort (size {fixed_size_data["size"].iloc[0]})')
    
    plt.plot(fixed_size_data['threads'], fixed_size_data['parallel_ms'], marker='o', label=f'Parallel Sort (size {fixed_size_data["size"].iloc[0]})')
    plt.xlabel('Число потоков')
    plt.ylabel('Время (мс)')
    plt.title(f'Производительность параллельной сортировки от числа потоков (размер массива {fixed_size_data["size"].iloc[0]})')
    plt.legend()
    plt.grid(True)
    plt.xticks(fixed_size_data['threads'].unique()) # Показать все значения потоков на оси X
    plt.tight_layout()
    plt.savefig("sort_performance_vs_threads.png")
    plt.show()