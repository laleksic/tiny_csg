project(tiny_csg)

# find_package(glm REQUIRED)
find_package(SDL2 REQUIRED) # for demo only
find_package(GLEW REQUIRED) # for demo only

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(csg
    csg.cpp
    csg.hpp
    csg_private.hpp
    query_point.cpp
    query_box.cpp
    query_ray.cpp
    query_frustum.cpp
    rebuild.cpp
)
target_include_directories(csg PUBLIC 3rdp/glm)
# target_link_libraries(csg PUBLIC glm)
target_compile_options(csg PRIVATE -Wall -Wextra -Wpedantic)

add_executable(demo
    demo.cpp
    demo_flythrough_camera.cpp
    3rdp/shader_loader/src/shader.cpp
    3rdp/shader_loader/src/shader_program.cpp        
    3rdp/randomColor-cpp/randomcolor.cpp
)
if (WIN32)
    target_link_libraries(demo PRIVATE csg SDL2::SDL2 SDL2::SDL2main GLEW::GLEW)
endif ()
if (UNIX)
    target_link_libraries(demo PRIVATE csg SDL2::SDL2 SDL2::SDL2main GLEW::GLEW GL)
endif ()
target_include_directories(demo PRIVATE 3rdp/flythrough_camera)
target_include_directories(demo PRIVATE 3rdp/shader_loader/include)
target_include_directories(demo PRIVATE 3rdp/randomColor-cpp)
target_include_directories(demo PRIVATE 3rdp/defer)
target_compile_options(demo PRIVATE -Wall -Wextra -Wpedantic)
