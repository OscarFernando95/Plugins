juce_add_plugin(Tufi
    COMPANY_NAME "FERB"
    BUNDLE_ID com.ferb.tufi
    PLUGIN_MANUFACTURER_CODE Ferb
    PLUGIN_CODE Tufi
    FORMATS ${FERB_PLUGIN_FORMATS}
    PRODUCT_NAME "Tufi"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Distortion
    AAX_CATEGORY AAX_ePlugInCategory_Harmonic
    COPY_PLUGIN_AFTER_BUILD TRUE)

# Motor de DSP: sus fuentes se compilan dentro de cada destino que lo use (plugin y pruebas).
add_library(tufi_dsp INTERFACE)
target_sources(tufi_dsp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp/StageCurves.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp/ShapeTable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dsp/TufiEngine.cpp)
target_include_directories(tufi_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source)
target_link_libraries(tufi_dsp INTERFACE ferb_core juce::juce_audio_basics)

juce_add_binary_data(TufiAssets NAMESPACE TufiData HEADER_NAME TufiAssets.h SOURCES
    assets/fonts/DMSerifDisplay-Regular.ttf
    assets/fonts/JosefinSans-SemiBold.ttf
    assets/fonts/CourierPrime-Regular.ttf
    assets/fonts/CourierPrime-Bold.ttf)
set_target_properties(TufiAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_sources(Tufi PRIVATE
    source/Parameters.cpp
    source/Presets.cpp
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/ui/KnobUnit.cpp
    source/ui/ScopeView.cpp
    source/ui/VuMeter.cpp
    source/ui/TubeView.cpp)

target_compile_definitions(Tufi PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Tufi
    PRIVATE
        tufi_dsp
        ferb_ui
        TufiAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(FERB_BUILD_TESTS)
    add_executable(TufiTests tests/TufiEngineTests.cpp)
    target_compile_definitions(TufiTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(TufiTests PRIVATE ${PROJECT_SOURCE_DIR}/tests/common)
    # juce_dsp solo para la FFT de las pruebas de aliasing.
    target_link_libraries(TufiTests PRIVATE tufi_dsp juce::juce_dsp Catch2::Catch2WithMain)
    add_test(NAME TufiTests COMMAND TufiTests)

    # Procesador completo (con editor), sin anfitrión: lo que un validador comprueba desde fuera.
    add_executable(TufiProcessorTests tests/ProcessorTests.cpp)
    target_compile_definitions(TufiProcessorTests PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(TufiProcessorTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/core ${PROJECT_SOURCE_DIR}/tests/common)
    target_link_libraries(TufiProcessorTests PRIVATE Tufi TufiAssets juce::juce_gui_basics juce::juce_audio_utils Catch2::Catch2WithMain)
    add_test(NAME TufiProcessorTests COMMAND TufiProcessorTests)
endif()

# Renderiza el editor a un PNG para revisar la interfaz sin abrir un anfitrión.
if(FERB_BUILD_TOOLS)
    add_executable(TufiSnapshot ${PROJECT_SOURCE_DIR}/tools/Snapshot.cpp)
    target_compile_definitions(TufiSnapshot PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 FERB_PLUGIN_POTY=0 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(TufiSnapshot PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source ${PROJECT_SOURCE_DIR}/ui ${PROJECT_SOURCE_DIR}/core)
    target_link_libraries(TufiSnapshot PRIVATE Tufi TufiAssets juce::juce_gui_basics juce::juce_audio_utils)
endif()
