# Piezas comunes de Tufi y Poty, en C++ estándar (sin JUCE). Las fuentes se compilan dentro
# de cada destino que las use, igual que los motores de DSP.
add_library(ferb_core INTERFACE)
target_include_directories(ferb_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(ferb_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Oversampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Saturators.cpp)
target_compile_features(ferb_core INTERFACE cxx_std_20)

if(FERB_BUILD_TESTS)
    add_executable(CoreTests tests/CoreTests.cpp)
    target_include_directories(CoreTests PRIVATE ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(CoreTests PRIVATE ferb_core Catch2::Catch2WithMain)
    add_test(NAME CoreTests COMMAND CoreTests)
endif()
