import os
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def run_pi_test():
    # Пути
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    build_dir = os.path.join(base_dir, 'Lab_1', 'build')
    results_file = os.path.join(base_dir, 'scripts', 'pi_results.csv')
    
    # Создание и сборка проекта
    print("Сборка проекта...")
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    
    os.chdir(build_dir)
    subprocess.run(['cmake', '..'], check=True)
    subprocess.run(['cmake', '--build', '.', '--config', 'Release'], check=True)
    
    # Параметры тестирования
    num_processes = [1, 2, 4, 8]
    num_points = [1000000, 4000000, 16000000]
    
    # Создаем DataFrame для результатов
    results = []
    
    # Запуск тестов
    print("\nЗапуск тестов вычисления π...")
    for n_proc in num_processes:
        for n_points in num_points:
            print(f"\nТест: {n_proc} процессов, {n_points} точек")
            
            # Запуск MPI программы
            result = subprocess.run(
                ['mpiexec', '-n', str(n_proc), 'mpi_pi.exe', str(n_points)],
                capture_output=True,
                text=True,
                check=True
            )
            
            # Парсинг вывода
            output = result.stdout
            pi_value = float(output.split('π: ')[1].split('\n')[0])
            time_taken = float(output.split('время: ')[1].split(' ')[0])
            
            results.append({
                'Процессы': n_proc,
                'Точки': n_points,
                'Значение π': pi_value,
                'Время (с)': time_taken
            })
    
    # Сохранение результатов
    df = pd.DataFrame(results)
    df.to_csv(results_file, index=False)
    print("\nРезультаты измерений:")
    print(df.to_string(index=False))
    
    # Визуализация
    print("\nСоздание графиков...")
    
    # График зависимости времени от числа процессов
    plt.figure(figsize=(10, 6))
    for points in num_points:
        data = df[df['Точки'] == points]
        plt.plot(data['Процессы'], data['Время (с)'], 'o-', 
                label=f'{points:,} точек')
    
    plt.xlabel('Количество процессов')
    plt.ylabel('Время выполнения (с)')
    plt.title('Зависимость времени выполнения от числа процессов')
    plt.grid(True)
    plt.legend()
    plt.savefig(os.path.join(base_dir, 'scripts', 'pi_time.png'))
    plt.close()
    
    # График зависимости точности от числа процессов
    plt.figure(figsize=(10, 6))
    for points in num_points:
        data = df[df['Точки'] == points]
        error = abs(data['Значение π'] - np.pi)
        plt.plot(data['Процессы'], error, 'o-', 
                label=f'{points:,} точек')
    
    plt.xlabel('Количество процессов')
    plt.ylabel('Погрешность')
    plt.title('Зависимость точности от числа процессов')
    plt.grid(True)
    plt.legend()
    plt.savefig(os.path.join(base_dir, 'scripts', 'pi_accuracy.png'))
    plt.close()
    
    print("\nГрафики сохранены в папке scripts:")
    print("- pi_time.png: зависимость времени от числа процессов")
    print("- pi_accuracy.png: зависимость точности от числа процессов")

if __name__ == '__main__':
    try:
        run_pi_test()
    except subprocess.CalledProcessError as e:
        print(f"Ошибка при выполнении команды: {e}")
    except Exception as e:
        print(f"Произошла ошибка: {e}") 