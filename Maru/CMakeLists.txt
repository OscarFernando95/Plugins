juce_add_plugin(Maru
    COMPANY_NAME "FERB"
    VERSION ${MARU_VERSION}
    BUNDLE_ID com.ferb.maru
    PLUGIN_MANUFACTURER_CODE Ferb
    PLUGIN_CODE Maru
    FORMATS ${FERB_PLUGIN_FORMATS}
    PRODUCT_NAME "Maru"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Restoration
    AAX_CATEGORY AAX_ePlugInCategory_NoiseReduction
    COPY_PLUGIN_AFTER_BUILD TRUE)

# Motor de DSP: sus fuentes se compilan dentro de cada destino que lo use (plugin y pruebas).
add_library(maru_dsp INTERFACE)
target_sources(maru_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp/MaruEngine.cpp)
target_include_directories(maru_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source)
target_link_libraries(maru_dsp INTERFACE ferb_core juce::juce_audio_basics juce::juce_dsp)

target_sources(Maru PRIVATE
    source/Parameters.cpp
    source/Presets.cpp
    source/PluginProcessor.cpp
    source/PluginEditor.cpp)

target_include_directories(Maru PRIVATE source)
target_compile_definitions(Maru PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Maru
    PRIVATE
        maru_dsp
        ferb_ui
        ferb_vintage
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(FERB_BUILD_TESTS)
    add_executable(MaruTests tests/MaruEngineTests.cpp tests/GateTests.cpp)
    target_compile_definitions(MaruTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(MaruTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(MaruTests PRIVATE maru_dsp juce::juce_dsp Catch2::Catch2WithMain)
    add_test(NAME MaruTests COMMAND MaruTests)

    # Procesador completo (con editor), sin anfitrión: lo que un validador comprueba desde fuera.
    add_executable(MaruProcessorTests tests/ProcessorTests.cpp)
    target_compile_definitions(MaruProcessorTests PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(MaruProcessorTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/ui/vintage ${PROJECT_SOURCE_DIR}/core ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(MaruProcessorTests PRIVATE Maru juce::juce_gui_basics juce::juce_audio_utils Catch2::Catch2WithMain)
    add_test(NAME MaruProcessorTests COMMAND MaruProcessorTests)
endif()

# Renderiza el editor a un PNG para revisar la interfaz sin abrir un anfitrión.
if(FERB_BUILD_TOOLS)
    add_executable(MaruSnapshot ${PROJECT_SOURCE_DIR}/tools/Snapshot.cpp)
    target_compile_definitions(MaruSnapshot PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 FERB_PLUGIN_MARU=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(MaruSnapshot PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/ui/vintage ${PROJECT_SOURCE_DIR}/core)
    target_link_libraries(MaruSnapshot PRIVATE Maru juce::juce_gui_basics juce::juce_audio_utils)
endif()
