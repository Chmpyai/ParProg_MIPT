# lab1_1.py
import os
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import re

def run_pi_test():
    # Пути
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    build_dir = os.path.join(base_dir, "build")
    results_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "lab1_1.csv")

    # Сборка проекта
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    
    # Переходим в директорию сборки
    os.chdir(build_dir)
    
    # Собираем проект
    subprocess.run(["cmake", ".."], check=True)
    subprocess.run(["cmake", "--build", ".", "--config", "Release"], check=True)

    # Параметры тестирования
    num_processes = [1, 2, 4, 8]
    num_points = [1000000, 4000000, 16000000]
    
    results = []
    
    for points in num_points:
        for procs in num_processes:
            print(f"\nЗапуск: {procs} процессов, {points} точек")
            
            # Запускаем программу
            result = subprocess.run(["mpiexec", "-n", str(procs), "./task1_1.exe", str(points)],
                                  capture_output=True, text=True, encoding='utf-8', check=True)
            
            # Читаем результат из файла
            res_df = pd.read_csv("pi_result.csv")
            pi_value = float(res_df["π"].iloc[0])
            time_value = float(res_df["Время(с)"].iloc[0])
            
            results.append({
                'Процессы': procs,
                'Точки': points,
                'π': pi_value,
                'Время (с)': time_value
            })
            
            print(f"π = {pi_value}")
            print(f"Время = {time_value} с")

    # Сохранение результатов
    df = pd.DataFrame(results)
    df.to_csv(results_file, index=False)
    print(f"\nРезультаты сохранены в {results_file}")

    # Отображение таблицы результатов
    print("\nТаблица результатов:")
    pd.set_option('display.float_format', lambda x: '%.5f' % x)  # Форматирование чисел
    pd.set_option('display.max_rows', None)  # Показывать все строки
    pd.set_option('display.max_columns', None)  # Показывать все колонки
    pd.set_option('display.width', None)  # Автоматическая ширина
    print(df.to_string(index=False))

    # Визуализация
    plt.figure(figsize=(15, 5))

    # График времени выполнения
    plt.subplot(1, 2, 1)
    for points in num_points:
        data = df[df['Точки'] == points]
        plt.plot(data['Процессы'], data['Время (с)'], 
                marker='o', label=f'{points:,} точек')
    
    plt.xlabel('Количество процессов')
    plt.ylabel('Время выполнения (с)')
    plt.title('Время выполнения в зависимости от количества процессов')
    plt.grid(True)
    plt.legend()

    # График точности вычисления π
    plt.subplot(1, 2, 2)
    for points in num_points:
        data = df[df['Точки'] == points]
        error = abs(data['π'] - np.pi)
        plt.plot(data['Процессы'], error, 
                marker='o', label=f'{points:,} точек')
    
    plt.xlabel('Количество процессов')
    plt.ylabel('Ошибка вычисления π')
    plt.title('Точность вычисления π')
    plt.grid(True)
    plt.legend()

    plt.tight_layout()
    plt.savefig(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lab1_1.png'))
    print("\nГрафики сохранены в lab1_1.png")

if __name__ == "__main__":
    run_pi_test()