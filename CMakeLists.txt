cmake_minimum_required(VERSION 3.22)
project(PigletASM)

set(CMAKE_CXX_STANDARD 20)

add_executable(PigletASM asm.cpp)
add_executable(PigletVM vm.cpp)
add_executable(cpp-tutorial tutorial.cpp)
add_executable(experiment experiment.cpp)

set_property(TARGET PigletVM PROPERTY COMPILE_OPTIONS "-ljitplus -ljit")
set_property(TARGET PigletVM PROPERTY INTERFACE_COMPILE_OPTIONS "-ljitplus -ljit")
target_link_libraries(PigletVM "-ljitplus -ljit")

set_property(TARGET cpp-tutorial PROPERTY COMPILE_OPTIONS "-ljitplus -ljit")
set_property(TARGET cpp-tutorial PROPERTY INTERFACE_COMPILE_OPTIONS "-ljitplus -ljit")
target_link_libraries(cpp-tutorial "-ljitplus -ljit")