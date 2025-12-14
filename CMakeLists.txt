cmake_minimum_required(VERSION 3.10)
project(TrabalhoGC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(assimp REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${OPENGL_INCLUDE_DIRS}
    ${ASSIMP_INCLUDE_DIRS}
)

# Source files
file(GLOB SOURCES 
    "src/*.cpp"
    "src/glad.c"
)

# Executable
add_executable(trabalho_gc ${SOURCES})

# Link libraries
target_link_libraries(trabalho_gc 
    glfw
    ${OPENGL_LIBRARIES}
    ${ASSIMP_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

# Copy shaders to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders 
     DESTINATION ${CMAKE_BINARY_DIR})

# Copy models to build directory (if any)
file(COPY ${CMAKE_SOURCE_DIR}/models 
     DESTINATION ${CMAKE_BINARY_DIR})
