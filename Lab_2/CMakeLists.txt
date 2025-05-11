cmake_minimum_required(VERSION 3.10)
project(Lab_2_ParallelComputing CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic") # Рекомендуемые флаги

# Включение pthreads
find_package(Threads REQUIRED)

# Исполняемые файлы
add_executable(parallel_sort src/parallel_sort.cpp)
target_link_libraries(parallel_sort PRIVATE Threads::Threads)

add_executable(pipe_benchmark src/pipe_benchmark.cpp)
# pipe_benchmark_s обычно не требует дополнительных библиотек кроме стандартных

add_executable(shared_mem_benchmark src/shared_mem_threads_benchmark.cpp)
target_link_libraries(shared_mem_benchmark PRIVATE Threads::Threads)

add_executable(integral_pthread src/integral_pthread.cpp)
# Для std::atomic на C++17 обычно явное связывание не требуется,
# но если возникнут проблемы, можно добавить -latomic в CMAKE_EXE_LINKER_FLAGS
# или попробовать target_link_libraries(... std::atomic) если компилятор поддерживает.
# Threads::Threads уже должен покрывать необходимое для pthreads.
target_link_libraries(integral_pthread PRIVATE Threads::Threads)

# Опционально: если хотите, чтобы исполняемые файлы были в Lab_2/bin/
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
# По умолчанию они будут в Lab_2/build/ (или подкаталогах типа Lab_2/build/Debug)