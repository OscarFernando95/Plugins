# FERB audio — plugins VST3 y AU

Monorepo de plugins de audio para macOS (JUCE 8.0.15, CMake + Ninja), de uso personal. Etiqueta del creador en las interfaces: **By FERB**.

## Plugins

<!-- La versión sale de cmake/Versions.cmake; docs/construir.py avisa si esta tabla no coincide. -->

| Plugin | Versión | Qué es | Estado | Documentación |
| --- | --- | --- | --- | --- |
| **Tufi** | 1.0.0 | Saturador armónico analógico (triodo, pentodo, diodo) | Completo; falta probarlo a mano en Logic y Ableton | [PDF](docs/Tufi%20%E2%80%94%20Documentaci%C3%B3n.pdf) · [ficha](Tufi/Tufi%20%E2%80%94%20ficha%20t%C3%A9cnica.md) |
| **Poty** | 1.0.0 | Enhancer multibanda de cinco bandas con saturación del realce | Completo; falta probarlo a mano en Logic y Ableton | [PDF](docs/Poty%20%E2%80%94%20Documentaci%C3%B3n.pdf) · [ficha](Poty/Poty%20%E2%80%94%20ficha%20t%C3%A9cnica.md) |
| **Cremita** | 1.0.0 | Inflator: sonoridad y calidez sin comprimir | Completo; falta probarlo en Ableton, Logic y Pro Tools | [PDF](docs/Cremita%20%E2%80%94%20Documentaci%C3%B3n.pdf) · [ficha](Cremita/Cremita%20%E2%80%94%20ficha%20t%C3%A9cnica%20del%20inflator.md) |
| **Nel** | 1.0.0 | Limitador de true peak con medición de sonoridad | Completo; falta probarlo en los anfitriones | [PDF](docs/Nel%20%E2%80%94%20Documentaci%C3%B3n.pdf) · [ficha](Nel/Nel%20%E2%80%94%20ficha%20t%C3%A9cnica%20del%20limitador.md) |
| **Maru** | 1.0.0 (prevista) | Gate y supresor de ruido | En curso: M0 a M3 hechos (esqueleto, Supresión, Gate, parámetros y presets) | [ficha](Maru/Maru%20%E2%80%94%20ficha%20t%C3%A9cnica%20del%20gate%20y%20supresor%20de%20ruido.md) |
| **ChauS** | 1.0.0 | De-esser con EQ dinámico que sigue la sibilante | Completo; falta probarlo en los anfitriones y ajustar el detector con voces grabadas | [PDF](docs/ChauS%20%E2%80%94%20Documentaci%C3%B3n.pdf) · [ficha](ChauS/ChauS%20%E2%80%94%20ficha%20t%C3%A9cnica%20del%20de-esser.md) |

Todos: VST3 y AU (AUv2, efecto), macOS 11 o superior, binario universal (arm64 y x86_64), 44,1 a 192 kHz. Las fichas son la fuente de verdad del diseño; cada una termina con «Estado y cambios sobre la ficha», que manda sobre el resto del texto.

## Compilar

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build                              # plugins, pruebas y herramientas
cmake --build build --target Tufi_VST3 Tufi_AU   # solo un plugin (idem Poty, Cremita, Nel, ChauS)
ctest --test-dir build                           # todas las pruebas
```

Para iterar más rápido: `-DCMAKE_OSX_ARCHITECTURES=arm64`. Cada compilación copia el plugin a `~/Library/Audio/Plug-Ins/{VST3,Components}` con firma ad hoc (sin notarización). AAX es opcional y requiere el SDK de Avid (`-DFERB_AAX_SDK_PATH=…`); no está compilado ni probado.

## Calidad

- Pruebas con Catch2: DSP y medidas de cada motor, y el procesador completo con editor (parámetros aleatorios, tasas y bloques extremos, estado, latencia y **cero reservas de memoria en `processBlock`**).
- Validación de formato: `auval` y `pluginval` nivel 10 pasan en Tufi, Poty, Cremita, Nel y ChauS (VST3 y AU).
- Reglas del proyecto (tiempo real, latencia constante, sin clics, IDs de parámetros estables): ver [CLAUDE.md](CLAUDE.md).

## Versiones

Cada plugin lleva su propia versión SemVer (`MAJOR.MINOR.PATCH`) en [cmake/Versions.cmake](cmake/Versions.cmake), que la pasa al paquete (`CFBundleShortVersionString`) y a la portada de su PDF. Todos arrancan en **1.0.0**. El historial está en [CHANGELOG.md](CHANGELOG.md).

## Estructura

```
cmake/Versions.cmake  Versión de cada plugin
core/                 DSP común sin JUCE (sobremuestreador, saturadores, ADAA, filtros, medidor de sonoridad)
ui/                   Interfaz y presets comunes (ui/vintage: kit «aparato analógico»)
tests/common/         Utilidades de prueba
tools/Snapshot.cpp    Renderiza el editor a un PNG sin anfitrión
Tufi/ Poty/ Cremita/ Nel/ Maru/ ChauS/   Un plugin por carpeta: source/, tests/ y su ficha
docs/                 Documentación en PDF de cada plugin y su generador
```

## Pendiente en todos

- Probar a mano en los anfitriones (Logic Pro, Ableton Live, Pro Tools).
- Documentación en PDF de Maru, cuando esté terminado.
- Revisar las condiciones de licencia de JUCE antes de distribuir.
- Firma con Developer ID y notarización, si algún día se distribuye.

## Licencia

Software de uso personal, sin licencia pública todavía. Está construido con [JUCE](https://juce.com) (su licencia debe revisarse antes de distribuir) y [Catch2](https://github.com/catchorg/Catch2).
