import os
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def run_comm_test():
    # Пути
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    build_dir = os.path.join(base_dir, "build")
    results_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "lab1_2.csv")

    # Сборка проекта
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    os.chdir(build_dir)
    subprocess.run(["cmake", ".."], check=True)
    subprocess.run(["cmake", "--build", ".", "--config", "Release"], check=True)

    # Запуск MPI программы
    print("\nЗапуск теста коммуникации...")
    with open(results_file, 'w') as f:
        subprocess.run(["mpiexec", "-n", "2", "task1_2.exe"], 
                      stdout=f, 
                      text=True, 
                      encoding='utf-8', 
                      check=True)

    # Чтение и вывод результатов
    print("\nРезультаты измерений:")
    data = pd.read_csv(results_file)
    print(data.to_string(index=False))

    # Визуализация
    plt.figure(figsize=(10, 6))
    plt.plot(data['Размер (байт)'], data['Время (мкс)'], 'bo-', label='Измеренное время')
    
    # Теоретическая зависимость
    x = np.array(data['Размер (байт)'])
    y = np.array(data['Время (мкс)'])
    z = np.polyfit(x, y, 1)
    p = np.poly1d(z)
    plt.plot(x, p(x), 'r--', label=f'Теоретическая зависимость (y={z[0]:.2f}x+1)')
    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel('Размер сообщения (байт)')
    plt.ylabel('Время коммуникации (мкс)')
    plt.title('Зависимость времени коммуникации от размера сообщения')
    plt.grid(True)
    plt.legend()
    
    # Сохранение графика
    plot_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lab1_2.png')
    plt.savefig(plot_file)
    plt.close()
    
    print(f"\nГрафик сохранен в: {plot_file}")
    print("\nТеоретическая зависимость:")
    print(f"y = {z[0]:.2f}x + {z[1]:.2f}")
    print(f"где x - размер сообщения в байтах")
    print(f"y - время коммуникации в микросекундах")

if __name__ == "__main__":
    run_comm_test() 