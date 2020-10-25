cmake_minimum_required(VERSION 3.0.0)
project(gbc_cpp VERSION 0.1.0)

include(CTest)
enable_testing()

find_package(SDL2)
find_package(Boost)

string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)
message ([STATUS] "SDL2_INCLUDE_DIRS" ${SDL2_INCLUDE_DIRS})
message ([STATUS] "SDL2_LIBRARIES" ${SDL2_LIBRARIES})
message ([STATUS] "Boost_INCLUDE_DIRS" ${Boost_INCLUDE_DIRS})


add_executable(gbc_cpp main.c gpu.c gbz80.c apu.c mmu.c gb_apu/Gb_Apu.cpp gb_apu/GBAPU_Wrapper.cpp gb_apu/Multi_Buffer.cpp gb_apu/MB_Wrapper.cpp gb_apu/Blip_Buffer.cpp gb_apu/Gb_Oscs.cpp)
include_directories(gbc_cpp ${SDL2_INCLUDE_DIRS})
target_link_libraries(gbc_cpp ${SDL2_LIBRARIES})
include_directories(gbc_cpp ${Boost_INCLUDE_DIRS})


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
