# ferb-audio

Monorepo de plugins VST3 y AU para macOS (JUCE 8.0.15, CMake + Ninja), de uso personal. Etiqueta del creador en la interfaz: **By FERB**.

- **Tufi** (`Tufi/`): saturador armónico analógico. Especificación: `Tufi/Tufi — ficha técnica.md`.
- **Cremita**, **Nel**, **ChauS** y **Maru** (carpetas propias, con su ficha): Cremita, Nel, ChauS y Maru completos salvo pruebas en DAW (AAX compila pero sin validar ni firmar; el detector de ChauS y el estimador de Maru faltan por afinar con escucha). Kit de interfaz «aparato analógico» compartido en `ui/vintage/`; medidor de sonoridad BS.1770 en `core/Loudness.h` y sobremuestreador de 1× a 32× en `core/OversamplerN.h`.
- **Poty** (`Poty/`): enhancer multibanda. Especificación: `Poty/Poty — ficha técnica.md`.
- **Dra. McPoti** (`DraMcPoti/`): excitador armónico multibanda y dinámico (Vocal y Master, Vintage y Moderno), versión 1.0.0. Especificación: `DraMcPoti/Dra. McPoti — ficha técnica del excitador armónico.md`; maqueta: `DraMcPoti/Dra. McPoti — maqueta.html` (la referencia visual: **abrirla y renderizarla antes de tocar la interfaz**). Motor completo, interfaz según la maqueta, `auval` y pluginval nivel 10 pasan, PDF hecho.
- `Bosquejo de dos plugins.html`: bosquejo visual aprobado de ambas interfaces.

Las fichas son la fuente de verdad: leerlas antes de tocar un plugin. **Cada ficha termina con «Estado y cambios sobre la ficha»**: varias decisiones cambiaron tras medir y esa sección manda sobre el resto del texto. Los valores marcados como «iniciales» se afinan de oído y se actualizan en la ficha.

## Estructura

```
CMakeLists.txt        JUCE por FetchContent; opciones FERB_BUILD_TESTS y FERB_BUILD_TOOLS
core/                 DSP común, C++ estándar sin JUCE (Oversampler, Saturators, Adaa, DelayLine,
                      Biquad, DcBlocker, ModeSwitch, TripleBuffer, SampleRing, LevelMeter) + tests/
ui/                   Interfaz y presets comunes, con JUCE (PresetManager, PresetBar, ScaledEditor,
                      SegmentedControl, ParamField, Format) + tests/
tests/common/         Utilidades de prueba (señales, medidores, AliasingMeter, AllocGuard, ProcessorSuite)
tools/Snapshot.cpp    Renderiza el editor a un PNG sin anfitrión
Tufi/                 source/ (dsp/, ui/), assets/fonts/, tests/        (idem Poty, Cremita, Nel y ChauS)
Poty/                 source/ (dsp/, ui/), assets/fonts/, tests/
cmake/Versions.cmake  Versión SemVer de cada plugin (única fuente de verdad)
docs/                 PDF de documentación de cada plugin: fuentes/ (Markdown), estilo/, construir.py
README.md, CHANGELOG.md   Estado del repositorio e historial de versiones
```

Los motores de DSP (`TufiEngine`, `PotyEngine`) son independientes del anfitrión y de los parámetros: reciben una struct `Settings` y se prueban solos. `core/`, `ui/` y los motores son bibliotecas `INTERFACE` cuyas fuentes se compilan dentro de cada destino que las usa (plugin y pruebas).

## Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build                              # plugins, pruebas y herramientas
cmake --build build --target Tufi_VST3 Tufi_AU   # solo Tufi (idem Poty)
ctest --test-dir build                           # todas las pruebas
```

Universal (arm64 + x86_64) y macOS 11 por defecto. Para iterar más rápido: `-DCMAKE_OSX_ARCHITECTURES=arm64`.
AAX (Pro Tools), opcional: `-DFERB_AAX_SDK_PATH=/ruta/al/AAX_SDK` añade el formato a los seis plugins (sin esa opción no se compila). El SDK (2.8.0) está en `~/Downloads/aax-sdk-2-8-0` y **no se copia al repositorio**. Verificado: los seis compilan como AAX en arm64 (`cmake -S . -B <dir-aparte> -DCMAKE_OSX_ARCHITECTURES=arm64 -DFERB_AAX_SDK_PATH=… -DFERB_BUILD_TESTS=OFF -DFERB_BUILD_TOOLS=OFF`, `--target <Plugin>_AAX`); la compilación los copia a `/Library/Application Support/Avid/Audio/Plug-Ins/`. **Sin validar** (el AAX Validator y DigiShell no están en el SDK: se bajan aparte de developer.avid.com), **sin firma PACE** (`wraptool`, requiere cuenta de Avid e iLok) y **sin probar en Pro Tools**: Pro Tools de venta solo carga AAX firmados; sin firma solo carga en Pro Tools Developer. El SDK 2.8 se licencia como comercial o de código abierto: revisar junto con la licencia de JUCE antes de distribuir.
Cada build copia a `~/Library/Audio/Plug-Ins/{VST3,Components}`; JUCE firma ad-hoc el paquete antes de copiarlo (sin notarización).

Pruebas (Catch2): `CoreTests`, `UiTests`, `TufiTests`, `PotyTests`, `CremitaTests`, `NelTests`, `ChauSTests`, `DraMcPotiTests` (DSP y medidas) y `TufiProcessorTests`, `PotyProcessorTests`, `CremitaProcessorTests`, `NelProcessorTests`, `ChauSProcessorTests`, `DraMcPotiProcessorTests` (procesador completo con editor: parámetros aleatorios, tasas y bloques extremos, estado, vida del editor, latencia, y **cero reservas de memoria en `processBlock`**).
Informes de medidas, que no son pruebas: `./build/Poty/PotyTests "[.report]"`, `./build/Tufi/TufiTests "[.report]"` (aliasing, CPU, respuesta lineal) y `./build/ChauS/ChauSTests "[.report]"` (barrido del sesgo del detector, lookahead, CPU; con `CHAUS_TTS_DIR` las voces de `say`). El ajuste de los pesos de ChauS se repite con `ChauSTests "[.train]"`; con grabaciones propias, `CHAUS_REAL_FILE=… CHAUS_REAL_OUT=… ChauSTests "[.real]"` vuelca CSV y audio procesado (no escribir dentro de la carpeta de las grabaciones).
Interfaz: `./build/Poty/PotySnapshot salida.png [escala] [preset]` (idem `TufiSnapshot`; en ChauS `ChausSnapshot` y en Maru `MaruSnapshot` con `FERB_SNAPSHOT_BLOCKS=500`; en ChauS con `FERB_SNAPSHOT_BLOCKS=232` cae en medio de una «s») y comparar con el bosquejo o la maqueta (Chrome headless `--screenshot` sobre `…html#chaus`).
Validación: `auval -v aufx <código> Ferb` y `pluginval --strictness-level 10 --validate-in-process <paquete>`. `pluginval` no queda instalado: se baja de https://github.com/Tracktion/pluginval/releases (`pluginval_macOS.zip`) a una carpeta aislada. Nivel 10 pasa en Tufi, Poty, Cremita, Nel y ChauS (VST3 y AU).

## Reglas

- **Sin asignaciones de memoria ni bloqueos en `processBlock`.** Todo se reserva en `prepare`, para el máximo (8×); cambiar de modo no asigna. Lo vigila `*ProcessorTests` con un asignador instrumentado. `juce::ScopedNoDenormals` al inicio y parámetros suavizados con `juce::SmoothedValue`.
- **Latencia constante**: se declara la del factor máximo (8×, 47 muestras) y los demás modos retrasan lo que falta. Cambiar calidad o sobremuestreo nunca cambia la latencia.
- **Sin clics al mover cualquier control**: suavizado en parámetros continuos, y fundido a silencio y vuelta (`ModeSwitch`) para cambios de estructura. Toda perilla nueva debe pasar una prueba de «sin clics» comparando el salto máximo entre muestras con el de la señal estable.
- **Un editor se puede abrir antes de `prepareToPlay`** (pluginval lo hace): todo lo que el editor lea desde el procesador (anillos de muestras, telemetría) debe reservarse en el constructor, no en `prepareToPlay`.
- Todos los parámetros por `AudioProcessorValueTreeState` con `ParameterID{id, 1}`. No renombrar IDs: rompe presets y sesiones. El bypass del plugin es el del anfitrión (`getBypassParameter`) y no forma parte de los presets.
- Textos de interfaz en español con coma decimal. Cadenas con tildes o `×`: `juce::String::fromUTF8` (un literal `"Cálido"` suelto sale como `CÃ¡lido`).
- Un hito por vez, ejecutar la prueba que lo define antes de darlo por terminado, un commit por hito. El repositorio es <https://github.com/OscarFernando95/Plugins> (rama `main`).

## Versiones, README y documentación

- **Versión:** cada plugin tiene la suya, SemVer, en `cmake/Versions.cmake` (todos arrancan en 1.0.0). Mayor si se rompen presets o sesiones (IDs de parámetros, formato del estado); menor para funciones nuevas compatibles; parche para correcciones. Nunca se escribe a mano en otro sitio: CMake la pasa al paquete y `docs/construir.py` a la portada del PDF.
- **Al cerrar un hito o cambiar un plugin**, en el mismo commit: anotar el cambio en `CHANGELOG.md` (y subir la versión si procede), actualizar la tabla de estado del `README.md` y, si el plugin ya tiene documentación, editar `docs/fuentes/<Plugin>.md` y regenerar el PDF (`.cache/docs-venv/bin/python docs/construir.py <Plugin>`; ver `docs/README.md`). `construir.py` avisa si la versión del README no coincide con `cmake/Versions.cmake`.
- **Documentación:** un PDF por plugin en `docs/` con la identidad de `docs/Identidad de documentación.md`. Las cifras de calidad salen de las fichas y de las pruebas, con su condición de medida y fecha; lo no probado se declara. Plugins nuevos: ver «Añadir un plugin nuevo» en esa guía.

## Estado

Versiones: Tufi, Poty, Cremita, Nel, ChauS, Maru y Dra. McPoti en 1.0.0. PDF de documentación hechos: los siete.

### Tufi — hitos 1 a 9 hechos
Pendiente (motor): solo el 5 % de pares y las esquinas de Aire sobre Nyquist, descartados por ahora. comprobar a mano en Logic y Ableton; afinar de oído los valores iniciales (escalas de las etapas, coeficientes 0,02 y 0,4 de la forma).

### Poty — hitos 1 a 11 hechos
Pendiente: comprobar a mano en Logic y Ableton; afinar de oído los valores iniciales (estante de la cinta, `b` del tubo, `k` del germanio, escalas de intensidad).

### ChauS — hitos M0 a M5 hechos
Pendiente: ajustar el detector con voces grabadas (M6; hi-hat y «t» activan el detector y el 95 % de aciertos solo se midió con señales sintéticas), probar en Ableton, Logic y Pro Tools, y validar y firmar el AAX.

### Maru — hitos M0 a M5 hechos
Pendiente: escucharlo (ruido musical), probar en Ableton, Logic y Pro Tools, STOI y comparación con NS1 (no medidos), el estimador se infla ≈ 7 dB con voz continua y el Rango no se alcanza (≈ 21,5 dB), y validar y firmar el AAX.

### Dra. McPoti — 1.0.0
Pendiente: uso prolongado en mezclas reales y afinar de oído (coeficientes, umbrales de la guarda, plantillas, presets), probarlo sistemáticamente en Ableton, Logic y Pro Tools, y validar y firmar el AAX (no se hará en esta etapa). Descartado con razón: el 5 % de pares de Aire y las esquinas de Aire sobre Nyquist. El oversampling por defecto es 4× (2× queda a −61,1 dB, por debajo de su meta de −70). La fase lineal es la única opción que cambia la latencia.

### Pendiente en ambos
- Revisar las condiciones de licencia de JUCE antes de distribuir.
- Firma con Developer ID y notarización, si algún día se distribuye.
