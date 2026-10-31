# Tufi — ficha técnica

Oct 28, 2026 · @Oscar Fernando

## Resumen

Tufi es un saturador armónico de sonido analógico, en VST3 y AU para macOS, de uso personal. Su referencia funcional es el RA II de Dave Hill Designs: una curva de transferencia que se dobla por zonas (nivel bajo, picos y mitades positiva y negativa), con Entrada y Salida para igualar volumen.

Objetivo de sonido: cálido, tipo válvula, con 2.º y 3.º armónico controlables y sin aliasing audible. Objetivo técnico: bajo consumo de CPU y un comportamiento medible igual o mejor que la referencia. Etiqueta del creador en la interfaz: **By FERB**.

> Tras implementarlo y medirlo, varias decisiones de esta ficha cambiaron. Están al final, en «Estado y cambios sobre la ficha», con los resultados medidos. Donde el texto de arriba choca con esa sección, manda esa sección.

## Alcance

| Tema | Decisión |
| --- | --- |
| Sistema | macOS 11 o superior, binario universal (arm64 y x86\_64) |
| Formatos | VST3 y AU (AUv2, tipo efecto). Sin AAX, CLAP, Windows ni Linux |
| Canales | Mono a mono y estéreo a estéreo, cada canal procesado de forma independiente |
| Frecuencias de muestreo | 44,1 a 192 kHz, tamaño de bloque variable |
| Distribución | Solo uso personal. Firma ad-hoc, sin notarización |
| Presets | 5 de fábrica y los del usuario, en `~/Library/Application Support/FERB/Tufi/Presets` |
| Ventana | Redimensionable del 60 % al 200 %, proporción fija; el tamaño se guarda en el estado |
| Fuera de alcance | Sidechain, MIDI, curva editable a mano |

Antes de distribuirlo hay que revisar las condiciones de licencia de JUCE.

## Parámetros

Todos pasan por `AudioProcessorValueTreeState` con `ParameterID{id, 1}` (versión 1). Los controles de cantidad van de 0 a 10 y se normalizan internamente a 0–1. Los valores por defecto son los del bosquejo.

| ID | Nombre en la interfaz | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `stage` | Etapa | Choice | Triodo, Pentodo, Diodo | Triodo | Cambia la curva base |
| `drive` | Entrada | Float | −6 a +6 dB | 0 | Suavizado de 20 ms, paso 0,1 |
| `lowAmount` | Nivel bajo · Cantidad | Float | 0 a 10 | 3,5 | 0 = sin efecto |
| `peakAmount` | Picos · Cantidad | Float | 0 a 10 | 5,5 | 0 = sin efecto |
| `peakHardness` | Picos · Dureza | Float | 0 a 10 | 4,0 | Techo de −3 dBFS (0) a −10 dBFS (10) |
| `evenTop` | Pares · Arriba | Float | 0 a 10 | 3,0 | Redondeo de la mitad positiva |
| `evenBottom` | Pares · Abajo | Float | 0 a 10 | 4,5 | Redondeo de la mitad negativa |
| `trim` | Salida | Float | −6 a +6 dB | 0 | Para igualar nivel |
| `oversample` | Sobremuestreo | Choice | Desactivado, 2×, 4×, 8× | 4× | Sobremuestreo de la curva |
| `bypass` | Bypass | Bool | apagado o encendido | Apagado | Fundido de 10 ms |

Con `evenTop` igual a `evenBottom` la curva es simétrica y solo hay armónicos impares.

## Arquitectura DSP

La señal pasa por esta cadena, por canal y sin enlace estéreo:

1. **Entrada.** Ganancia `drive` en dB, suavizada 20 ms.
2. **Sobremuestreo 2×** con `juce::dsp::Oversampling<float>` y filtro polifase IIR. La latencia se informa al anfitrión con `setLatencySamples`. Si `oversample` está apagado, se salta este paso y el 4.
3. **Curva de transferencia.** Una tabla de consulta (LUT) con interpolación, generada a partir de la etapa elegida y de los controles de forma.
4. **Submuestreo** de vuelta a la frecuencia original.
5. **Filtro de DC.** Paso alto de primer orden a 10 Hz, necesario porque la asimetría genera componente continua.
6. **Salida.** Ganancia `trim`, suavizada 20 ms.
7. **Medidor.** El nivel de pico de salida se publica en un `std::atomic<float>` que lee el VU de la interfaz.

El bypass cruza al 100 % seco con un fundido de 10 ms, retrasando la señal seca lo mismo que la latencia del sobremuestreo para no cambiar el tiempo.

## Modelos de las etapas

Cada etapa define una curva base. Se calculan fuera del hilo de audio y se guardan en una tabla de 4096 puntos para entradas de −2 a +2 (hasta +6 dBFS tras la entrada), con interpolación cúbica.

| Etapa | Carácter | Base matemática | Cómo se obtiene la tabla |
| --- | --- | --- | --- |
| Triodo | Curva suave, domina el 2.º armónico | Modelo de Koren de triodo y recta de carga | Se resuelve la corriente de placa para cada tensión de entrada y se normaliza |
| Pentodo | Más ganancia, rodilla marcada, más impares | Modelo de Koren de pentodo con rejilla pantalla | Igual, con tensión de pantalla fija |
| Diodo | Recorte seco al llegar al codo | Ecuación de Shockley: par antiparalelo `2·Is·sinh(V/(n·Vt))` | Se resuelve el recorte con resistencia en serie y se normaliza. Un diodo contra dos en serie da asimetría |

Todas se normalizan para que la pendiente en cero sea 1: con los controles de forma en 0 la curva es una recta y Tufi queda transparente.

Los controles de forma se aplican sobre la curva base:

- **Nivel bajo:** pendiente en cero igual a `1 + lowAmount / 10`, mediante el término `k·x·exp(−x²/0,02)`.
- **Picos:** mezcla entre la curva y un recorte suave por encima del umbral `T`, de −3 a −10 dBFS según la dureza.
- **Pares:** umbrales distintos por mitad, `Tarriba = T·(1 − 0,4·evenTop/10)` y `Tabajo = T·(1 − 0,4·evenBottom/10)`.

```cpp
float lowLevel(float x, float k) { return x + k * x * std::exp(-x * x / 0.02f); }

float peakSoft(float x, float T) {
    float a = std::abs(x);
    if (a <= T) return x;
    return std::copysign(T + (1.0f - T) * std::tanh((a - T) / (1.0f - T)), x);
}

float shape(float x) {          // x ya con drive aplicado
    float y = lowLevel(x, k);
    y = baseCurve(y);           // curva de la etapa elegida
    float p = y >= 0 ? peakSoft(y, Ttop) : peakSoft(y, Tbottom);
    return y + peakMix * (p - y);
}
```

Los coeficientes (0,02 y 0,4) son valores iniciales para afinar de oído. La tabla que lee el hilo de audio muestrea `shape()` completa, así el procesado por muestra es una sola consulta. Cuando cambia un control de forma, la tabla se reconstruye en un hilo secundario y se intercambia de forma atómica con doble búfer.

## Optimización y consumo de CPU

Meta a medir, no un dato: **2 % o menos de un núcleo** en estéreo, 48 kHz, bloque de 256 y sobremuestreo 2×.

- Una consulta a tabla por muestra, sin `exp`, `tanh` ni `pow` en el hilo de audio.
- La tabla se reconstruye fuera del hilo de audio y se intercambia de forma atómica.
- Sin asignaciones de memoria ni bloqueos en `processBlock`; `juce::ScopedNoDenormals` al inicio.
- Parámetros suavizados con `juce::SmoothedValue`, actualizados por bloque.
- Solo sobremuestreo 2× con filtro IIR polifase, que cuesta menos que el FIR.
- Con los controles de forma en 0 y la entrada en 0 dB, el bloque se procesa como ganancia simple y se salta la curva.
- Interfaz: un único temporizador a 30 Hz para medidor y osciloscopio, y la curva se dibuja desde 200 puntos precalculados.

Cada decisión se comprueba con las pruebas de la sección «Pruebas y validación».

## Interfaz

Bosquejo aprobado en el lienzo «Bosquejo de dos plugins», pantalla de Tufi. Tamaño base 1080 × 600 px; la ventana se redimensiona y todo el contenido se escala con ella. Estilo de equipo de los años 60.

| Zona | Contenido |
| --- | --- |
| Cabecera (72 px) | Nombre «Tufi» con la placa **By FERB** al lado, subtítulo, teclas de etapa (Triodo, Pentodo, Diodo) con lámpara piloto, 2× OS y Bypass |
| Columna izquierda (270 px) | Osciloscopio verde con la curva de transferencia y tres contadores: pendiente, techo y asimetría |
| Columna central | Grupos Nivel bajo, Picos y Pares, y las dos perillas grandes Entrada y Salida |
| Columna derecha (250 px) | VU con aguja que marca la salida y ventana con una válvula encendida |

Elementos de código:

- **Perillas:** `LookAndFeel_V4` propio. Barrido de 270° (de 135° a 405°), 11 marcas grabadas y puntero crema. Tamaños: 84 px (Nivel bajo), 64 px (Picos y Pares) y 118 px (Entrada y Salida).
- **VU:** aguja con balística de 300 ms. Arco de −48° a +48°, el cero a +20° y zona roja de 0 a +3.
- **Osciloscopio:** se dibuja `shape()` con 200 puntos en un `juce::Path`, con un trazo ancho translúcido bajo el trazo fino y líneas de barrido encima.
- **Etiqueta By FERB:** placa pequeña con borde de latón junto al nombre, Josefin Sans 600, 12 px, letras espaciadas.

| Elemento | Valor |
| --- | --- |
| Panel marfil | `#E4D6B4` |
| Tinta | `#2B2118` |
| Tinta suave | `#5B4A38` |
| Latón | `#B08D4F` |
| Baquelita | `#17110D` |
| Nogal (laterales) | `#4A2D1A` |
| Puntero crema | `#E6D9BB` |
| Fósforo y fondo del osciloscopio | `#8DFFA8` y `#07140B` |
| Ámbar de contadores y lámparas | `#FFB347` |
| Fondo del VU | `#F2DCA0` |
| Rojo del VU | `#A8321D` |

Tipografías: DM Serif Display (nombre), Josefin Sans (rótulos) y Courier Prime (valores), incrustadas con `juce_add_binary_data`.

## Proyecto y build

```
tufi/
├─ CMakeLists.txt
├─ CLAUDE.md
├─ docs/FICHA.md
├─ assets/fonts/
├─ source/
│  ├─ PluginProcessor.h / .cpp
│  ├─ PluginEditor.h / .cpp
│  ├─ Parameters.h / .cpp
│  ├─ dsp/   (StageCurves, ShapeTable, DcBlocker, LevelMeter)
│  └─ ui/    (TufiLookAndFeel, ScopeView, VuMeter, TubeView, KeyButton)
├─ tests/    (Catch2 y analizador de armónicos)
└─ tools/    (scripts de análisis en Python)
```

Configuración en `CMakeLists.txt`, con JUCE 8 por `FetchContent` (fijar la etiqueta exacta al crear el proyecto):

```cmake
juce_add_plugin(Tufi
    COMPANY_NAME "FERB"
    BUNDLE_ID com.ferb.tufi
    PLUGIN_MANUFACTURER_CODE Ferb
    PLUGIN_CODE Tufi
    FORMATS VST3 AU
    PRODUCT_NAME "Tufi"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Distortion
    COPY_PLUGIN_AFTER_BUILD TRUE)
```

Compilar desde la terminal de VS Code:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

Los plugins se copian a `~/Library/Audio/Plug-Ins/VST3` y `~/Library/Audio/Plug-Ins/Components`. Firma ad-hoc: `codesign --force --deep -s - <paquete>`. Extensiones de VS Code: CMake Tools y clangd.

Poty se desarrolla en el mismo repositorio y las piezas comunes están en `core/` (DSP) y `ui/` (interfaz y presets). La estructura real está en `CLAUDE.md`; esto era el plan inicial.

## Pruebas y validación

Las cifras son metas, no resultados medidos.

| Prueba | Cómo | Meta |
| --- | --- | --- |
| Transparencia | Controles de forma en 0, entrada y salida en 0 dB; restar entrada y salida | Residuo por debajo de −90 dBFS |
| Armónicos | Senoidal de 1 kHz a −12, −6 y −3 dBFS, FFT de 65 536 puntos, medir H2 a H7 | Perfil esperado por etapa: en Triodo domina H2, en Diodo simétrico solo hay impares |
| Aliasing | Senoidal de 9 kHz a −6 dBFS con 2× apagado y encendido; medir la energía fuera de los armónicos | Con 2× encendido, por debajo de −80 dB respecto de la fundamental |
| Continua | Asimetría máxima, 10 s de señal | Componente continua por debajo de −80 dBFS tras el filtro |
| CPU | 60 s de estéreo a 48 kHz, bloque de 256, medir el tiempo de proceso | 2 % o menos de un núcleo |
| Compatibilidad | `auval -v aufx Tufi Ferb` y `pluginval` en nivel 5 o más | Sin errores |

Sobre la referencia: el RA II figura como plugin AAX para Pro Tools, así que la comparación se hace midiendo el mismo perfil de armónicos y de oído, sin cargar ambos en el mismo anfitrión.

## Hitos de desarrollo y guía para Claude Code

1. Esqueleto: CMake, procesador y editor vacío; carga en el anfitrión y pasa `auval`.
2. Parámetros con `AudioProcessorValueTreeState` y guardado de estado.
3. DSP base: entrada, salida, curva `tanh`, filtro de DC y prueba de transparencia.
4. Sobremuestreo 2× con la latencia informada.
5. Curvas por etapa (triodo, pentodo, diodo), controles de forma y tabla con doble búfer; script de armónicos.
6. Interfaz: `LookAndFeel`, perillas, teclas con lámpara y contadores.
7. Osciloscopio de la curva, VU y ventana de la válvula; placa By FERB.
8. Optimización, medición de CPU y `pluginval`.
9. Pulido y copia final a las carpetas de plugins.

Cómo trabajar con Claude Code:

- Guarda esta ficha como `docs/FICHA.md` y crea un `CLAUDE.md` con los comandos de build, la regla de no asignar memoria en `processBlock` y la lista de hitos.
- Pide un hito por vez: «Lee `docs/FICHA.md` y `CLAUDE.md` e implementa el hito 3». Un commit por hito.
- Pide siempre que ejecute la prueba que define el hito antes de darlo por terminado.
- Si un valor marcado como inicial no suena bien, ajústalo de oído y actualiza la ficha.

## Estado y cambios sobre la ficha

Fecha: 28 oct 2026. Los hitos 1 a 9 están implementados y las pruebas pasan (`ctest`, `auval`). Pendiente: validar con `pluginval` (la herramienta no se pudo descargar sin autorización) y comprobar a mano las funciones nuevas en Logic y Ableton.

**Decisiones que cambiaron**

| Tema | Ficha original | Ahora | Por qué |
| --- | --- | --- | --- |
| Aliasing | Sobremuestreo 2× | ADAA de primer orden sobre la tabla, más sobremuestreo seleccionable: Desactivado, 2×, 4×, 8×; defecto 4× | Con 2× solo se llega a −59 dB en el caso medido; 4× llega a −123 dB. El coste es mínimo (ver CPU) |
| Interpolación de la tabla | Cúbica | Lineal a tramos con 4096 puntos, con antiderivada exacta | Permite ADAA sin funciones trascendentes en el hilo de audio; el error frente a la curva continua es ~1e-7 |
| Sobremuestreo | `juce::dsp::Oversampling`, IIR polifase | Etapas de media banda FIR propias (Kaiser, ~100 dB, fase lineal) | Latencia entera exacta y el mismo componente que usa Poty |
| Latencia | Informada, según el 2× | Constante, 47 muestras (la del 8×), en todos los modos | Cambiar el sobremuestreo no obliga al anfitrión a recalcular la compensación |
| Curva con los controles en 0 | Recta | Recta exacta: el mezclado de picos parte de la señal con el nivel bajo aplicado, no de la curva de la etapa | El fragmento de la ficha devolvía la curva de la etapa con Picos · Cantidad en 0, lo que contradecía «Tufi queda transparente» |
| Cambio de una perilla de forma | Tabla nueva con intercambio atómico | La tabla de trabajo se acerca a la nueva en ~15 ms (de forma suave), calculada en un hilo secundario y entregada sin bloqueos | Un intercambio de golpe suena a clic |
| Cambio de sobremuestreo, o llevar la forma a 0 | — | Fundido de 5 ms hacia la ruta de ganancia simple, cambio, calentamiento y vuelta | Pedido expreso: sin clics |
| Etapas | «Valores por afinar» | Triodo: Koren 12AX7, recta de carga de 100 kΩ con 300 V, escala 2,5. Pentodo: Koren tipo EL34, 2,5 kΩ con 400 V, escala 8. Diodo: par antiparalelo de Shockley, escala 0,5 | Elegidos mirando el espectro de armónicos: triodo con H2 dominante, pentodo con más H3, diodo solo impares |
| Contadores | Sin definir | Pendiente en cero, techo = umbral de picos según la dureza, asimetría = parte par frente a impar en ±1 | El techo de −6 dB del bosquejo corresponde a dureza 4,0 |
| Presets | Fuera de alcance | 5 de fábrica, guardar, guardar como, eliminar, anterior y siguiente. El bypass no entra en un preset | Pedido expreso |
| Ventana | Fija | Redimensionable | Pedido expreso |
| Bypass | Parámetro propio | Es el bypass del anfitrión (`getBypassParameter`) | El botón del dispositivo en Ableton y el bypass de Logic usan el fundido de 10 ms |
| VU | — | 0 VU = −12 dBFS de pico; la aguja sigue el máximo entre lecturas, con 300 ms de balística | — |

**Resultados medidos** (48 kHz, bloque de 256, ajustes de fábrica salvo que se indique)

| Prueba | Meta | Resultado |
| --- | --- | --- |
| Transparencia | < −90 dBFS | Residuo nulo |
| Nivel bajo, pendiente en cero | `1 + cantidad/10` | 1,5 con cantidad 5, error < 1 % en todos los modos de sobremuestreo |
| Armónicos por etapa, 1 kHz a −6 dBFS | Triodo con H2 dominante; diodo simétrico solo con impares | Triodo H2 sobre H3 por más de 6 dB; pentodo con más H3 que el triodo; diodo con pares por debajo de −80 dB |
| Aliasing, 9 kHz a −6 dBFS | < −80 dB con el sobremuestreo activo | Sin sobremuestreo −46 dB, 2× −59 dB, **4× −123 dB**, **8× −130 dB** |
| Continua, asimetría máxima | < −80 dBFS | Cumple |
| CPU, estéreo | ≤ 2 % con 2× | Sin sobremuestreo 0,03 %, 2× 0,17 %, **4× 0,28 %**, 8× 0,45 % |
| Cambios de forma, etapa, sobremuestreo, paso por cero y bypass con audio sonando | Sin clics | Salto máximo entre muestras ≤ 1,4 veces el de la señal estable |
| `processBlock` y memoria | Sin asignaciones | Cero reservas, medidas con un asignador instrumentado |
| Procesador completo, 18 millones de comprobaciones | Sin NaN ni picos | 44,1 a 192 kHz, bloques de 1 a 4096, mono y estéreo, parámetros aleatorios con audio sonando |
| `auval -v aufx Tufi Ferb` | Sin errores | Pasa |

Un defecto encontrado y corregido durante las pruebas: el ADAA guardaba la antiderivada de la muestra anterior calculada con la tabla vieja; al mover una perilla la división `(F nueva − F vieja) / dv` producía picos de varias unidades. Ahora se recalcula con la tabla nueva.

**Presets de fábrica:** Triodo cálido (los valores por defecto), Pentodo con mordida, Diodo seco, Bus pegamento y Transparente.
