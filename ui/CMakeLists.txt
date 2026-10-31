# Piezas de interfaz y de presets compartidas por Tufi y Poty (necesitan JUCE).
# Las fuentes se compilan dentro de cada destino que las use, igual que el DSP.
add_library(ferb_ui INTERFACE)
target_include_directories(ferb_ui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(ferb_ui INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/PresetManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PresetBar.cpp)
target_link_libraries(ferb_ui INTERFACE juce::juce_gui_basics juce::juce_audio_processors)

if(FERB_BUILD_TESTS)
    add_executable(UiTests tests/PresetManagerTests.cpp)
    target_compile_definitions(UiTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(UiTests PRIVATE ferb_ui Catch2::Catch2WithMain)
    add_test(NAME UiTests COMMAND UiTests)
endif()
