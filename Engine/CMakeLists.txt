cmake_minimum_required(VERSION 3.10)
project(Engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

# ════════════════════════════════════════════════════════════════
#  DÉTECTION SIMD
# ════════════════════════════════════════════════════════════════

include(CheckCXXCompilerFlag)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    CHECK_CXX_COMPILER_FLAG("-mavx2" COMPILER_SUPPORTS_AVX2)
    CHECK_CXX_COMPILER_FLAG("-mavx" COMPILER_SUPPORTS_AVX)
    CHECK_CXX_COMPILER_FLAG("-msse4.1" COMPILER_SUPPORTS_SSE41)
    
    if(COMPILER_SUPPORTS_AVX2)
        set(SIMD_FLAGS "-mavx2 -mfma")
        add_compile_definitions(GLM_FORCE_SIMD_AVX2)
        message(STATUS "✅ SIMD: AVX2 + FMA")
    elseif(COMPILER_SUPPORTS_AVX)
        set(SIMD_FLAGS "-mavx")
        add_compile_definitions(GLM_FORCE_SIMD_AVX)
        message(STATUS "✅ SIMD: AVX")
    elseif(COMPILER_SUPPORTS_SSE41)
        set(SIMD_FLAGS "-msse4.1")
        add_compile_definitions(GLM_FORCE_SIMD_SSE4)
        message(STATUS "✅ SIMD: SSE4.1")
    endif()
    
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM|aarch64|AARCH64")
    # ✅ Apple Silicon détecte automatiquement NEON sans flag explicite
    if(APPLE)
        set(SIMD_FLAGS "")  # Pas besoin de -march sur macOS ARM
        add_compile_definitions(GLM_FORCE_NEON)
        message(STATUS "✅ SIMD: ARM NEON (Apple Silicon - auto)")
    else()
        CHECK_CXX_COMPILER_FLAG("-march=armv8-a+simd" COMPILER_SUPPORTS_NEON)
        if(COMPILER_SUPPORTS_NEON)
            set(SIMD_FLAGS "-march=armv8-a+simd")
            add_compile_definitions(GLM_FORCE_NEON)
            message(STATUS "✅ SIMD: ARM NEON (Linux ARM)")
        endif()
    endif()
endif()

if(DEFINED SIMD_FLAGS)
    add_compile_options(${SIMD_FLAGS})
endif()

# ════════════════════════════════════════════════════════════════
#  OPTIMISATIONS DE COMPILATION
# ════════════════════════════════════════════════════════════════

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "🚀 Release mode: optimizations enabled")
    if(NOT MSVC)
        add_compile_options(
            -O3
            -ffast-math
            -march=native
            -mtune=native
            -funroll-loops
        )
    endif()
endif()

# ════════════════════════════════════════════════════════════════
#  GLM OPTIMIZATIONS
# ════════════════════════════════════════════════════════════════

add_compile_definitions(
    GLM_FORCE_INLINE
    GLM_FORCE_INTRINSICS
    GLM_FORCE_ALIGNED_GENTYPES
    GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
)
# Vérifications de sécurité
if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Veuillez choisir un répertoire de build séparé (par ex : build/)")
endif()
if(CMAKE_SOURCE_DIR MATCHES " ")
    message(WARNING "Votre répertoire source contient des espaces. Cela peut poser problème lors de la compilation.")
endif()

include(FetchContent)

# --- Lua ---
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.6
)
FetchContent_MakeAvailable(lua)

# --- IMGUI ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.9
)
FetchContent_MakeAvailable(imgui)
# ImGui n'a pas de CMakeLists.txt, donc on crée une bibliothèque manuellement
set(IMGUI_DIR ${imgui_SOURCE_DIR})

add_library(imgui STATIC
    # Core ImGui
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    
    # Backends GLFW + OpenGL3
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# ✅ Force l'utilisation de GLFW 3.1.2 locale
target_include_directories(imgui PRIVATE
    ${CMAKE_SOURCE_DIR}/external/glfw-3.1.2/include
)

target_link_libraries(imgui PUBLIC
    glfw
    ${OPENGL_LIBRARY}
)

# ✅ AJOUT : Définitions pour ImGui + macOS
target_compile_definitions(imgui PUBLIC
    IMGUI_IMPL_OPENGL_LOADER_GLEW
)

# ✅ Définit la plateforme automatiquement
if(APPLE)
    target_compile_definitions(imgui PRIVATE
        GLFW_EXPOSE_NATIVE_COCOA
        GLFW_EXPOSE_NATIVE_NSGL
    )
elseif(WIN32)
    target_compile_definitions(imgui PRIVATE
        GLFW_EXPOSE_NATIVE_WIN32
        GLFW_EXPOSE_NATIVE_WGL
    )
elseif(UNIX)
    target_compile_definitions(imgui PRIVATE
        GLFW_EXPOSE_NATIVE_X11
        GLFW_EXPOSE_NATIVE_GLX
    )
endif()

message(STATUS "✅ ImGui will be downloaded and built automatically")

# --- Dépendances externes ---
add_subdirectory(external)

include_directories(
    external/glfw-3.1.2/include/
    external/glm-0.9.7.1/
    external/glew-1.13.0/include/
    ${lua_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    .
)

# Liste des bibliothèques à lier
set(ALL_LIBS
    ${OPENGL_LIBRARY}
    glfw
    GLEW_1130
    imgui
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