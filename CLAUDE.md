# ferb-audio

Monorepo de plugins VST3 y AU para macOS (JUCE 8.0.15, CMake + Ninja), de uso personal. Etiqueta del creador en la interfaz: **By FERB**.

- **Tufi** (`Tufi/`): saturador armónico analógico. Especificación: `Tufi/Tufi — ficha técnica.md`.
- **Cremita**, **Nel**, **Maru** y **ChauS** (carpetas propias, con su ficha): Cremita y Nel completos salvo pruebas en DAW (Nel sin AAX); Maru y ChauS por hacer. Kit de interfaz «aparato analógico» compartido en `ui/vintage/`; medidor de sonoridad BS.1770 en `core/Loudness.h` y sobremuestreador de 1× a 32× en `core/OversamplerN.h`.
- **Poty** (`Poty/`): enhancer multibanda. Especificación: `Poty/Poty — ficha técnica.md`.
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
Tufi/                 source/ (dsp/, ui/), assets/fonts/, tests/
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
AAX (Pro Tools), opcional: `-DFERB_AAX_SDK_PATH=/ruta/al/AAX_SDK` añade el formato a ambos plugins (sin esa opción no se compila). El SDK se descarga de developer.avid.com con cuenta de desarrollador; **no está en este equipo**. Pro Tools de venta solo carga AAX firmados con PACE (`wraptool`, requiere iLok); sin firma solo carga en Pro Tools Developer. AAX aún sin compilar ni probar.
Cada build copia a `~/Library/Audio/Plug-Ins/{VST3,Components}`; JUCE firma ad-hoc el paquete antes de copiarlo (sin notarización).

Pruebas (Catch2): `CoreTests`, `UiTests`, `TufiTests`, `PotyTests` (DSP y medidas) y `TufiProcessorTests`, `PotyProcessorTests` (procesador completo con editor: parámetros aleatorios, tasas y bloques extremos, estado, vida del editor, latencia, y **cero reservas de memoria en `processBlock`**).
Informes de medidas, que no son pruebas: `./build/Poty/PotyTests "[.report]"` y `./build/Tufi/TufiTests "[.report]"` (aliasing, CPU, respuesta lineal).
Interfaz: `./build/Poty/PotySnapshot salida.png [escala] [preset]` (idem `TufiSnapshot`) y comparar con el bosquejo.
Validación: `auval -v aufx <código> Ferb` y `pluginval --strictness-level 10 --validate-in-process <paquete>`. `pluginval` no queda instalado: se baja de https://github.com/Tracktion/pluginval/releases (`pluginval_macOS.zip`) a una carpeta aislada. Nivel 10 pasa en Tufi, Poty, Cremita y Nel (VST3 y AU).

## Reglas

- **Sin asignaciones de memoria ni bloqueos en `processBlock`.** Todo se reserva en `prepare`, para el máximo (8×); cambiar de modo no asigna. Lo vigila `*ProcessorTests` con un asignador instrumentado. `juce::ScopedNoDenormals` al inicio y parámetros suavizados con `juce::SmoothedValue`.
- **Latencia constante**: se declara la del factor máximo (8×, 47 muestras) y los demás modos retrasan lo que falta. Cambiar calidad o sobremuestreo nunca cambia la latencia.
- **Sin clics al mover cualquier control**: suavizado en parámetros continuos, y fundido a silencio y vuelta (`ModeSwitch`) para cambios de estructura. Toda perilla nueva debe pasar una prueba de «sin clics» comparando el salto máximo entre muestras con el de la señal estable.
- Todos los parámetros por `AudioProcessorValueTreeState` con `ParameterID{id, 1}`. No renombrar IDs: rompe presets y sesiones. El bypass del plugin es el del anfitrión (`getBypassParameter`) y no forma parte de los presets.
- Textos de interfaz en español con coma decimal. Cadenas con tildes o `×`: `juce::String::fromUTF8` (un literal `"Cálido"` suelto sale como `CÃ¡lido`).
- Un hito por vez, ejecutar la prueba que lo define antes de darlo por terminado, un commit por hito. El repositorio es <https://github.com/OscarFernando95/Plugins> (rama `main`).

## Versiones, README y documentación

- **Versión:** cada plugin tiene la suya, SemVer, en `cmake/Versions.cmake` (todos arrancan en 1.0.0). Mayor si se rompen presets o sesiones (IDs de parámetros, formato del estado); menor para funciones nuevas compatibles; parche para correcciones. Nunca se escribe a mano en otro sitio: CMake la pasa al paquete y `docs/construir.py` a la portada del PDF.
- **Al cerrar un hito o cambiar un plugin**, en el mismo commit: anotar el cambio en `CHANGELOG.md` (y subir la versión si procede), actualizar la tabla de estado del `README.md` y, si el plugin ya tiene documentación, editar `docs/fuentes/<Plugin>.md` y regenerar el PDF (`.cache/docs-venv/bin/python docs/construir.py <Plugin>`; ver `docs/README.md`). `construir.py` avisa si la versión del README no coincide con `cmake/Versions.cmake`.
- **Documentación:** un PDF por plugin en `docs/` con la identidad de `docs/Identidad de documentación.md`. Las cifras de calidad salen de las fichas y de las pruebas, con su condición de medida y fecha; lo no probado se declara. Plugins nuevos: ver «Añadir un plugin nuevo» en esa guía.

## Estado

Versiones: Tufi, Poty, Cremita y Nel en 1.0.0 (las de Maru y ChauS, prevista). PDF de documentación hechos: Tufi, Poty y Cremita.

### Tufi — hitos 1 a 9 hechos
Pendiente: comprobar a mano en Logic y Ableton; afinar de oído los valores iniciales (escalas de las etapas, coeficientes 0,02 y 0,4 de la forma).

### Poty — hitos 1 a 11 hechos
Pendiente: comprobar a mano en Logic y Ableton; afinar de oído los valores iniciales (estante de la cinta, `b` del tubo, `k` del germanio, escalas de intensidad).

### Pendiente en ambos
- Revisar las condiciones de licencia de JUCE antes de distribuir.
- Firma con Developer ID y notarización, si algún día se distribuye.
