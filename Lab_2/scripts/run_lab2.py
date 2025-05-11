import subprocess, os, io
from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd

# --- Конфигурация (оставляем как есть) ---
LAB2_ROOT = Path(__file__).parent.parent.resolve()
BUILD_DIR = LAB2_ROOT / "build"
DATA_DIR = LAB2_ROOT / "data"
PLOTS_DIR = LAB2_ROOT / "plots"
RESULTS_FILES = {
    "sort": DATA_DIR / "sort_results.txt", "pipe": DATA_DIR / "pipe_results.txt",
    "shared_mem": DATA_DIR / "shared_mem_results.txt", "integral": DATA_DIR / "integral_results.txt"
}
SORT_SIZES = [100000, 500000, 1000000]
CPU_COUNT = os.cpu_count() or 4
COMMON_THREADS = sorted(list(set([1, 2, 4, CPU_COUNT])))
INTEGRAL_PARAMS = {"epsilon": "1e-8", "a": "0.01", "b": "2.0"}
integral_exe_name = "integral_pthread"

# --- Вспомогательные функции ---
def ensure_dir(path: Path): path.mkdir(parents=True, exist_ok=True)

def run_cmd(cmd_list, suppress_output_on_success=False, **kwargs):

    # вывод ошибок
    is_verbose = not suppress_output_on_success
    if is_verbose: print(f"Executing: {' '.join(map(str, cmd_list))}")
    
    try:
        return subprocess.run(cmd_list, capture_output=True, text=True, check=True, **kwargs)
    except subprocess.CalledProcessError as e:
        print(f"Error in '{' '.join(map(str,e.args[1]))}':\n{e.stderr or e.stdout or 'No output'}")
        raise
    except FileNotFoundError:
        print(f"Error: Command {cmd_list[0]} not found.") # Ошибку поиска файла показываем
        raise

def parse_dp(filepath, prefix, cols, names):
    lines = [l.replace(prefix, "").strip() for l in (filepath.read_text().splitlines() if filepath.exists() else []) if l.startswith(prefix)]
    return pd.read_csv(io.StringIO("\n".join(lines)), sep=" ", header=None, names=names) if lines else pd.DataFrame(columns=names)

def plot_sorts():
    df = parse_dp(RESULTS_FILES["sort"], "DATAPOINT: ", 5, ["size", "threads", "qsort_ms", "stdsort_ms", "parallel_ms"])
    if df.empty: return
    
    max_thr = df['threads'].max() if not df.empty else COMMON_THREADS[-1]
    df_f_thr = df[df['threads'] == max_thr]
    if not df_f_thr.empty:
        df_f_thr.plot(x='size', y=['qsort_ms', 'stdsort_ms', 'parallel_ms'], kind='line', marker='o', logx=True, logy=True,
                      title=f'Сортировка: время от размера (паралл. {max_thr} thr)', xlabel='Размер', ylabel='Время (мс)')
        plt.grid(True, which="both", ls="-"); plt.tight_layout(); plt.savefig(PLOTS_DIR / "sort_vs_size.png"); plt.close()

    max_sz = df['size'].max() if not df.empty else SORT_SIZES[-1]
    df_f_sz = df[df['size'] == max_sz]
    if not df_f_sz.empty:
        ax = df_f_sz.plot(x='threads', y='parallel_ms', kind='line', marker='o', title=f'Сортировка: время от потоков (размер {max_sz})', xlabel='Потоки', ylabel='Время (мс)')
        if not df_f_sz['qsort_ms'].empty: ax.axhline(y=df_f_sz['qsort_ms'].iloc[0], color='r', ls='--', label='qsort')
        if not df_f_sz['stdsort_ms'].empty: ax.axhline(y=df_f_sz['stdsort_ms'].iloc[0], color='g', ls='--', label='std::sort')
        plt.xticks(sorted(df_f_sz['threads'].unique())); plt.legend(); plt.grid(True); plt.tight_layout(); plt.savefig(PLOTS_DIR / "sort_vs_threads.png"); plt.close()

def plot_integral_perf():
    s_df = parse_dp(RESULTS_FILES["integral"], "DATAPOINT_INTEGRAL_SINGLE: ", 2, ["time_ms", "evals"])
    m_df = parse_dp(RESULTS_FILES["integral"], "DATAPOINT_INTEGRAL_MULTI: ", 3, ["threads", "time_ms", "evals"])
    if s_df.empty: # Если нет данных для одного потока, не можем построить ускорение
        if not m_df.empty : print("Warning: Single-thread integral data missing, cannot plot speedup/efficiency.")
        return 
    if m_df.empty and not s_df.empty: # Если есть только однопоточные данные
         pass 
    elif m_df.empty and s_df.empty:
        return

    s_time = s_df['time_ms'].iloc[0]
    df_list = [s_df.assign(threads=1)]
    if not m_df.empty: df_list.append(m_df)
    df = pd.concat(df_list).sort_values('threads').drop_duplicates('threads').reset_index(drop=True)
    
    if df.empty: return

    df['speedup'] = s_time / df['time_ms']
    df['efficiency'] = df['speedup'] / df['threads']

    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    df.plot(x='threads', y='speedup', marker='o', ax=axes[0], label='Ускорение')
    axes[0].plot(df['threads'], df['threads'], ls='--', color='gray', label='Идеальное')
    axes[0].set_title('Интегрирование: Ускорение'); axes[0].set_xlabel('Потоки'); axes[0].set_ylabel('Ускорение'); axes[0].legend(); axes[0].grid(True)
    
    df.plot(x='threads', y='efficiency', marker='s', ax=axes[1], label='Эффективность (%)', ylim=(0,1.1))
    axes[1].axhline(y=1.0, ls='--', color='gray', label='Идеальная (100%)')
    axes[1].set_title('Интегрирование: Эффективность'); axes[1].set_xlabel('Потоки'); axes[1].set_ylabel('Эффективность'); axes[1].legend(); axes[1].grid(True)
    axes[1].yaxis.set_major_formatter(plt.FuncFormatter(lambda y, _: '{:.0%}'.format(y)))
    
    for ax in axes: ax.set_xticks(sorted(df['threads'].unique()))
    plt.tight_layout(); plt.savefig(PLOTS_DIR / "integral_performance.png"); plt.close()

def plot_integral_balance(cpp_stdout, num_thr, fname="integral_thread_balance.png"):
    evals = [int(l.split()[-1]) for l in cpp_stdout.splitlines() if l.startswith("THREAD_EVALS:")]
    if not evals or (num_thr > 1 and len(evals) != num_thr): # Проверка для многопоточного случая

        return
    if num_thr <=1 and not evals: 
        return


    plt.figure(figsize=(8, 6))
    plt.bar(range(len(evals)), evals, color='skyblue', label='Вычисления/поток')
    if evals and len(evals) > 0 : plt.axhline(sum(evals)/len(evals), color='r', ls='--', label=f'Среднее: {sum(evals)/len(evals):.0f}')
    plt.xlabel("ID Потока (условный)"); plt.ylabel("Вычисления"); plt.title(f"Балансировка ({len(evals)} потоков)")
    if evals: plt.xticks(range(len(evals)))
    plt.legend(); plt.grid(axis='y', ls='--'); plt.tight_layout(); plt.savefig(PLOTS_DIR / fname); plt.close()


def plot_dynamic_step_indicator(evals_near_zero, evals_far_zero, plot_filename="integral_dynamic_step.png"):
    if evals_near_zero is None or evals_far_zero is None: return # Молча выходим

    labels = ['Участок "Near Zero"', 'Участок "Far From Zero"']
    eval_counts = [evals_near_zero, evals_far_zero]
    plt.figure(figsize=(8, 6))
    bars = plt.bar(labels, eval_counts, color=['coral', 'lightsteelblue'])
    plt.ylabel("Общее количество вычислений функции")
    plt.title("Индикатор динамического шага")
    plt.grid(axis='y', linestyle='--')
    for bar in bars:
        yval = bar.get_height()
        plt.text(bar.get_x()+bar.get_width()/2.0, yval + 0.01*max(eval_counts, default=1), f'{yval:.0f}', ha='center', va='bottom')
    plt.tight_layout(); plt.savefig(PLOTS_DIR / plot_filename); plt.close()


# --- Основная логика ---
def main():
    for d in [BUILD_DIR, DATA_DIR, PLOTS_DIR]: ensure_dir(d)
    for f in RESULTS_FILES.values(): 
        if f.exists(): f.unlink()

    print("--- Building C++ projects ---")
    run_cmd(["cmake", "-S", str(LAB2_ROOT), "-B", str(BUILD_DIR)])
    run_cmd(["cmake", "--build", str(BUILD_DIR)])
    print("Build complete.")


    exe = lambda name: str(BUILD_DIR / name)
    integral_exe_path = exe(integral_exe_name) # Используем глобальную integral_exe_name

    # Убираем заголовки секций для бенчмарков
    with open(RESULTS_FILES["sort"], "a") as f:
        for sz in SORT_SIZES:
            for thr in COMMON_THREADS: f.write(run_cmd([exe("parallel_sort"), str(sz), str(thr)], suppress_output_on_success=True).stdout)
    
    for bench, name in [("pipe", "Pipe"), ("shared_mem", "Shared Mem")]:
        with open(RESULTS_FILES[bench], "w") as f: f.write(run_cmd([exe(f"{bench}_benchmark")], suppress_output_on_success=True).stdout)

    last_integral_run_stdout = ""
    with open(RESULTS_FILES["integral"], "a") as f:
        for thr in COMMON_THREADS:
            # print(f"Integrating with {thr} thread(s)...") # Убрано
            res = run_cmd([integral_exe_path, str(thr), INTEGRAL_PARAMS["epsilon"], INTEGRAL_PARAMS["a"], INTEGRAL_PARAMS["b"]], suppress_output_on_success=True)
            f.write(res.stdout)
            if thr == (COMMON_THREADS[-1] if COMMON_THREADS else 0) and thr > 1: last_integral_run_stdout = res.stdout

    plot_sorts(); plot_integral_perf()
    
    if last_integral_run_stdout and COMMON_THREADS and COMMON_THREADS[-1] > 1:
        plot_integral_balance(last_integral_run_stdout, COMMON_THREADS[-1])
    
    evals_data_for_plot = {"near_zero": None, "far_zero": None}
    intervals_to_test = [
        {"key": "near_zero", "a": 0.001, "b": 0.011}, {"key": "far_zero", "a": 1.0, "b": 1.010}
    ]
    for item in intervals_to_test:
        try:
            res_stdout = run_cmd([integral_exe_path, "1", INTEGRAL_PARAMS["epsilon"], str(item["a"]), str(item["b"])], suppress_output_on_success=True).stdout
            eval_line = next((l for l in res_stdout.splitlines() if "Total function evaluations:" in l), None)
            if eval_line: evals_data_for_plot[item["key"]] = int(eval_line.split(":")[-1].strip())
        except Exception: pass # Молча пропускаем ошибки парсинга/запуска для этих демо-запусков
    
    plot_dynamic_step_indicator(evals_data_for_plot["near_zero"], evals_data_for_plot["far_zero"])

if __name__ == "__main__": main()