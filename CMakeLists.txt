cmake_minimum_required(VERSION 3.17)
project(VulkanSandbox)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -std=c++20 -Wno-deprecated")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Metal -framework Foundation -framework QuartzCore -framework IOKit -framework IOSurface -framework Cocoa")

include_directories(include)
include_directories(include/vulkan)
link_directories(libs)
link_directories(libs/vulkan)

add_executable(VulkanSandbox src/main.cpp src/GraphicsHandler.cpp src/GraphicsHandler.h src/Mesh.cpp src/Mesh.h src/Camera.cpp src/Camera.h src/Light.cpp src/Light.h src/Text.cpp src/Text.h src/PhysicsHandler.cpp src/PhysicsHandler.h src/WanglingEngine.cpp src/WanglingEngine.h src/TextureHandler.cpp src/TextureHandler.h src/Ocean.cpp src/Ocean.h src/ParticleSystem.cpp src/ParticleSystem.h src/Terrain.cpp src/Terrain.h)

#target_link_libraries(${PROJECT_NAME} vulkan.1.2.176 glfw.3 -m64)
target_link_libraries(${PROJECT_NAME} vulkan.1.3.216 glfw3 freetype bz2 z -m64)
