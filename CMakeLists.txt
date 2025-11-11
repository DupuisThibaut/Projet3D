cmake_minimum_required(VERSION 3.14)
project(projet3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Détecte tous les fichiers .cpp et .c dans source/
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    source/*.cpp
    source/*.c
    include/*.cpp
    include/*.c
    main.cpp
)

# Crée la lib GLAD
add_library(GLAD STATIC "source/glad.c")
target_include_directories(GLAD PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Exécutable final
add_executable(${PROJECT_NAME} ${SOURCES})

# Inclure les headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Libs pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(FREETYPE2 REQUIRED freetype2)
pkg_check_modules(ASSIMP REQUIRED assimp)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${GLFW_INCLUDE_DIRS}
    ${FREETYPE2_INCLUDE_DIRS}
    ${ASSIMP_INCLUDE_DIRS}
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${GLFW_LIBRARY_DIRS}
    ${FREETYPE2_LIBRARY_DIRS}
    ${ASSIMP_LIBRARY_DIRS}
)

# Link final
target_link_libraries(${PROJECT_NAME}
    GLAD
    ${GLFW_LIBRARIES}
    ${FREETYPE2_LIBRARIES}
    ${ASSIMP_LIBRARIES}
    GL
    GLU
    glut
    dl
    pthread
    m
)
