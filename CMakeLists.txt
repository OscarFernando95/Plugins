# Debe fijarse antes de project() para que aplique a todo el árbol.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Versión mínima de macOS")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Binario universal")

cmake_minimum_required(VERSION 3.24)
project(ferb-audio VERSION 0.1.0 LANGUAGES C CXX)

option(FERB_BUILD_TESTS "Compila las pruebas (Catch2)" ON)
option(FERB_BUILD_TOOLS "Compila las herramientas de desarrollo (capturas de la interfaz)" ON)
if(FERB_BUILD_TESTS)
    enable_testing()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

if(FERB_BUILD_TESTS)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.7.1
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(Catch2)
endif()

# AAX (Pro Tools): el SDK de Avid no se descarga solo (requiere cuenta de desarrollador).
# Uso: cmake -B build -DFERB_AAX_SDK_PATH=/ruta/al/AAX_SDK
set(FERB_AAX_SDK_PATH "" CACHE PATH "Carpeta del AAX SDK de Avid; vacía = sin AAX")
set(FERB_PLUGIN_FORMATS VST3 AU)
if(FERB_AAX_SDK_PATH)
    if(NOT EXISTS "${FERB_AAX_SDK_PATH}/Interfaces/AAX.h")
        message(FATAL_ERROR "FERB_AAX_SDK_PATH no apunta a un AAX SDK (falta Interfaces/AAX.h): ${FERB_AAX_SDK_PATH}")
    endif()
    juce_set_aax_sdk_path("${FERB_AAX_SDK_PATH}")
    list(APPEND FERB_PLUGIN_FORMATS AAX)
endif()

add_subdirectory(core)
add_subdirectory(ui)

add_subdirectory(Tufi)
add_subdirectory(Poty)
