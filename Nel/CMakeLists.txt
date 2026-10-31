juce_add_plugin(Nel
    COMPANY_NAME "FERB"
    BUNDLE_ID com.ferb.nel
    PLUGIN_MANUFACTURER_CODE Ferb
    PLUGIN_CODE Nel1
    FORMATS ${FERB_PLUGIN_FORMATS}
    PRODUCT_NAME "Nel"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Dynamics
    AAX_CATEGORY AAX_ePlugInCategory_Dynamics
    COPY_PLUGIN_AFTER_BUILD TRUE)

# Motor de DSP: sus fuentes se compilan dentro de cada destino que lo use (plugin y pruebas).
add_library(nel_dsp INTERFACE)
target_sources(nel_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp/NelEngine.cpp)
target_include_directories(nel_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source)
target_link_libraries(nel_dsp INTERFACE ferb_core juce::juce_audio_basics)

target_sources(Nel PRIVATE
    source/Parameters.cpp
    source/Presets.cpp
    source/PluginProcessor.cpp
    source/PluginEditor.cpp)

target_compile_definitions(Nel PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Nel
    PRIVATE
        nel_dsp
        ferb_ui
        ferb_vintage
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(FERB_BUILD_TESTS)
    add_executable(NelTests tests/NelEngineTests.cpp)
    target_compile_definitions(NelTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(NelTests PRIVATE ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(NelTests PRIVATE nel_dsp juce::juce_dsp Catch2::Catch2WithMain)
    add_test(NAME NelTests COMMAND NelTests)

    # Procesador completo (con editor), sin anfitrión: lo que un validador comprueba desde fuera.
    add_executable(NelProcessorTests tests/ProcessorTests.cpp)
    target_compile_definitions(NelProcessorTests PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(NelProcessorTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/ui/vintage ${PROJECT_SOURCE_DIR}/core ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(NelProcessorTests PRIVATE Nel juce::juce_gui_basics juce::juce_audio_utils Catch2::Catch2WithMain)
    add_test(NAME NelProcessorTests COMMAND NelProcessorTests)
endif()

# Renderiza el editor a un PNG para revisar la interfaz sin abrir un anfitrión.
if(FERB_BUILD_TOOLS)
    add_executable(NelSnapshot ${PROJECT_SOURCE_DIR}/tools/Snapshot.cpp)
    target_compile_definitions(NelSnapshot PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 FERB_PLUGIN_NEL=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(NelSnapshot PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/ui/vintage ${PROJECT_SOURCE_DIR}/core)
    target_link_libraries(NelSnapshot PRIVATE Nel juce::juce_gui_basics juce::juce_audio_utils)
endif()
