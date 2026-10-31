# Visión general {.pb}

**Tufi** es un saturador armónico de sonido analógico. Dobla la curva de transferencia de la señal por zonas —nivel bajo, picos, y mitades positiva y negativa— para añadir calor y armónicos como lo haría una etapa de válvulas o de diodos, con controles de cantidad del 0 al 10 y dos perillas de ganancia (Entrada y Salida) para comparar a igual volumen.

Su referencia funcional es la familia de saturadores de curva por zonas (el RA II de Dave Hill Designs); el modelo matemático de Tufi es propio.

<div class="kpi">
<div><b>0,28 %</b><span>CPU de un núcleo, 4×, estéreo</span></div>
<div><b>−123 dB</b><span>Aliasing a 4× (9 kHz, −6 dBFS)</span></div>
<div><b>47 muestras</b><span>Latencia constante, 0,98 ms a 48 kHz</span></div>
<div><b>5</b><span>Presets de fábrica</span></div>
</div>

<figure class="shot"><img src="{{img}}/tufi.png" style="width:100%"><figcaption>Interfaz de Tufi (1080 × 600 px base; la ventana se redimensiona del 60 % al 200 %).</figcaption></figure>

## Características

- **Tres etapas** con carácter distinto: Triodo (domina el 2.º armónico), Pentodo (más ganancia y más impares) y Diodo (recorte seco).
- **Cuatro grupos de forma** independientes: Nivel bajo, Picos (Cantidad y Dureza) y Pares (Arriba y Abajo, que reparten la asimetría).
- **Transparente cuando se pide.** Con los controles de forma en 0 y Entrada y Salida en 0 dB, la curva es una recta exacta y la salida es la entrada, sin residuo.
- **Sin aliasing audible.** ADAA de primer orden sobre una tabla de la curva, más sobremuestreo seleccionable (Desactivado, 2×, 4× y 8×).
- **Sin clics** al mover cualquier control: todo está suavizado o con fundidos cortos.
- **Medidores analógicos:** osciloscopio con la curva real, contadores de pendiente, techo y asimetría, VU de aguja y una válvula que se enciende con la etapa activa.
- **Presets**, con guardar, guardar como, eliminar y navegación anterior y siguiente.

## Cuándo usarlo y cuándo no

Tufi sirve para dar cuerpo y presencia a pistas sueltas (voz, bajo, guitarras, sintetizadores) y para pegar un bus con saturación suave. No es un compresor ni un limitador: no tiene detector ni envolvente y no protege de picos. Es un procesador **estático**: la misma señal de entrada produce siempre la misma salida, sin importar lo que sonó antes.

# Instalación y compatibilidad

| Tema | Detalle |
| --- | --- |
| Sistema | macOS 11 o superior, binario universal (Apple Silicon e Intel) |
| Formatos | VST3 y AU (AUv2, tipo efecto) |
| Canales | Mono a mono y estéreo a estéreo; cada canal se procesa de forma independiente |
| Frecuencias de muestreo | 44,1 a 192 kHz, tamaño de bloque variable |
| Latencia | 47 muestras, constante en todos los modos |
| Fuera de alcance | Sidechain, MIDI, curva editable a mano, Windows, Linux |

## Dónde se instala

| Formato | Carpeta |
| --- | --- |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/Tufi.vst3` |
| Audio Unit | `~/Library/Audio/Plug-Ins/Components/Tufi.component` |
| Presets del usuario | `~/Library/Application Support/FERB/Tufi/Presets` |

Cada compilación copia el plugin a esas carpetas y lo firma «ad hoc». Si el paquete se lleva a otro equipo, macOS puede bloquearlo por no estar notarizado; se desbloquea con `xattr -dr com.apple.quarantine <paquete>`. Tras instalar o actualizar la versión de un AU, el anfitrión puede necesitar volver a escanear los plugins.

> **Aviso.** Tufi no está notarizado ni firmado con Developer ID. Está pensado para uso personal en el equipo donde se compila.

# Guía de uso {.pb}

## Primeros pasos

1. Inserta Tufi en una pista o bus. Con los ajustes de fábrica («Triodo cálido») ya hay un efecto moderado.
2. Elige la **etapa** con las tres teclas de la cabecera: Triodo, Pentodo o Diodo.
3. Sube **Entrada** para empujar la señal a la curva (más saturación) y usa **Salida** para volver al volumen original y comparar con el botón Bypass.
4. Ajusta Nivel bajo, Picos y Pares mirando el osciloscopio: la curva verde es la forma real que se aplica a la señal.

## Mapa de la interfaz

| Zona | Contenido |
| --- | --- |
| Cabecera | Nombre y placa **By FERB**; teclas de etapa con lámpara piloto; tecla de sobremuestreo; Bypass |
| Columna izquierda | Osciloscopio con la curva de transferencia y tres contadores: Pendiente, Techo y Asimetría |
| Columna central | Grupos Nivel bajo, Picos y Pares; perillas grandes Entrada y Salida; selector de presets |
| Columna derecha | VU de aguja con el nivel de salida y la ventana con la válvula de la etapa activa |

## Controles

| Control | Qué hace | Rango (defecto) |
| --- | --- | --- |
| **Etapa** | Elige la curva base: Triodo, Pentodo o Diodo | Triodo |
| **Entrada** | Ganancia antes de la curva. Más entrada, más saturación | −6 a +6 dB (0) |
| **Nivel bajo · Cantidad** | Realza la pendiente de la curva cerca de cero: da cuerpo a las señales suaves. A 0 no hay efecto | 0 a 10 (3,5) |
| **Picos · Cantidad** | Cuánto se mezcla el recorte suave de los picos. A 0 no hay efecto | 0 a 10 (5,5) |
| **Picos · Dureza** | Dónde empieza el recorte: techo de −3 dBFS (0) a −10 dBFS (10). Más dureza, recorte antes | 0 a 10 (4,0) |
| **Pares · Arriba** | Redondea la mitad positiva de la onda | 0 a 10 (3,0) |
| **Pares · Abajo** | Redondea la mitad negativa. Si Arriba y Abajo son iguales la curva es simétrica y solo hay armónicos impares | 0 a 10 (4,5) |
| **Salida** | Ganancia después de la curva, para igualar el nivel | −6 a +6 dB (0) |
| **Sobremuestreo** (tecla OS) | Cada pulsación recorre Desactivado, 2×, 4× y 8×. Más sobremuestreo, menos aliasing y algo más de CPU | 4× |
| **Bypass** | Deja pasar la señal original con un fundido de 10 ms, sin cambiar el tiempo | Apagado |

## Medidores

- **Osciloscopio:** dibuja la curva de transferencia real (200 puntos), con la diagonal de referencia punteada.
- **Pendiente:** ganancia de la curva en cero (por ejemplo, 1,35×).
- **Techo:** el umbral de los picos, según la Dureza (el valor por defecto es −6 dB).
- **Asimetría:** parte par frente a impar de la curva en ±1. En 0 % la curva es simétrica.
- **VU:** la aguja sigue el máximo del nivel de salida con balística de 300 ms. El 0 VU corresponde a −12 dBFS de pico; la zona roja va de 0 a +3.
- **Válvula:** se enciende con la etapa activa.

## Gestos del ratón

| Gesto | Perillas | Campos de valor bajo cada perilla |
| --- | --- | --- |
| Arrastrar | Cambia el valor (vertical u horizontal) | Arrastre vertical; con **Mayús** es más fino |
| Rueda | Cambia el valor | — |
| Doble clic | Vuelve al valor por defecto | Escribe un valor a mano |
| **Alt** + clic | — | Vuelve al valor por defecto |

La ventana se redimensiona arrastrando la esquina inferior derecha, del 60 % al 200 % con proporción fija. El tamaño se guarda con la sesión.

## Presets

El selector de la parte inferior tiene flechas para pasar al preset anterior o siguiente y un menú con los de fábrica, los del usuario y estas órdenes: **Guardar**, **Guardar como…**, **Eliminar** y **Mostrar carpeta de presets**. Un asterisco indica que se modificó el preset cargado. Un preset guarda los valores de todos los controles salvo el Bypass.

| Preset | Etapa | Entrada | Nivel bajo | Picos (cant. · dureza) | Pares (arriba · abajo) | Salida | OS |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Triodo cálido** (defecto) | Triodo | 0,0 | 3,5 | 5,5 · 4,0 | 3,0 · 4,5 | 0,0 | 4× |
| **Pentodo con mordida** | Pentodo | +2,0 | 2,0 | 7,0 · 6,0 | 4,0 · 2,0 | −1,5 | 4× |
| **Diodo seco** | Diodo | +3,0 | 1,0 | 8,0 · 7,0 | 2,0 · 2,0 | −2,5 | 4× |
| **Bus pegamento** | Triodo | +1,0 | 5,0 | 3,0 · 2,0 | 2,0 · 3,0 | −0,5 | 4× |
| **Transparente** | Triodo | 0,0 | 0,0 | 0,0 · 0,0 | 0,0 · 0,0 | 0,0 | 2× |

Los presets del usuario son archivos `.ferbpreset` (XML) en la carpeta indicada arriba; un preset que no menciona un parámetro lo deja en su valor por defecto, así los presets antiguos siguen sirviendo si el plugin gana controles.

## Consejos de uso

- **Compara a igual volumen.** Un efecto que sube el nivel parece mejor solo por sonar más fuerte. Baja Salida hasta que el nivel coincida con el Bypass.
- **Calidez en una pista:** Triodo, Entrada entre 0 y +3 dB, Pares con Arriba menor que Abajo (más 2.º armónico).
- **Solo impares (más «mordida»):** pon Pares · Arriba y Abajo en el mismo valor, o usa Diodo.
- **Pegar un bus:** valores bajos de Picos y Nivel bajo medio, como en «Bus pegamento».
- **Sin sobremuestreo** solo si la CPU apremia: el 4× cuesta menos del 0,3 % de un núcleo.

> **Nota.** Los valores de fábrica de las escalas de las etapas y de la forma son iniciales: están pensados para afinarse de oído. La sección de pendientes lo recoge.

# Referencia de parámetros

Todos los parámetros pasan por `AudioProcessorValueTreeState` con identificador de versión 1, son automatizables y se guardan en el estado del plugin. **No se renombran:** hacerlo rompe presets y sesiones guardadas.

| ID | Nombre en la interfaz | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `stage` | Etapa | Opciones | Triodo, Pentodo, Diodo | Triodo | Cambia la curva base |
| `drive` | Entrada | Decimal | −6 a +6 dB (paso 0,1) | 0 | Suavizado de 20 ms |
| `lowAmount` | Nivel bajo · Cantidad | Decimal | 0 a 10 (paso 0,1) | 3,5 | 0 = sin efecto |
| `peakAmount` | Picos · Cantidad | Decimal | 0 a 10 | 5,5 | 0 = sin efecto |
| `peakHardness` | Picos · Dureza | Decimal | 0 a 10 | 4,0 | Techo de −3 dBFS (0) a −10 dBFS (10) |
| `evenTop` | Pares · Arriba | Decimal | 0 a 10 | 3,0 | Redondeo de la mitad positiva |
| `evenBottom` | Pares · Abajo | Decimal | 0 a 10 | 4,5 | Redondeo de la mitad negativa |
| `trim` | Salida | Decimal | −6 a +6 dB (paso 0,1) | 0 | Suavizado de 20 ms |
| `oversample` | Sobremuestreo | Opciones | Desactivado, 2×, 4×, 8× | 4× | Cambiarlo hace un fundido de 5 ms |
| `bypass` | Bypass | Sí/No | apagado o encendido | Apagado | Es el bypass del anfitrión; no forma parte de los presets |

**Estado de la interfaz** (no automatizable, viaja con la sesión): `uiScale` (tamaño de la ventana) y `presetName` (preset actual).

# Documentación técnica

## Arquitectura de señal

{{fig:flujo}}

La señal pasa por esta cadena en cada canal, sin enlace estéreo:

1. **Entrada.** Ganancia `drive` en dB, suavizada 20 ms.
2. **Sobremuestreo** de 1× a 8× con etapas de media banda FIR propias (Kaiser, unos 100 dB de rechazo, fase lineal).
3. **Curva de transferencia.** Una tabla de consulta generada a partir de la etapa y de los controles de forma, con ADAA de primer orden.
4. **Submuestreo** a la frecuencia original.
5. **Filtro de continua.** Paso alto de primer orden a 10 Hz: la asimetría genera componente continua.
6. **Salida.** Ganancia `trim`, suavizada 20 ms.
7. **Medidor.** El nivel de pico de salida se publica en un `std::atomic<float>` que lee el VU.

Con los controles de forma en 0 y la entrada en 0 dB el bloque se procesa como ganancia simple y se salta la curva.

## Modelos de las etapas

Cada etapa define una curva base, calculada fuera del hilo de audio y guardada en una tabla de 4096 puntos para entradas de −2 a +2 (hasta +6 dBFS tras la entrada). Todas se normalizan a pendiente 1 en cero.

| Etapa | Carácter | Base matemática | Parámetros del modelo |
| --- | --- | --- | --- |
| Triodo | Curva suave, domina el 2.º armónico | Modelo de Koren de triodo con recta de carga | 12AX7, carga de 100 kΩ a 300 V, escala 2,5 |
| Pentodo | Más ganancia, rodilla marcada, más impares | Modelo de Koren de pentodo con rejilla pantalla | Tipo EL34, 2,5 kΩ a 400 V, escala 8 |
| Diodo | Recorte seco al llegar al codo | Ecuación de Shockley, par antiparalelo `2·Is·sinh(V/(n·Vt))` con resistencia en serie | Escala 0,5 |

{{fig:curvas}}

## Controles de forma

Los controles de forma se aplican sobre la curva base:

- **Nivel bajo:** término `k·x·exp(−x²/0,02)` con `k = Cantidad/10`; la pendiente en cero vale `1 + k`.
- **Picos:** mezcla entre la curva y un recorte suave por encima del umbral `T`.
- **Pares:** umbrales distintos por mitad.

<div class="formula">T = 10<sup>(−3 − 0,7·Dureza)/20</sup><br>T<sub>arriba</sub> = T·(1 − 0,4·Arriba/10) &nbsp;&nbsp; T<sub>abajo</sub> = T·(1 − 0,4·Abajo/10)</div>

```cpp
double lowLevel(double x, double k) { return x + k * x * std::exp(-x * x / 0.02); }

double peakSoft(double x, double T) {
    double a = std::abs(x);
    if (a <= T) return x;
    return std::copysign(T + (1 - T) * std::tanh((a - T) / (1 - T)), x);
}

double shape(double x, ...) {                 // x ya con la ganancia de entrada
    double y0   = lowLevel(x, lowSlope);
    double b    = lookup(stageTable, y0);     // curva de la etapa elegida
    double soft = peakSoft(b, b >= 0 ? Ttop : Tbottom);
    return y0 + peakMix * (soft - y0);        // peakMix = Picos · Cantidad / 10
}
```

La mezcla de picos parte de la señal con el nivel bajo aplicado, no de la curva de la etapa: así, con Picos · Cantidad y Nivel bajo en 0, la curva es una recta exacta y Tufi es transparente. La tabla que lee el hilo de audio muestrea `shape()` completa, de modo que el procesado por muestra es una sola consulta.

## ADAA y tabla de consulta

La curva se guarda como **lineal a tramos con 4096 puntos** y su **antiderivada exacta**. Eso permite ADAA de primer orden sin funciones trascendentes en el hilo de audio:

<div class="formula">y[n] = ( F(x[n]) − F(x[n−1]) ) / ( x[n] − x[n−1] )</div>

El error de la tabla frente a la curva continua es de unos 10<sup>−7</sup>. Cuando una perilla de forma cambia, la tabla de trabajo se acerca a la nueva en unos 15 ms, calculada en un hilo secundario y entregada sin bloqueos con triple búfer; la antiderivada de la muestra anterior se recalcula con la tabla nueva para evitar picos.

## Latencia

La latencia declarada al anfitrión es **constante, 47 muestras** (la del modo 8×), en todos los modos. Los modos con menos sobremuestreo retrasan lo que falta. Cambiar el sobremuestreo nunca cambia la latencia ni obliga al anfitrión a recalcular la compensación. El bypass retrasa la señal seca la misma cantidad, así que no cambia el tiempo.

| Frecuencia de muestreo | 44,1 kHz | 48 kHz | 96 kHz | 192 kHz |
| --- | --- | --- | --- | --- |
| Latencia (47 muestras) | 1,07 ms | 0,98 ms | 0,49 ms | 0,24 ms |

## Cambios sin clics

| Cambio | Mecanismo |
| --- | --- |
| Entrada, Salida | Suavizado de 20 ms |
| Controles de forma | La tabla se acerca a la nueva en ~15 ms |
| Sobremuestreo, o llevar la forma a 0 | Fundido de 5 ms hacia la ruta de ganancia simple, cambio, calentamiento y vuelta |
| Bypass | Fundido de 10 ms, con la señal seca retrasada lo mismo que la latencia |
| Etapa | Misma vía que los controles de forma: la tabla se acerca a la nueva (medido sin clics) |

## Reglas de tiempo real

- Sin asignaciones de memoria ni bloqueos en `processBlock`: todo se reserva en `prepare` para el máximo (8×). Lo vigila una prueba con un asignador instrumentado.
- `juce::ScopedNoDenormals` al inicio del bloque.
- Parámetros suavizados con `juce::SmoothedValue`, actualizados por bloque.
- Interfaz: un único temporizador a 30 Hz para el medidor y el osciloscopio.

## Estado y presets

El estado del plugin es el árbol de `AudioProcessorValueTreeState` más las propiedades de interfaz `uiScale` y `presetName`. Los presets guardan el valor real de cada parámetro en unidades de la interfaz; los de fábrica van en el código y los del usuario en archivos `.ferbpreset`.

## Estructura del código

| Carpeta | Contenido |
| --- | --- |
| `Tufi/source/dsp/` | `StageCurves` (curvas base), `ShapeCurve` y `ShapeTable` (forma, tabla y ADAA), `TufiEngine` (motor independiente del anfitrión) |
| `Tufi/source/ui/` | Perillas, osciloscopio, VU y válvula |
| `core/` | DSP común sin JUCE: sobremuestreador, ADAA, retardo, filtro de continua, medidor de nivel |
| `ui/` | Presets, escalado de ventana y campos de valor comunes |

El motor recibe una estructura de ajustes y no depende de JUCE ni del anfitrión, por lo que se prueba por sí solo.

# Calidad y verificación

Medidas a 48 kHz, estéreo, bloque de 256 y ajustes de fábrica salvo que se indique (28 de octubre de 2026).

| Prueba | Meta | Resultado |
| --- | --- | --- |
| Transparencia (forma en 0, 0 dB) | < −90 dBFS | Residuo nulo |
| Nivel bajo, pendiente en cero | `1 + cantidad/10` | 1,5 con cantidad 5; error < 1 % en todos los sobremuestreos |
| Armónicos por etapa (1 kHz, −6 dBFS) | Triodo con H2 dominante; diodo simétrico solo impares | Triodo H2 sobre H3 por más de 6 dB; pentodo con más H3 que el triodo; diodo con pares por debajo de −80 dB |
| Aliasing (9 kHz, −6 dBFS) | < −80 dB con sobremuestreo | Sin sobremuestreo −46 dB · 2× −59 dB · **4× −123 dB** · **8× −130 dB** |
| Componente continua, asimetría máxima | < −80 dBFS | Cumple |
| CPU, estéreo (% de un núcleo) | ≤ 2 % con 2× | Sin sobremuestreo 0,03 % · 2× 0,17 % · **4× 0,28 %** · 8× 0,45 % |
| Cambios de forma, etapa, sobremuestreo, paso por cero y bypass con audio sonando | Sin clics | Salto máximo entre muestras ≤ 1,4 veces el de la señal estable |
| `processBlock` y memoria | Sin asignaciones | Cero reservas (asignador instrumentado) |
| Procesador completo, 18 millones de comprobaciones | Sin NaN ni picos | 44,1 a 192 kHz, bloques de 1 a 4096, mono y estéreo, parámetros aleatorios con audio sonando |

> **Nota.** Con el sobremuestreo en 2× el aliasing medido es de −59 dB, por encima de la meta de −80 dB. Por eso el valor por defecto es 4×, que la cumple de sobra y cuesta menos de 0,3 % de un núcleo.

## Cómo repetir las medidas

```bash
ctest --test-dir build                         # todas las pruebas
./build/Tufi/TufiTests "[.report]"             # informe de aliasing, CPU y respuesta lineal
./build/Tufi/TufiProcessorTests                # procesador completo con editor
```

# Compatibilidad y validación

| Validador | Resultado |
| --- | --- |
| `auval -v aufx Tufi Ferb` | Pasa |
| `pluginval` nivel de rigor 10 (VST3 y AU) | Pasa |
| `TufiProcessorTests` | Pasa (rangos de tasa y bloque, estado, vida del editor, latencia, cero reservas en `processBlock`) |

Pendiente: comprobación manual en Logic Pro y Ableton Live (ver la sección siguiente).

# Limitaciones y pendientes

- **Sin prueba manual en anfitriones.** Los validadores y las pruebas automáticas pasan, pero falta comprobarlo a mano en Logic Pro y Ableton Live.
- **Valores iniciales por afinar de oído:** las escalas de las etapas (2,5 · 8 · 0,5) y los coeficientes 0,02 y 0,4 de la forma.
- **Solo macOS.** Sin Windows, Linux, AAX, CLAP, sidechain, MIDI ni curva editable a mano. El AAX compila con el SDK de Avid (2.8.0, arm64) pero va sin firmar (PACE), sin validar y sin probar en Pro Tools.
- **Distribución:** firma ad hoc, sin notarización. Antes de distribuirlo hay que revisar las condiciones de licencia de JUCE.

# Historial de versiones

| Versión | Fecha | Cambios |
| --- | --- | --- |
| **1.0.0** | {{fecha}} | Primera edición numerada. Reúne los hitos 1 a 9: tres etapas con controles de forma, ADAA y sobremuestreo hasta 8×, latencia constante, presets, interfaz redimensionable, osciloscopio, VU y válvula; sin clics y sin reservas en `processBlock`. |

El número de versión sigue SemVer y está en `cmake/Versions.cmake`: **mayor** si algo rompe presets o sesiones (IDs de parámetros, formato de estado), **menor** para funciones nuevas compatibles y **parche** para correcciones.

# Avisos {.ap}

- **Licencias de software.** Tufi está construido con JUCE 8.0.15 y las pruebas con Catch2. Las condiciones de licencia de JUCE deben revisarse antes de cualquier distribución.
- **Tipografías.** DM Serif Display, Josefin Sans y Courier Prime, incrustadas en el plugin bajo sus licencias abiertas (ver `Tufi/assets/fonts/LICENSE.md`).
- **Marcas de terceros.** Los nombres de productos de otras empresas se citan solo como referencia funcional y pertenecen a sus propietarios. FERB no está afiliado a ellas.
- **Uso.** Software de uso personal, entregado tal cual, sin garantía.
