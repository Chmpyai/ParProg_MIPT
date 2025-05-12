# --- Файл: run_lab2.py (Только измененные части и функция main) ---

import subprocess, os, io
from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd

# --- Конфигурация (остается прежней) ---
# ... (все переменные конфигурации как раньше) ...
LAB2_ROOT = Path(__file__).parent.parent.resolve()
BUILD_DIR = LAB2_ROOT / "build"
DATA_DIR = LAB2_ROOT / "data"
PLOTS_DIR = LAB2_ROOT / "plots"
RESULTS_FILES = {
    "sort": DATA_DIR / "sort_results.txt",
    "pipe": DATA_DIR / "pipe_results.txt",
    "shared_mem": DATA_DIR / "shared_mem_results.txt",
    "integral": DATA_DIR / "integral_results.txt"
}
SORT_SIZES = [100000, 500000, 1000000]

CPU_COUNT = os.cpu_count() or 4
COMMON_THREADS = sorted(list(set([1, 2, 4, CPU_COUNT])))
INTEGRAL_PARAMS = {"epsilon": "1e-8", "a": "0.01", "b": "2.0"}
integral_exe_name = "integral_pthread"

# --- Вспомогательные функции (остаются прежними) ---
def ensure_dir(path: Path): path.mkdir(parents=True, exist_ok=True)
def run_cmd(cmd_list, suppress_output_on_success=False, **kwargs): # Оставим вывод ошибок
    is_verbose = not suppress_output_on_success
    # if is_verbose: print(f"Executing: {' '.join(map(str, cmd_list))}") # Можно закомментировать для полной тишины
    try:
        return subprocess.run(cmd_list, capture_output=True, text=True, check=True, **kwargs)
    except subprocess.CalledProcessError as e:
        print(f"Error in '{' '.join(map(str,e.args[1]))}':\n{e.stderr or e.stdout or 'No output'}")
        raise
    except FileNotFoundError: print(f"Error: Command {cmd_list[0]} not found."); raise
def parse_dp(filepath, prefix, cols, names): # Оставляем как есть
    lines = [l.replace(prefix, "").strip() for l in (filepath.read_text().splitlines() if filepath.exists() else []) if l.startswith(prefix)]
    return pd.read_csv(io.StringIO("\n".join(lines)), sep=" ", header=None, names=names) if lines else pd.DataFrame(columns=names)

# --- Функции построения графиков (Изменяем plot_integral_balance) ---
def plot_sorts(): # Оставляем как есть (тихая версия)
    df = parse_dp(RESULTS_FILES["sort"], "DATAPOINT: ", 5, ["size", "threads", "qsort_ms", "stdsort_ms", "parallel_ms"])
    if df.empty: return
    max_thr = df['threads'].max() if not df.empty else COMMON_THREADS[-1]
    df_f_thr = df[df['threads'] == max_thr];
    if not df_f_thr.empty: df_f_thr.plot(x='size', y=['qsort_ms', 'stdsort_ms', 'parallel_ms'], kind='line', marker='o', logx=True, logy=True, title=f'Сортировка: время от размера (паралл. {max_thr} thr)', xlabel='Размер', ylabel='Время (мс)'); plt.grid(True, which="both", ls="-"); plt.tight_layout(); plt.savefig(PLOTS_DIR / "sort_vs_size.png"); plt.close()
    max_sz = df['size'].max() if not df.empty else SORT_SIZES[-1]
    df_f_sz = df[df['size'] == max_sz];
    if not df_f_sz.empty:
        ax = df_f_sz.plot(x='threads', y='parallel_ms', kind='line', marker='o', title=f'Сортировка: время от потоков (размер {max_sz})', xlabel='Потоки', ylabel='Время (мс)');
        if not df_f_sz['qsort_ms'].empty: ax.axhline(y=df_f_sz['qsort_ms'].iloc[0], color='r', ls='--', label='qsort')
        if not df_f_sz['stdsort_ms'].empty: ax.axhline(y=df_f_sz['stdsort_ms'].iloc[0], color='g', ls='--', label='std::sort')
        plt.xticks(sorted(df_f_sz['threads'].unique())); plt.legend(); plt.grid(True); plt.tight_layout(); plt.savefig(PLOTS_DIR / "sort_vs_threads.png"); plt.close()

def plot_integral_perf(): # Оставляем как есть (тихая версия)
    s_df = parse_dp(RESULTS_FILES["integral"], "DATAPOINT_INTEGRAL_SINGLE: ", 2, ["time_ms", "evals"])
    m_df = parse_dp(RESULTS_FILES["integral"], "DATAPOINT_INTEGRAL_MULTI: ", 3, ["threads", "time_ms", "evals"])
    if s_df.empty: return # Не можем посчитать ускорение без T1
    if m_df.empty and not s_df.empty: pass # Только одна точка будет
    elif m_df.empty and s_df.empty: return
    s_time = s_df['time_ms'].iloc[0]; df_list = [s_df.assign(threads=1)];
    if not m_df.empty: df_list.append(m_df)
    df = pd.concat(df_list).sort_values('threads').drop_duplicates('threads').reset_index(drop=True)
    if df.empty: return
    df['speedup'] = s_time / df['time_ms']; df['efficiency'] = df['speedup'] / df['threads']
    fig, axes = plt.subplots(1, 2, figsize=(12, 5)); df.plot(x='threads', y='speedup', marker='o', ax=axes[0], label='Ускорение'); axes[0].plot(df['threads'], df['threads'], ls='--', color='gray', label='Идеальное'); axes[0].set_title('Интегрирование: Ускорение'); axes[0].set_xlabel('Потоки'); axes[0].set_ylabel('Ускорение'); axes[0].legend(); axes[0].grid(True); df.plot(x='threads', y='efficiency', marker='s', ax=axes[1], label='Эффективность (%)', ylim=(0,1.1)); axes[1].axhline(y=1.0, ls='--', color='gray', label='Идеальная (100%)'); axes[1].set_title('Интегрирование: Эффективность'); axes[1].set_xlabel('Потоки'); axes[1].set_ylabel('Эффективность'); axes[1].legend(); axes[1].grid(True); axes[1].yaxis.set_major_formatter(plt.FuncFormatter(lambda y, _: '{:.0%}'.format(y)));
    for ax in axes: ax.set_xticks(sorted(df['threads'].unique()))
    plt.tight_layout(); plt.savefig(PLOTS_DIR / "integral_performance.png"); plt.close()

# --- НОВАЯ ВЕРСИЯ ФУНКЦИИ ДЛЯ ГРАФИКА БАЛАНСИРОВКИ ВРЕМЕНИ ---
def plot_thread_time_balance(cpp_stdout_log: str, num_expected_threads: int, plot_filename="integral_thread_time_balance.png"):
    """Парсит THREAD_TIME_MS из stdout и строит гистограмму времени."""
    thread_times_ms = []
    time_balance_summary_line = ""

    for line in cpp_stdout_log.splitlines():
        if line.startswith("THREAD_TIME_MS:"):
            parts = line.strip().split()
            if len(parts) == 3:
                try:
                    time_ms = float(parts[2]) # Парсим время как float
                    thread_times_ms.append(time_ms)
                except ValueError: pass # Молча игнорируем ошибки парсинга
        elif "Load Balancing (Time_ms):" in line: # Ищем новую строку со статистикой времени
            time_balance_summary_line = line.strip() # Сохраняем ее (но не выводим здесь)

    if not thread_times_ms or (num_expected_threads > 1 and len(thread_times_ms) != num_expected_threads):
        # print(f"Warning: Insufficient/mismatched THREAD_TIME_MS data for {num_expected_threads} thr for time balance plot.") # Молчим
        return
    if num_expected_threads <=1 and not thread_times_ms: return # Для 1 потока тоже выходим, если данных нет

    plt.figure(figsize=(8, 6))
    threads_ids = range(len(thread_times_ms))
    plt.bar(threads_ids, thread_times_ms, color='mediumseagreen', label='Время потока (мс)') # Новый цвет и метка
    plt.xlabel("ID Потока (условный)")
    plt.ylabel("Время выполнения (мс)") # Новая метка оси Y
    plt.title(f"Балансировка Времени ({len(thread_times_ms)} потоков)") # Новый заголовок
    if threads_ids: plt.xticks(threads_ids)

    if thread_times_ms: # Только если есть данные для расчета среднего
        mean_time_ms = sum(thread_times_ms) / len(thread_times_ms)
        plt.axhline(mean_time_ms, color='r', linestyle='dashed', linewidth=1, label=f'Среднее: {mean_time_ms:.1f} мс') # Новая метка для среднего

    plt.legend()
    plt.grid(axis='y', linestyle='--')
    plt.tight_layout()
    save_path = PLOTS_DIR / plot_filename
    plt.savefig(save_path)
    plt.close()
    # print(f"Thread time balance plot saved. {time_balance_summary_line}") # Убираем вывод

def plot_dynamic_step_indicator(evals_near_zero, evals_far_zero, plot_filename="integral_dynamic_step.png"): # Оставляем как есть (тихая версия)
    if evals_near_zero is None or evals_far_zero is None: return
    labels = ['Участок "Near Zero"', 'Участок "Far From Zero"']; eval_counts = [evals_near_zero, evals_far_zero]
    plt.figure(figsize=(8, 6)); bars = plt.bar(labels, eval_counts, color=['coral', 'lightsteelblue']); plt.ylabel("Общее количество вычислений функции"); plt.title("Индикатор динамического шага"); plt.grid(axis='y', linestyle='--');
    for bar in bars: yval = bar.get_height(); plt.text(bar.get_x()+bar.get_width()/2.0, yval + 0.01*max(eval_counts, default=1), f'{yval:.0f}', ha='center', va='bottom')
    plt.tight_layout(); plt.savefig(PLOTS_DIR / plot_filename); plt.close()

# --- Основная логика ---
def main():
    for d in [BUILD_DIR, DATA_DIR, PLOTS_DIR]: ensure_dir(d)
    for f in RESULTS_FILES.values():
        if f.exists(): f.unlink()

    print("--- Building C++ projects ---") # Оставляем вывод этапов сборки
    run_cmd(["cmake", "-S", str(LAB2_ROOT), "-B", str(BUILD_DIR)])
    run_cmd(["cmake", "--build", str(BUILD_DIR)])
    print("Build complete.")

    exe = lambda name: str(BUILD_DIR / name)
    integral_exe_path = exe(integral_exe_name)

    # Запуск бенчмарков (максимально тихий режим)
    with open(RESULTS_FILES["sort"], "a") as f:
        for sz in SORT_SIZES:
            for thr in COMMON_THREADS: f.write(run_cmd([exe("parallel_sort"), str(sz), str(thr)], suppress_output_on_success=True).stdout)
    for bench in ["pipe", "shared_mem"]:
        with open(RESULTS_FILES[bench], "w") as f: f.write(run_cmd([exe(f"{bench}_benchmark")], suppress_output_on_success=True).stdout)

    last_integral_run_stdout = ""
    with open(RESULTS_FILES["integral"], "a") as f:
        for thr in COMMON_THREADS:
            res = run_cmd([integral_exe_path, str(thr), INTEGRAL_PARAMS["epsilon"], INTEGRAL_PARAMS["a"], INTEGRAL_PARAMS["b"]], suppress_output_on_success=True)
            f.write(res.stdout)
            if thr == (COMMON_THREADS[-1] if COMMON_THREADS else 0) and thr > 1: last_integral_run_stdout = res.stdout

    # Построение основных графиков
    plot_sorts()
    plot_integral_perf()

    # Демонстрация свойств интеграла (только генерация графиков)
    if last_integral_run_stdout and COMMON_THREADS and COMMON_THREADS[-1] > 1:
        plot_thread_time_balance(last_integral_run_stdout, COMMON_THREADS[-1]) # Вызов обновленной функции

    evals_data_for_plot = {"near_zero": None, "far_zero": None}
    for item in [{"key": "near_zero", "a": 0.001, "b": 0.011}, {"key": "far_zero", "a": 1.0, "b": 1.010}]:
        try:
            res_stdout = run_cmd([integral_exe_path, "1", INTEGRAL_PARAMS["epsilon"], str(item["a"]), str(item["b"])], suppress_output_on_success=True).stdout
            eval_line = next((l for l in res_stdout.splitlines() if "Total function evaluations:" in l), None)
            if eval_line: evals_data_for_plot[item["key"]] = int(eval_line.split(":")[-1].strip())
        except Exception: pass
    plot_dynamic_step_indicator(evals_data_for_plot["near_zero"], evals_data_for_plot["far_zero"])

    # Финальное сообщение (можно тоже убрать, если нужна полная тишина)
    print(f"Processing complete. Plots saved to: {PLOTS_DIR.resolve()}")

if __name__ == "__main__": main()