cmake_minimum_required(VERSION 3.24)
project(dataloader CXX CUDA)
set(CMAKE_CXX_STANDARD 20)

include_directories(include)
add_executable(
    ${PROJECT_NAME}
    src/main.cu
)