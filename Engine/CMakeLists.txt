cmake_minimum_required(VERSION 3.10)
project(Engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

# Vérifications de sécurité
if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Veuillez choisir un répertoire de build séparé (par ex : build/)")
endif()
if(CMAKE_SOURCE_DIR MATCHES " ")
    message(WARNING "Votre répertoire source contient des espaces. Cela peut poser problème lors de la compilation.")
endif()
if(CMAKE_BINARY_DIR MATCHES " ")
    message(WARNING "Votre répertoire de build contient des espaces. Cela peut poser problème lors de la compilation.")
endif()

include(FetchContent)

# --- Lua ---
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.6
)
FetchContent_MakeAvailable(lua)

# --- Dépendances externes ---
add_subdirectory(external)

include_directories(
    external/glfw-3.1.2/include/
    external/glm-0.9.7.1/
    external/glew-1.13.0/include/
    ${lua_SOURCE_DIR}
    .
)

# Liste des bibliothèques à lier
set(ALL_LIBS
    ${OPENGL_LIBRARY}
    glfw
    GLEW_1130
)

# Définitions globales
add_definitions(
    -DTW_STATIC
    -DTW_NO_LIB_PRAGMA
    -DTW_NO_DIRECT3D
    -DGLEW_STATIC
    -D_CRT_SECURE_NO_WARNINGS
)

# Compiler Lua en tant que librairie statique (tous les fichiers sources nécessaires)
add_library(lua_static STATIC
    ${lua_SOURCE_DIR}/lapi.c
    ${lua_SOURCE_DIR}/lauxlib.c
    ${lua_SOURCE_DIR}/lbaselib.c
    ${lua_SOURCE_DIR}/lcode.c
    ${lua_SOURCE_DIR}/lcorolib.c
    ${lua_SOURCE_DIR}/lctype.c
    ${lua_SOURCE_DIR}/ldblib.c
    ${lua_SOURCE_DIR}/ldebug.c
    ${lua_SOURCE_DIR}/ldo.c
    ${lua_SOURCE_DIR}/ldump.c
    ${lua_SOURCE_DIR}/lfunc.c
    ${lua_SOURCE_DIR}/lgc.c
    ${lua_SOURCE_DIR}/linit.c
    ${lua_SOURCE_DIR}/liolib.c
    ${lua_SOURCE_DIR}/llex.c
    ${lua_SOURCE_DIR}/lmathlib.c
    ${lua_SOURCE_DIR}/lmem.c
    ${lua_SOURCE_DIR}/loadlib.c
    ${lua_SOURCE_DIR}/lobject.c
    ${lua_SOURCE_DIR}/lopcodes.c
    ${lua_SOURCE_DIR}/loslib.c
    ${lua_SOURCE_DIR}/lparser.c
    ${lua_SOURCE_DIR}/lstate.c
    ${lua_SOURCE_DIR}/lstring.c
    ${lua_SOURCE_DIR}/lstrlib.c
    ${lua_SOURCE_DIR}/ltable.c
    ${lua_SOURCE_DIR}/ltablib.c
    ${lua_SOURCE_DIR}/ltm.c
    ${lua_SOURCE_DIR}/lundump.c
    ${lua_SOURCE_DIR}/lutf8lib.c
    ${lua_SOURCE_DIR}/lvm.c
    ${lua_SOURCE_DIR}/lzio.c
)

# Ajouter les flags de compilation nécessaires pour Lua
target_compile_definitions(lua_static PRIVATE LUA_USE_LINUX)
target_include_directories(lua_static PUBLIC ${lua_SOURCE_DIR})

# Lier avec les bibliothèques système nécessaires pour Lua (libm et libdl sur Linux)
target_link_libraries(lua_static PUBLIC m dl)

# === Cible principale ===
add_executable(Engine
    main.cpp
    common/shader.cpp
    common/shader.hpp
    common/controls.cpp
    common/controls.hpp
    common/texture.cpp
    common/texture.hpp
    common/objloader.cpp
    common/objloader.hpp
    common/vboindexer.cpp
    common/vboindexer.hpp
    common/stb_image.h
    common/Mesh.h
    common/Mesh.cpp
    Shaders/vertex_shader.glsl
    Shaders/fragment_shader.glsl
    Shaders/vertex.glsl
    Shaders/fragment.glsl
    Shaders/computeShader.glsl
)

target_link_libraries(Engine ${ALL_LIBS} lua_static)

# Groupes pour IDE
source_group(common REGULAR_EXPRESSION ".*/common/.*")
source_group(shaders REGULAR_EXPRESSION ".*/Shaders/.*")

# Post-build : copie dans le dossier source (facultatif)
add_custom_command(
    TARGET Engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "$<TARGET_FILE:Engine>"
        "${CMAKE_SOURCE_DIR}/Engine${CMAKE_EXECUTABLE_SUFFIX}"
)