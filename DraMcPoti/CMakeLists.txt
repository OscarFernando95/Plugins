juce_add_plugin(DraMcPoti
    COMPANY_NAME "FERB"
    VERSION ${DRAMCPOTI_VERSION}
    BUNDLE_ID com.ferb.dramcpoti
    PLUGIN_MANUFACTURER_CODE Ferb
    PLUGIN_CODE McPo
    FORMATS ${FERB_PLUGIN_FORMATS}
    PRODUCT_NAME "Dra. McPoti"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Distortion
    AAX_CATEGORY AAX_ePlugInCategory_Harmonic
    COPY_PLUGIN_AFTER_BUILD TRUE)

# Motor de DSP: sus fuentes se compilan dentro de cada destino que lo use (plugin y pruebas).
add_library(mcpoti_dsp INTERFACE)
target_sources(mcpoti_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp/McPotiEngine.cpp)
target_include_directories(mcpoti_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source)
target_link_libraries(mcpoti_dsp INTERFACE ferb_core juce::juce_audio_basics)

target_sources(DraMcPoti PRIVATE
    source/Parameters.cpp
    source/Presets.cpp
    source/PluginProcessor.cpp
    source/PluginEditor.cpp)

target_include_directories(DraMcPoti PRIVATE source)
target_compile_definitions(DraMcPoti PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(DraMcPoti
    PRIVATE
        mcpoti_dsp
        ferb_ui
        ferb_vintage
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(FERB_BUILD_TESTS)
    add_executable(DraMcPotiTests tests/EngineTests.cpp)
    target_compile_definitions(DraMcPotiTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(DraMcPotiTests PRIVATE ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(DraMcPotiTests PRIVATE mcpoti_dsp juce::juce_dsp Catch2::Catch2WithMain)
    add_test(NAME DraMcPotiTests COMMAND DraMcPotiTests)

    add_executable(DraMcPotiProcessorTests tests/ProcessorTests.cpp)
    target_compile_definitions(DraMcPotiProcessorTests PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(DraMcPotiProcessorTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/ui/vintage ${PROJECT_SOURCE_DIR}/core ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(DraMcPotiProcessorTests PRIVATE DraMcPoti juce::juce_gui_basics juce::juce_audio_utils Catch2::Catch2WithMain)
    add_test(NAME DraMcPotiProcessorTests COMMAND DraMcPotiProcessorTests)
endif()

# Renderiza el editor a un PNG para revisar la interfaz sin abrir un anfitrión.
if(FERB_BUILD_TOOLS)
    add_executable(DraMcPotiSnapshot ${PROJECT_SOURCE_DIR}/tools/Snapshot.cpp)
    target_compile_definitions(DraMcPotiSnapshot PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 FERB_PLUGIN_DRAMCPOTI=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(DraMcPotiSnapshot PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/ui/vintage ${PROJECT_SOURCE_DIR}/core)
    target_link_libraries(DraMcPotiSnapshot PRIVATE DraMcPoti juce::juce_gui_basics juce::juce_audio_utils)
endif()
