juce_add_plugin(Cremita
    COMPANY_NAME "FERB"
    BUNDLE_ID com.ferb.cremita
    PLUGIN_MANUFACTURER_CODE Ferb
    PLUGIN_CODE Crem
    FORMATS ${FERB_PLUGIN_FORMATS}
    PRODUCT_NAME "Cremita"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Distortion
    AAX_CATEGORY AAX_ePlugInCategory_Harmonic
    COPY_PLUGIN_AFTER_BUILD TRUE)

# Motor de DSP: sus fuentes se compilan dentro de cada destino que lo use (plugin y pruebas).
add_library(cremita_dsp INTERFACE)
target_sources(cremita_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp/CremitaEngine.cpp)
target_include_directories(cremita_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source)
target_link_libraries(cremita_dsp INTERFACE ferb_core juce::juce_audio_basics)

target_sources(Cremita PRIVATE
    source/Parameters.cpp
    source/Presets.cpp
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/ui/CurveView.cpp)

target_compile_definitions(Cremita PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Cremita
    PRIVATE
        cremita_dsp
        ferb_ui
        ferb_vintage
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(FERB_BUILD_TESTS)
    add_executable(CremitaTests tests/CremitaEngineTests.cpp)
    target_compile_definitions(CremitaTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(CremitaTests PRIVATE ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(CremitaTests PRIVATE cremita_dsp juce::juce_dsp Catch2::Catch2WithMain)
    add_test(NAME CremitaTests COMMAND CremitaTests)

    # Procesador completo (con editor), sin anfitrión: lo que un validador comprueba desde fuera.
    add_executable(CremitaProcessorTests tests/ProcessorTests.cpp)
    target_compile_definitions(CremitaProcessorTests PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(CremitaProcessorTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/ui/vintage ${PROJECT_SOURCE_DIR}/core ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(CremitaProcessorTests PRIVATE Cremita juce::juce_gui_basics juce::juce_audio_utils Catch2::Catch2WithMain)
    add_test(NAME CremitaProcessorTests COMMAND CremitaProcessorTests)
endif()

# Renderiza el editor a un PNG para revisar la interfaz sin abrir un anfitrión.
if(FERB_BUILD_TOOLS)
    add_executable(CremitaSnapshot ${PROJECT_SOURCE_DIR}/tools/Snapshot.cpp)
    target_compile_definitions(CremitaSnapshot PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 FERB_PLUGIN_CREMITA=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(CremitaSnapshot PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/ui/vintage ${PROJECT_SOURCE_DIR}/core)
    target_link_libraries(CremitaSnapshot PRIVATE Cremita juce::juce_gui_basics juce::juce_audio_utils)
endif()
