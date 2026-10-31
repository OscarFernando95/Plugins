# Piezas de interfaz y de presets compartidas por Tufi y Poty (necesitan JUCE).
# Las fuentes se compilan dentro de cada destino que las use, igual que el DSP.
add_library(ferb_ui INTERFACE)
target_include_directories(ferb_ui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(ferb_ui INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/PresetManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PresetBar.cpp)
target_link_libraries(ferb_ui INTERFACE juce::juce_gui_basics juce::juce_audio_processors)

# Kit de interfaz «aparato analógico» (Cremita, Maru, ChauS y Nel): tipografías incrustadas y piezas.
juce_add_binary_data(FerbVintageFonts NAMESPACE VintageData HEADER_NAME VintageFonts.h SOURCES
    vintage/fonts/Limelight-Regular.ttf
    vintage/fonts/Oxanium-Regular.ttf
    vintage/fonts/Oxanium-Medium.ttf
    vintage/fonts/JosefinSans-SemiBold.ttf)
set_target_properties(FerbVintageFonts PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_library(ferb_vintage INTERFACE)
target_include_directories(ferb_vintage INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vintage)
target_sources(ferb_vintage INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/vintage/Vintage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vintage/Controls.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vintage/Meters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vintage/Frame.cpp)
target_link_libraries(ferb_vintage INTERFACE ferb_ui FerbVintageFonts)

if(FERB_BUILD_TESTS)
    add_executable(UiTests tests/PresetManagerTests.cpp)
    target_compile_definitions(UiTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(UiTests PRIVATE ferb_ui Catch2::Catch2WithMain)
    add_test(NAME UiTests COMMAND UiTests)
endif()
