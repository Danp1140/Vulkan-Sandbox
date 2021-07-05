cmake_minimum_required(VERSION 3.17)
project(VulkanSandbox)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo")

include_directories(include)
include_directories(include/vulkan)
link_directories(libs)
link_directories(libs/vulkan)

#add_custom_target(ShaderCompilation ALL
#        DEPENDS compilelog.txt)
#add_custom_command(OUTPUT compilelog.txt
#        COMMAND compile.sh)

add_executable(VulkanSandbox src/main.cpp src/GraphicsHandler.cpp src/GraphicsHandler.h src/Mesh.cpp src/Mesh.h src/Camera.cpp src/Camera.h src/Light.cpp src/Light.h src/Grass.cpp src/Grass.h src/Text.cpp src/Text.h)

target_link_libraries(${PROJECT_NAME} vulkan.1.2.176 glfw.3 -m64)
#target_link_libraries(${PROJECT_NAME} vulkan.1.2.176 glfw3 freetype bz2 z -m64)