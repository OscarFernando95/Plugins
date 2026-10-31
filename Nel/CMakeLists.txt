# Motor de DSP: sus fuentes se compilan dentro de cada destino que lo use (plugin y pruebas).
add_library(nel_dsp INTERFACE)
target_sources(nel_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp/NelEngine.cpp)
target_include_directories(nel_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source)
target_link_libraries(nel_dsp INTERFACE ferb_core juce::juce_audio_basics)

if(FERB_BUILD_TESTS)
    add_executable(NelTests tests/NelEngineTests.cpp)
    target_compile_definitions(NelTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(NelTests PRIVATE ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(NelTests PRIVATE nel_dsp juce::juce_dsp Catch2::Catch2WithMain)
    add_test(NAME NelTests COMMAND NelTests)
endif()
