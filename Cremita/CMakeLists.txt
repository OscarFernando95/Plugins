# Motor de DSP: sus fuentes se compilan dentro de cada destino que lo use (plugin y pruebas).
add_library(cremita_dsp INTERFACE)
target_sources(cremita_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp/CremitaEngine.cpp)
target_include_directories(cremita_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source)
target_link_libraries(cremita_dsp INTERFACE ferb_core juce::juce_audio_basics)

if(FERB_BUILD_TESTS)
    add_executable(CremitaTests tests/CremitaEngineTests.cpp)
    target_compile_definitions(CremitaTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(CremitaTests PRIVATE ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(CremitaTests PRIVATE cremita_dsp juce::juce_dsp Catch2::Catch2WithMain)
    add_test(NAME CremitaTests COMMAND CremitaTests)
endif()
