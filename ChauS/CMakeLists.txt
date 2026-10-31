juce_add_plugin(ChauS
    COMPANY_NAME "FERB"
    VERSION ${CHAUS_VERSION}
    BUNDLE_ID com.ferb.chaus
    PLUGIN_MANUFACTURER_CODE Ferb
    PLUGIN_CODE ChaS
    FORMATS ${FERB_PLUGIN_FORMATS}
    PRODUCT_NAME "ChauS"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Dynamics
    AAX_CATEGORY AAX_ePlugInCategory_Dynamics
    COPY_PLUGIN_AFTER_BUILD TRUE)

# Motor de DSP: sus fuentes se compilan dentro de cada destino que lo use (plugin y pruebas).
add_library(chaus_dsp INTERFACE)
target_sources(chaus_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp/SibilanceDetector.cpp)
target_include_directories(chaus_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source)
target_link_libraries(chaus_dsp INTERFACE ferb_core)

target_sources(ChauS PRIVATE
    source/Parameters.cpp
    source/Presets.cpp
    source/PluginProcessor.cpp
    source/PluginEditor.cpp)

target_include_directories(ChauS PRIVATE source)
target_compile_definitions(ChauS PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ChauS
    PRIVATE
        chaus_dsp
        ferb_ui
        ferb_vintage
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(FERB_BUILD_TESTS)
    add_executable(ChauSTests tests/ChauSEngineTests.cpp tests/TrainClassifier.cpp)
    target_include_directories(ChauSTests PRIVATE ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(ChauSTests PRIVATE chaus_dsp Catch2::Catch2WithMain)
    add_test(NAME ChauSTests COMMAND ChauSTests)

    # Procesador completo (con editor), sin anfitrión: lo que un validador comprueba desde fuera.
    add_executable(ChauSProcessorTests tests/ProcessorTests.cpp)
    target_compile_definitions(ChauSProcessorTests PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(ChauSProcessorTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/ui/vintage ${PROJECT_SOURCE_DIR}/core ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(ChauSProcessorTests PRIVATE ChauS juce::juce_gui_basics juce::juce_audio_utils Catch2::Catch2WithMain)
    add_test(NAME ChauSProcessorTests COMMAND ChauSProcessorTests)
endif()

# Renderiza el editor a un PNG para revisar la interfaz sin abrir un anfitrión.
if(FERB_BUILD_TOOLS)
    add_executable(ChausSnapshot ${PROJECT_SOURCE_DIR}/tools/Snapshot.cpp)
    target_compile_definitions(ChausSnapshot PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 FERB_PLUGIN_CHAUS=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(ChausSnapshot PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/ui/vintage ${PROJECT_SOURCE_DIR}/core)
    target_link_libraries(ChausSnapshot PRIVATE ChauS juce::juce_gui_basics juce::juce_audio_utils)
endif()
