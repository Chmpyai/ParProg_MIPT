# Лабораторная работа по параллельному программированию

## Требования

### Системные требования
- Windows 10 или выше
- Visual Studio 2019 или выше с поддержкой C++
- CMake 3.10 или выше
- MS-MPI (Microsoft MPI)
- Python 3.8 или выше

### Установка MS-MPI
1. Скачайте и установите MS-MPI:
   - [msmpisetup.exe](https://download.microsoft.com/download/a/5/2/a5207ca5-1203-491a-8fb8-906fd68ae623/msmpisetup.exe)
   - [msmpisdk.msi](https://download.microsoft.com/download/a/5/2/a5207ca5-1203-491a-8fb8-906fd68ae623/msmpisdk.msi)

2. Добавьте путь к MS-MPI в переменную PATH:
   ```
   C:\Program Files (x86)\Microsoft SDKs\MPI
   ```

## Установка и запуск

### 1. Клонирование репозитория
```bash
git clone https://github.com/your-username/ParProg_cursor.git
cd ParProg_cursor
```

### 2. Настройка Python окружения
```bash
# Создание виртуального окружения
python -m venv .venv

# Активация виртуального окружения
# Для Windows:
.venv\Scripts\activate

# Установка зависимостей
pip install -r Lab_1/requirements.txt
```

### 3. Сборка проекта
```bash
cd Lab_1
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### 4. Запуск тестов

#### Тест вычисления π
```bash
# Из виртуального окружения
python scripts/run_pi_test.py
```

#### Тест времени коммуникации
```bash
# Из виртуального окружения
python scripts/run_comm_test.py
```

#### Тест уравнения переноса
```bash
# Из виртуального окружения
python scripts/analyze_transport.py
```

## Структура проекта

```
Lab_1/
├── src/                    # Исходный код C++
│   ├── task1_1.cpp        # Вычисление π
│   ├── task1_2.cpp        # Измерение времени коммуникации
│   ├── task1_3_1.cpp      # Последовательное решение уравнения переноса
│   └── task1_3_2.cpp      # Параллельное решение уравнения переноса
├── scripts/               # Python скрипты для тестирования
│   ├── run_pi_test.py
│   ├── run_comm_test.py
│   └── analyze_transport.py
├── build/                # Директория сборки
├── CMakeLists.txt       # Файл конфигурации CMake
└── requirements.txt     # Зависимости Python
```

## Результаты

Результаты тестов сохраняются в следующие файлы:
- `scripts/pi_results.csv` - результаты вычисления π
- `scripts/comm_time_results.csv` - результаты измерения времени коммуникации
- `scripts/transport_results.csv` - результаты решения уравнения переноса

Графики сохраняются в:
- `scripts/pi_results.png`
- `scripts/communication_time.png`
- `scripts/transport_performance.png`

## Примечания

- Все Python скрипты должны запускаться из активированного виртуального окружения
- Для корректной работы с русскими символами в консоли используется кодировка UTF-8
- Результаты тестов сохраняются в формате CSV для дальнейшего анализа 