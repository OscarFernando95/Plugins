juce_add_plugin(Poty
    COMPANY_NAME "FERB"
    BUNDLE_ID com.ferb.poty
    PLUGIN_MANUFACTURER_CODE Ferb
    PLUGIN_CODE Poty
    FORMATS ${FERB_PLUGIN_FORMATS}
    PRODUCT_NAME "Poty"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Distortion
    AAX_CATEGORY AAX_ePlugInCategory_Harmonic
    COPY_PLUGIN_AFTER_BUILD TRUE)

# Motor de DSP: sus fuentes se compilan dentro de cada destino que lo use (plugin y pruebas).
add_library(poty_dsp INTERFACE)
target_sources(poty_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp/PotyEngine.cpp)
target_include_directories(poty_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source)
target_link_libraries(poty_dsp INTERFACE ferb_core juce::juce_audio_basics)

juce_add_binary_data(PotyAssets NAMESPACE PotyData HEADER_NAME PotyAssets.h SOURCES
    assets/fonts/Manrope-Medium.ttf
    assets/fonts/Manrope-Bold.ttf
    assets/fonts/JetBrainsMono-Medium.ttf
    assets/fonts/JetBrainsMono-Bold.ttf)
set_target_properties(PotyAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_sources(Poty PRIVATE
    source/Parameters.cpp
    source/Presets.cpp
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/ui/BandCard.cpp
    source/ui/GlobalPanel.cpp
    source/ui/SpectrumView.cpp)

target_compile_definitions(Poty PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Poty
    PRIVATE
        poty_dsp
        ferb_ui
        PotyAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(FERB_BUILD_TESTS)
    add_executable(PotyTests tests/PotyEngineTests.cpp)
    target_compile_definitions(PotyTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(PotyTests PRIVATE ${PROJECT_SOURCE_DIR}/tests/common)
    # juce_dsp solo para la referencia teórica de las pruebas (makePeakFilter).
    target_link_libraries(PotyTests PRIVATE poty_dsp juce::juce_dsp Catch2::Catch2WithMain)
    add_test(NAME PotyTests COMMAND PotyTests)

    # Procesador completo (con editor), sin anfitrión: lo que un validador comprueba desde fuera.
    add_executable(PotyProcessorTests tests/ProcessorTests.cpp)
    target_compile_definitions(PotyProcessorTests PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(PotyProcessorTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/core ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(PotyProcessorTests PRIVATE Poty PotyAssets juce::juce_gui_basics juce::juce_audio_utils Catch2::Catch2WithMain)
    add_test(NAME PotyProcessorTests COMMAND PotyProcessorTests)
endif()

# Renderiza el editor a un PNG para revisar la interfaz sin abrir un anfitrión.
if(FERB_BUILD_TOOLS)
    add_executable(PotySnapshot ${PROJECT_SOURCE_DIR}/tools/Snapshot.cpp)
    target_compile_definitions(PotySnapshot PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 FERB_PLUGIN_POTY=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(PotySnapshot PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/core)
    target_link_libraries(PotySnapshot PRIVATE Poty PotyAssets juce::juce_gui_basics juce::juce_audio_utils)
endif()
