# Poty — ficha técnica

Oct 28, 2026 · @Oscar Fernando

## Resumen

Poty es un enhancer multibanda moderno, en VST3 y AU para macOS, de uso personal. Su referencia funcional es el Spectre de Wavesfactory: un ecualizador de realce de cinco bandas cuya parte añadida pasa por un saturador, de modo que solo se generan armónicos donde se realza.

Cada banda elige su tipo de saturación (tubo, cinta, sólido, diodo de germanio o digital). Hay modos estéreo, mid y side, y una interfaz con analizador de espectro y nodos arrastrables. Objetivo técnico: bajo consumo de CPU y calidad medible igual o mejor que la referencia. Etiqueta del creador en la interfaz: **By FERB**.

> Tras implementarlo y medirlo, varias decisiones de esta ficha cambiaron. Están al final, en «Estado y cambios sobre la ficha», con los resultados medidos. Donde el texto de arriba choca con esa sección, manda esa sección.

## Alcance

| Tema | Decisión |
| --- | --- |
| Sistema | macOS 11 o superior, binario universal (arm64 y x86\_64) |
| Formatos | VST3 y AU (AUv2, tipo efecto). Sin AAX, CLAP, Windows ni Linux |
| Canales | Estéreo a estéreo y mono a mono, con modos Estéreo, Mid y Side |
| Frecuencias de muestreo | 44,1 a 192 kHz, tamaño de bloque variable |
| Presets | Básicos: archivos en `~/Library/Application Support/FERB/Poty/Presets`, con 5 de fábrica |
| Distribución | Solo uso personal. Firma ad-hoc, sin notarización |
| Ventana | Redimensionable del 60 % al 200 %, proporción fija; el tamaño se guarda en el estado |
| Fuera de alcance | Sexta banda, dinámica, sidechain, MIDI, bit-crusher |

Antes de distribuirlo hay que revisar las condiciones de licencia de JUCE.

## Parámetros

Todos pasan por `AudioProcessorValueTreeState` con `ParameterID{id, 1}`. Son 37 parámetros: 7 globales y 6 por banda.

**Globales**

| ID | Nombre en la interfaz | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `inGain` | Entrada | Float | −12 a +12 dB | 0 | Suavizado de 20 ms |
| `outGain` | Salida | Float | −12 a +12 dB | −0,5 | Suavizado de 20 ms |
| `mix` | Mezcla | Float | 0 a 100 % | 70 | Escala el camino saturado |
| `quality` | Calidad | Choice | Buena (1×), Mejor (2×), Máxima (4×) | Mejor | Sobremuestreo del saturador |
| `intensity` | Intensidad | Choice | Sutil, Media, Agresiva | Media | Escala el drive: ×0,6, ×1 y ×1,8 (valores iniciales) |
| `mode` | Modo | Choice | Estéreo, Mid, Side | Estéreo | Qué canal pasa por las bandas |
| `bypass` | Bypass | Bool | apagado o encendido | Apagado | Fundido de 10 ms |

**Por banda** (prefijo `b1_` a `b5_`)

| Sufijo | Tipo | Rango | Notas |
| --- | --- | --- | --- |
| `on` | Bool | apagado o encendido | Una banda apagada no se procesa |
| `freq` | Float | 20 a 20 000 Hz | Escala logarítmica |
| `gain` | Float | 0 a +12 dB | Solo realce |
| `q` | Float | 0,3 a 4 | Escala logarítmica |
| `type` | Choice | Tubo, Cinta, Sólido, Diodo Ge, Digital | Tipo de saturación |
| `drive` | Float | 0 a 100 % | Cantidad de saturación |

**Valores por defecto por banda** (los del bosquejo)

| Banda | Frecuencia | Ganancia | Q | Tipo | Drive |
| --- | --- | --- | --- | --- | --- |
| 1 | 90 Hz | +5,0 dB | 0,8 | Tubo | 45 % |
| 2 | 350 Hz | +3,0 dB | 1,0 | Cinta | 30 % |
| 3 | 1,8 kHz | +6,0 dB | 1,1 | Sólido | 55 % |
| 4 | 5,0 kHz | +8,0 dB | 1,2 | Diodo Ge | 70 % |
| 5 | 12 kHz | +7,0 dB | 0,7 | Digital | 40 % |

La banda seleccionada en la interfaz es estado de la ventana, no un parámetro.

## Arquitectura DSP

&#91;embedded content: flujo de señal · 5 bandas en paralelo\]

La salida es la señal original más la suma de lo que añade cada banda, escalada por la mezcla. Con los saturadores en modo lineal, el resultado es un ecualizador de realce en paralelo, útil como prueba de referencia.

```latex
y = x + \text{mezcla} \cdot \sum_{i=1}^{5} g_i(\Delta_i), \qquad \Delta_i = \text{realce}_i(x) - x
```

- **Modo:** en Mid o Side solo ese canal pasa por las bandas y el otro se deja intacto.
- **Saturador normalizado:** `g(u) = f(d·u) / d`, con `f'(0) = 1`. Con señal pequeña la banda se comporta como un realce exacto; la saturación aparece cuando el realce empuja niveles altos.
- **Drive efectivo:** `d = 1 + 19 · bN_drive/100 · escala de intensidad`, valor inicial que se ajusta de oído.
- **Calidad:** Buena es 2×, Mejor 4× y Máxima 8×, todas con ADAA de primer orden (ver «Tipos de saturación por banda»).
- Las bandas apagadas o con ganancia 0 se saltan por completo.

## Tipos de saturación por banda

Cada tipo es una función `f(v)` con `f'(0) = 1`, aplicada a `v = d·Δ` y dividida entre `d` (ver «Arquitectura DSP»). Todas tienen antiderivada cerrada, así que admiten ADAA de primer orden.

| Tipo | f(v) | Carácter | Antiderivada F(v) |
| --- | --- | --- | --- |
| Tubo | `(tanh(v + b) − tanh(b)) / sech²(b)`, con `b = 0,25` | Asimétrica: pares e impares, cálida | `(log cosh(v + b) − tanh(b)·v) / sech²(b)` |
| Cinta | `tanh(v)` entre un realce de agudos de +3 dB y su inverso | Simétrica y suave, con compresión de agudos | `log cosh(v)` |
| Sólido | `v / √(1 + v²)` | Simétrica, rodilla más marcada | `√(1 + v²)` |
| Diodo Ge | `asinh(k·v) / k`, con `k = 3` si `v ≥ 0` y `k = 2` si `v < 0` | Recorte asimétrico de diodo de germanio | `(v·asinh(k·v) − √(1 + k²v²)/k) / k`, por mitades, más una constante para ser continua en 0 |
| Digital | `clamp(v, −1, 1)` | Recorte duro simétrico | `v²/2` dentro de ±1 y `abs(v) − 1/2` fuera |

Los valores `b`, `k` y el realce de la cinta son iniciales y se afinan de oído. Para `log cosh` usa la forma estable `abs(v) + log1p(exp(−2·abs(v))) − log(2)`.

```cpp
// ADAA de primer orden: f es la función, F su antiderivada
float adaa(float v, float& vPrev, float& FPrev) {
    float Fv = F(v);
    float dv = v - vPrev;
    float y = std::abs(dv) > 1e-5f ? (Fv - FPrev) / dv
                                   : f(0.5f * (v + vPrev));
    vPrev = v;  FPrev = Fv;
    return y;
}
```

## Analizador de espectro y curva del EQ

- **Captura:** el hilo de audio copia la mezcla mono de la entrada y de la salida a dos colas sin bloqueos (`juce::AbstractFifo`, 4096 muestras cada una). Si el editor está cerrado, no copia nada.
- **FFT:** un `Timer` de la interfaz a 30 Hz toma 2048 muestras, aplica ventana Hann, ejecuta `juce::dsp::FFT` de orden 11 con `performFrequencyOnlyForwardTransform` y convierte a dB.
- **Balística:** sube de inmediato y baja 15 dB por segundo. Se dibuja de 20 Hz a 20 kHz en escala logarítmica y de 0 a −80 dB.
- **Curva del EQ:** suma en dB de la respuesta de los cinco filtros, calculada con `Coefficients::getMagnitudeForFrequencyArray` sobre 400 puntos logarítmicos. Solo se recalcula cuando cambia un parámetro de banda.
- **Nodos:** X = `bN_freq` (escala logarítmica) e Y = `bN_gain`. Arrastrar cambia ambos con `beginChangeGesture` y `endChangeGesture`; la rueda cambia `bN_q`; el doble clic restablece el valor por defecto; un clic selecciona la banda y muestra la etiqueta con frecuencia, ganancia, Q y tipo.
- **Dibujo:** rejilla y rótulos en una capa guardada como imagen (`setBufferedToImage`); espectros y curva como `juce::Path` en una capa que se repinta a 30 Hz.

El analizador toma las muestras de la frecuencia base, no de la sobremuestreada.

## Optimización y consumo de CPU

Meta a medir, no un dato: **3 % o menos de un núcleo** con las 5 bandas activas, estéreo, 48 kHz, bloque de 256 y calidad Mejor.

- Las bandas apagadas o con ganancia 0 se saltan por completo.
- Cada filtro de realce es un biquad; los coeficientes se recalculan solo cuando cambia un parámetro y se interpolan cada 32 muestras para evitar clics.
- El sobremuestreo se aplica solo al camino saturado. La señal original se retrasa lo mismo que el camino saturado y esa latencia, **constante** (47 muestras), se informa al anfitrión.
- Todas las calidades llevan ADAA además del sobremuestreo.
- La conversión a mid y side solo se hace cuando el modo no es Estéreo.
- Sin asignaciones de memoria ni bloqueos en `processBlock`; `juce::ScopedNoDenormals` al inicio y parámetros suavizados con `juce::SmoothedValue`.
- Interfaz: el analizador deja de leer las colas cuando el editor está cerrado, y todo el dibujo corre a 30 Hz.

Cada decisión se comprueba con las pruebas de la sección «Pruebas, validación e hitos de desarrollo».

## Interfaz

Bosquejo aprobado en el lienzo «Bosquejo de dos plugins», pantalla de Poty. Tamaño base 1100 × 710 px; la ventana se redimensiona y todo el contenido se escala con ella.

| Zona | Contenido |
| --- | --- |
| Cabecera (72 px) | Marca de cinco barras de colores, nombre «Poty» con la píldora **By FERB**, subtítulo, selector de preset, segmento Modo y Bypass |
| Analizador (1060 × 330 px) | Espectro de entrada (relleno) y de salida (línea), curva del EQ, cinco nodos numerados y etiqueta de la banda seleccionada. Área de dibujo de 980 × 230 px, de 20 Hz a 20 kHz y de 0 a −80 dB |
| Tarjetas de banda (5, de unos 133 px) | Frecuencia, ganancia, Q, barra de drive y selector de tipo de saturación; la banda seleccionada lleva borde de su color |
| Panel global (330 px) | Perillas Entrada, Salida y Mezcla, y segmentos Calidad e Intensidad |

Elementos de código:

- **Controles:** `LookAndFeel_V4` propio. Perillas de 60 px con barrido de 270°; botones y segmentos de 44 px de alto, el elegido relleno con el color de acento y texto oscuro.
- **Nodos:** círculo de 9 px de radio con el número de banda; el seleccionado lleva un aro de 16 px de radio y una línea punteada hasta el eje de frecuencias.
- **Etiqueta By FERB:** píldora con borde de 1 px junto al nombre, Manrope 700, 11 px, letras espaciadas, en el color de texto suave.

| Elemento | Valor |
| --- | --- |
| Fondo | `#0E1116` |
| Panel | `#151A22` |
| Borde | `#232B36` |
| Texto | `#E8EDF4` |
| Texto suave | `#9AA7B8` |
| Acento y trazo de salida | `#4CC9F0` |
| Bandas 1 a 5 | `#FF6B8A`, `#FFB454`, `#7EE08A`, `#4CC9F0`, `#B18CFF` |
| Espectro de entrada | relleno `#2A3646`, trazo `#3D5068` |
| Curva del EQ | `#E8EDF4` |

Tipografías: Manrope (interfaz) y JetBrains Mono (valores), incrustadas con `juce_add_binary_data`.

## Proyecto y build

Monorepo con bibliotecas comunes (`core/` de DSP, `ui/` de interfaz y presets) para las dos fichas (Tufi y Poty). La estructura real está en `CLAUDE.md`; esto era el plan inicial:

```
ferb-audio/
├─ CMakeLists.txt
├─ CLAUDE.md
├─ docs/ (FICHA_TUFI.md, FICHA_POTY.md)
├─ core/            (biblioteca estática compartida)
│  ├─ Saturators.h / .cpp   (f y F por tipo)
│  ├─ Adaa.h
│  ├─ DcBlocker.h
│  ├─ LevelMeter.h
│  └─ Smoothing.h
├─ tufi/            (source/, assets/, CMakeLists.txt)
└─ poty/
   ├─ CMakeLists.txt
   ├─ assets/fonts/
   └─ source/
      ├─ PluginProcessor.h / .cpp
      ├─ PluginEditor.h / .cpp
      ├─ Parameters.h / .cpp
      ├─ dsp/   (BandFilter, BandSaturator, MidSide, Oversampler)
      └─ ui/    (PotyLookAndFeel, SpectrumView, BandCard, SegmentedControl)
```

Configuración de `poty/CMakeLists.txt`, con JUCE 8 por `FetchContent` (fijar la etiqueta exacta al crear el proyecto):

```cmake
juce_add_plugin(Poty
    COMPANY_NAME "FERB"
    BUNDLE_ID com.ferb.poty
    PLUGIN_MANUFACTURER_CODE Ferb
    PLUGIN_CODE Poty
    FORMATS VST3 AU
    PRODUCT_NAME "Poty"
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
cmake --build build --target Poty_VST3 Poty_AU
```

Los plugins se copian a `~/Library/Audio/Plug-Ins/VST3` y `~/Library/Audio/Plug-Ins/Components`. Firma ad-hoc: `codesign --force --deep -s - <paquete>`. Extensiones de VS Code: CMake Tools y clangd.

## Pruebas, validación e hitos de desarrollo

Las cifras son metas, no resultados medidos.

| Prueba | Cómo | Meta |
| --- | --- | --- |
| Realce lineal | Saturadores en modo lineal; comparar la respuesta en frecuencia con la teórica | Diferencia menor de 0,1 dB de 20 Hz a 20 kHz |
| Transparencia | Todas las ganancias de banda en 0 | Residuo por debajo de −90 dBFS |
| Armónicos por tipo | Senoidal de 1 kHz y de 5 kHz con la banda 4 en +8 dB, cada tipo, FFT de 65 536 puntos | Tubo con pares e impares; Cinta y Sólido solo impares; Diodo Ge asimétrico |
| Aliasing | Barrido de senoidales en las tres calidades | Por debajo de −70 dB (Buena) y de −80 dB (Mejor y Máxima) respecto de la fundamental |
| Mid y Side | Señal solo en un lado | El otro lado queda sin cambios |
| CPU | 60 s de estéreo a 48 kHz, bloque de 256, 5 bandas activas, calidad Mejor | 3 % o menos de un núcleo |
| Compatibilidad | `auval -v aufx Poty Ferb` y `pluginval` en nivel 5 o más | Sin errores |
| Interfaz | Arrastrar nodos con audio sonando durante 10 minutos | Sin cortes ni crecimiento de memoria |

Hitos:

1. Esqueleto: CMake, procesador y editor vacío; carga en el anfitrión y pasa `auval`.
2. Los 37 parámetros con `AudioProcessorValueTreeState` y guardado de estado.
3. Realce en paralelo por diferencia, con la prueba de respuesta lineal.
4. Saturadores y ADAA; pruebas de armónicos y aliasing.
5. Sobremuestreo, calidades y latencia informada.
6. Modos Estéreo, Mid y Side.
7. Interfaz: `LookAndFeel`, tarjetas de banda, panel global y segmentos.
8. Analizador y curva del EQ con nodos arrastrables.
9. Presets: archivos y 5 de fábrica.
10. Optimización, medición de CPU y `pluginval`.
11. Pulido: píldora By FERB y copia final a las carpetas de plugins.

Cómo trabajar con Claude Code:

- Guarda esta ficha como `docs/FICHA_POTY.md` y crea un `CLAUDE.md` con los comandos de build, la regla de no asignar memoria en `processBlock` y la lista de hitos.
- Pide un hito por vez: «Lee `docs/FICHA_POTY.md` y `CLAUDE.md` e implementa el hito 4». Un commit por hito.
- Pide siempre que ejecute la prueba que define el hito antes de darlo por terminado.
- Si un valor marcado como inicial no suena bien, ajústalo de oído y actualiza la ficha.

## Estado y cambios sobre la ficha

Fecha: 28 oct 2026. Los hitos 1 a 11 están implementados y las pruebas pasan (`ctest`, `auval`). Pendiente: validar con `pluginval` (la herramienta no se pudo descargar sin autorización) y comprobar a mano las funciones nuevas en Logic y Ableton.

**Decisiones que cambiaron**

| Tema | Ficha original | Ahora | Por qué |
| --- | --- | --- | --- |
| Calidad | Buena 1×, Mejor 2×, Máxima 4× | Buena 2×, Mejor 4×, Máxima 8×; defecto Mejor | Medido: 1× llegaba a −36 dB de aliasing y 2× a −52 dB con saturación fuerte. El 1× ahorraba apenas 0,4 % de CPU |
| Sobremuestreo | Filtros de JUCE | Etapas de media banda FIR propias (Kaiser, ~100 dB de rechazo, fase lineal) | Latencia entera exacta: el original y el camino saturado se suman sin desfase |
| Latencia | Informada, según la calidad | Constante, 47 muestras (la del 8×), en todas las calidades | Cambiar de calidad no obliga al anfitrión a recalcular la compensación |
| ADAA | `float`, umbral 1e-5 | `double`, umbral 1e-4 | Restar `F(v) − F(v₋₁)` en `float` mete ruido; con `double` el punto medio es exacto a ese umbral |
| Cambios sin clics | No especificado | Calidad y modo: fundido de 5 ms a silencio, cambio, calentamiento y vuelta. Tipo de saturación: mezcla de 10 ms. Drive, intensidad, mezcla, entrada y salida: suavizado de 20 ms. Frecuencia, ganancia y Q: coeficientes interpolados | Pedido expreso: sin clics ni golpes al mover perillas |
| Analizador | FFT de 2048 | FFT de 8192 con anillo de muestras sin bloqueos | A 48 kHz, 2048 daba 23 Hz por intervalo y las bandas empiezan en 90 Hz |
| Presets | 5 de fábrica | 5 de fábrica, guardar, guardar como, eliminar, anterior y siguiente, carpeta propia. El bypass no entra en un preset | Pedido expreso |
| Ventana | Fija | Redimensionable | Pedido expreso |
| Bypass | Parámetro propio | Es el bypass del anfitrión (`getBypassParameter`) | El botón del dispositivo en Ableton y el bypass de Logic usan el fundido de 10 ms |

**Resultados medidos** (48 kHz, estéreo, bloque de 256, ajustes de fábrica salvo que se indique)

| Prueba | Meta | Resultado |
| --- | --- | --- |
| Respuesta lineal frente a la teórica | < 0,1 dB | Peor error 0,015 dB, en todas las combinaciones de 44,1, 48 y 96 kHz, las cuatro calidades internas y mezcla al 70 y 100 % |
| Transparencia, ganancias en 0 | < −90 dBFS | Por debajo de −90 dBFS en todas las calidades |
| Armónicos, senoidal a 1 y 5 kHz | Tubo y Ge con pares; Cinta, Sólido y Digital solo impares | Se cumple: pares por debajo de −80 dB en los simétricos |
| Aliasing, peor caso (banda centrada en la fundamental, +8 dB, saturación casi de recorte) | −70 dB (Buena), −80 dB (Mejor y Máxima) | Buena −54 dB, Mejor −68 dB, **Máxima −90 dB** |
| Aliasing con ajustes de fábrica, −6 dBFS, de 1 a 12 kHz | idem | Buena −52 dB, Mejor −73 dB, **Máxima −96 dB** |
| CPU, 5 bandas | ≤ 3 % con calidad Mejor | Buena 1,0 %, **Mejor 1,7 %**, Máxima 3,3 % (Mid o Side, la mitad) |
| Mid y Side | El otro canal queda igual | Residuo por debajo de −90 dB |
| Cambios de calidad, modo, tipo, drive, intensidad, nodos y bypass con audio sonando | Sin clics | Salto máximo entre muestras ≤ 1,3 veces el de la señal estable |
| `processBlock` y memoria | Sin asignaciones | Cero reservas, medidas con un asignador instrumentado, con cambios de modo en caliente |
| Procesador completo, 18 millones de comprobaciones | Sin NaN ni picos | 44,1 a 192 kHz, bloques de 1 a 4096, mono y estéreo, parámetros aleatorios con audio sonando |
| `auval -v aufx Poty Ferb` | Sin errores | Pasa. Un aviso conocido: la Q no «retiene» exactamente su valor por defecto por el redondeo de la escala logarítmica |

Aliasing de Buena (2×) y Mejor (4×): la meta de −70 y −80 dB de la ficha solo se cumple con Máxima. Si prefieres que la calidad por defecto cumpla −80 dB incluso en el peor caso, usa Máxima (cuesta 3,3 % de un núcleo).

**Valores iniciales que ya se fijaron:** estante de la cinta a 2 kHz con +3 dB (Q 0,7); `b = 0,25` del tubo; `k = 3` y `2` del germanio; escalas de intensidad ×0,6, ×1 y ×1,8; drive efectivo `1 + 19 · drive · escala`. Siguen siendo valores para afinar de oído.

**Presets de fábrica:** Brillo vocal (los valores por defecto), Calidez de bus, Aire y brillo, Graves con cuerpo y Presencia agresiva.
