import os
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def run_performance_test():
    # Пути
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    build_dir = os.path.join(base_dir, "build")
    results_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "lab_main.csv")

    # Сборка проекта
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    os.chdir(build_dir)
    subprocess.run(["cmake", ".."], check=True)
    subprocess.run(["cmake", "--build", ".", "--config", "Release"], check=True)

    # Параметры тестирования
    num_processes_to_test = [1, 2, 3, 4, 5, 6, 7, 8]
    grid_sizes = [400, 500, 600, 700, 800, 900, 1000]
    
    results_data = []
    
    for grid in grid_sizes:
        print(f"\n--- Тестирование для сетки {grid}x{grid} ---")
        
        seq_time_for_this_grid = None # Базовое время для текущей сетки

        for procs_count in num_processes_to_test:
            print(f"\nЗапуск: {procs_count} процесс(ов), сетка {grid}x{grid}")
            
            # Путь к исполняемым файлам теперь относительно build_dir
            executable_path = os.path.join(build_dir, "task1_3_2.exe")
            if not os.path.exists(executable_path):
                print(f"Ошибка: Исполняемый файл {executable_path} не найден!")
                # Прервать тестирование для этой сетки, если исполняемого файла нет
                # Можно добавить break здесь, если для этой сетки продолжать нет смысла
                continue 

            run_args = ["mpiexec", "-n", str(procs_count), executable_path, str(grid), str(grid)]
            
            try:
                result = subprocess.run(
                    run_args, 
                    capture_output=True, text=True, encoding='utf-8', check=True, timeout=300 # Добавлен таймаут
                )
            except subprocess.CalledProcessError as e:
                print(f"Ошибка выполнения для {procs_count} процессов, сетка {grid}:")
                print("Команда:", " ".join(e.cmd))
                print("STDOUT:", e.stdout)
                print("STDERR:", e.stderr)
                continue # Пропустить эту точку
            except subprocess.TimeoutExpired:
                print(f"Таймаут выполнения для {procs_count} процессов, сетка {grid}.")
                continue

            output = result.stdout
            # Раскомментируйте для подробного вывода каждого запуска
            # print(f"Вывод ({procs_count} проц., сетка {grid}):\n{output}")
            if result.stderr:
               print(f"STDERR ({procs_count} проц., сетка {grid}):\n{result.stderr}")
            
            time_lines = [line for line in output.split('\n') if "Время:" in line]
            if not time_lines:
                print(f"Не удалось найти строку с временем для {procs_count} процессов, сетка {grid}! Пропускаю точку.")
                print(f"Полный вывод:\n{output}")
                continue
            
            time_line = time_lines[0] # Берем первую найденную строку
            try:
                current_time_value = float(time_line.split("Время:")[1].split("с")[0].strip())
            except (IndexError, ValueError) as e:
                print(f"Ошибка парсинга времени для {procs_count} проц., сетка {grid}: '{time_line}'. Ошибка: {e}. Пропускаю точку.")
                print(f"Полный вывод:\n{output}")
                continue

            if procs_count == 1:
                seq_time_for_this_grid = current_time_value
                if seq_time_for_this_grid == 0:
                    print(f"[ВНИМАНИЕ] Базовое время (1 процесс) = 0 для сетки={grid}. "
                          f"Расчет ускорения для этой сетки невозможен. Пропускаю эту сетку.")
                    break # Прервать внутренний цикл по процессам для этой сетки, перейти к следующей grid

            # Эта проверка должна быть после того, как seq_time_for_this_grid могло быть установлено
            if seq_time_for_this_grid is None: 
                # Это может случиться, если num_processes_to_test не начинается с 1, 
                # или если первый запуск (procs_count=1) провалился до установки seq_time_for_this_grid
                print(f"Критическая ошибка: Базовое время (seq_time_for_this_grid) не установлено для сетки {grid}. "
                      f"Пропускаю расчет для {procs_count} процессов.")
                continue 

            # Расчет ускорения и эффективности
            speedup = np.nan # По умолчанию NaN, если что-то пойдет не так
            efficiency = np.nan

            if seq_time_for_this_grid == 0:
                 # Это уже должно было привести к break выше, но для надежности
                 print(f"[ВНИМАНИЕ] Базовое время = 0. Ускорение и эффективность будут NaN.")
            elif current_time_value == 0:
                print(f"[ВНИМАНИЕ] Время выполнения = 0 для процессов={procs_count}, сетка={grid}. Ускорение и эффективность будут inf/NaN.")
                speedup = float('inf') # или np.nan, если не хотим inf на графиках
                efficiency = float('inf') # или np.nan
            else:
                speedup = seq_time_for_this_grid / current_time_value
                efficiency = speedup / procs_count
            
            results_data.append({
                'Процессы': procs_count,
                'Сетка': grid,
                'Время (с)': current_time_value,
                'Ускорение': speedup,
                'Эффективность': efficiency
            })
            
            print(f"Время = {current_time_value:.4f} с")
            if procs_count > 0: # Это условие всегда истинно здесь
                 print(f"Ускорение = {speedup:.2f} (Базовое время для сетки {grid}x{grid}: {seq_time_for_this_grid:.4f} с)")
                 print(f"Эффективность = {efficiency:.2f}") 
    


    # Сохранение результатов
    df = pd.DataFrame(results_data)
    # Фильтрация строк с бесконечными значениями
    df = df.replace([float('inf'), float('-inf')], pd.NA).dropna()
    df.to_csv(results_file, index=False)
    print(f"\nРезультаты сохранены в {results_file}")

    # Визуализация
    plt.figure(figsize=(15, 5))

    # График времени выполнения
    plt.subplot(1, 2, 1)
    for grid in grid_sizes:
        data = df[df['Сетка'] == grid]
        plt.plot(data['Процессы'], data['Время (с)'], 
                marker='o', label=f'Сетка {grid}x{grid}')
    
    plt.xlabel('Количество процессов')
    plt.ylabel('Время выполнения (с)')
    plt.title('Время выполнения в зависимости от количества процессов')
    plt.grid(True)
    plt.legend()

    # График эффективности
    plt.subplot(1, 2, 2)
    for grid in grid_sizes:
        data = df[df['Сетка'] == grid]
        plt.plot(data['Процессы'], data['Эффективность'], 
                marker='o', label=f'Сетка {grid}x{grid}')
    
    plt.xlabel('Количество процессов')
    plt.ylabel('Эффективность')
    plt.title('Эффективность в зависимости от количества процессов')
    plt.grid(True)
    plt.legend()

    plt.tight_layout()
    plt.savefig(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lab_main.png'))
    print("\nГрафики сохранены в lab_main.png")

if __name__ == "__main__":
    run_performance_test() 