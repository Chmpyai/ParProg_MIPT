cmake_minimum_required(VERSION 3.10)
project(Lab_1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Находим MPI
find_package(MPI REQUIRED)

# Список всех исполняемых файлов
set(EXECUTABLES
    task1_1
    task1_2
    task1_3_1
    task1_3_2
)

# Добавление исполняемых файлов
foreach(EXEC ${EXECUTABLES})
    add_executable(${EXEC} src/${EXEC}.cpp)
    target_link_libraries(${EXEC} MPI::MPI_CXX)
    if(MSVC)
        target_compile_options(${EXEC} PRIVATE /W4)
    else()
        target_compile_options(${EXEC} PRIVATE -Wall -Wextra)
    endif()
endforeach() 