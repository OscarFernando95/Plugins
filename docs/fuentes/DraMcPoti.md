# Visión general {.pb}

**Dra. McPoti** es un excitador armónico multibanda y dinámico. Reúne en un solo aparato el sonido de los excitadores clásicos (un generador de armónicos que se mezcla en paralelo con la señal intacta) y lo que exige hoy el mastering: sobremuestreo, dinámica por banda, estéreo mono-compatible y compensación de sonoridad. Se ve como una unidad de rack de válvulas: placa gris pizarra con orejas de acero, perillas de baquelita negra con la escala grabada, palancas cromadas, cuatro indicadores de válvula que brillan con lo que se añade (a media luz con Bypass) y dos vúmetros de aguja.

Un interruptor elige el **mundo** (Vocal o Master), otro la **era** (Vintage o Moderno) y un selector de cuatro posiciones el **carácter** dentro de la era: ocho caracteres en total, disponibles en los dos modos.

<div class="kpi">
<div><b>4 bandas</b><span>Cuerpo, Medios, Presencia y Aire</span></div>
<div><b>8</b><span>Caracteres: 4 Vintage y 4 Moderno</span></div>
<div><b>44</b><span>Parámetros automatizables</span></div>
<div><b>21</b><span>Presets de fábrica (20 más Init)</span></div>
</div>

<figure class="shot"><img src="{{img}}/dramcpoti.png" style="width:100%"><figcaption>Interfaz implementada de Dra. McPoti (1400 × 600 px base; la ventana se redimensiona del 75 % al 200 %). Captura real con el preset «Radio setentas» y unos segundos de audio de prueba.</figcaption></figure>

<figure class="shot"><img src="{{img}}/dramcpoti-maqueta.png" style="width:100%"><figcaption>Maqueta aprobada de la interfaz (<code>Dra. McPoti — maqueta.html</code>, 1400 × 540 px), que es la referencia visual. <b>La implementación sigue su disposición y sus piezas (palancas de tuerca hexagonal, selector rotativo de Voicing, válvulas de vidrio con su placa, perillas con escala 0 a 10, vúmetros de marco negro), con las diferencias menores que se detallan en las limitaciones.</figcaption></figure>

## Características

- **Armónicos creados, no solo realzados.** Un pasa-altos alimenta un generador no lineal, el resultado se vuelve a filtrar y se suma en paralelo a la señal original. El seco nunca se procesa.
- **Dinámica por banda.** Cada banda mide su nivel y ajusta su drive: con Dinámica positiva aporta más en pasajes suaves (apertura sin dureza); con negativa, más en los picos (pegada).
- **Ocho caracteres.** Vintage: Clásico, Cinta, Válvula y Transistor (generan armónicos). Moderno: Dinámico (compander), Adaptativo (Válvula guiada por el balance tonal), Seda (shelf paralelo) y Aire (shelf que se aplana al comprimir).
- **Guarda de sibilancia** (Vocal): baja Presencia y Aire hasta 12 dB cuando detecta una «s».
- **Master:** Ancho del lateral de lo añadido (0 a 200 %) y Mono seguro (lo añadido bajo 150 Hz va en mono).
- **Cruce de fase lineal** opcional (FIR de 2049 coeficientes) o Linkwitz-Riley de 4.º orden por defecto.
- **Sobremuestreo de 1× a 8×** con ADAA de primer orden: el aliasing de peor caso queda en −96,5 dB a 4× y 8×.
- **Medición honesta:** Delta (escuchar solo lo añadido), Solo por banda, vúmetros de entrada y salida, ventana de armónicos e **Igualar** (compensación de sonoridad K de 3 s, ±12 dB) para comparar sin que engañe el volumen.
- **Sin clics** al mover cualquier control y **cero reservas de memoria** en el hilo de audio.

## Cuándo usarlo y cuándo no

Sirve para dar brillo, aire o pegada a voces, mezclas y masters, y para corregir material apagado. **No** es un de-esser (la guarda solo evita que el brillo exagere las eses), ni un ecualizador (no resta nada), ni un limitador. Con Drive alto y sin sobremuestreo hay aliasing audible: use 4× o más con material delicado.

# Instalación y compatibilidad

| Tema | Detalle |
| --- | --- |
| Sistema | macOS 11 o superior, binario universal (Apple Silicon e Intel) |
| Formatos | VST3 y AU (AUv2, tipo efecto). AAX compila pero sin validar, sin firmar y sin probar |
| Canales | Mono y estéreo (Ancho y Mono seguro solo actúan en estéreo) |
| Frecuencias de muestreo | 44,1 a 192 kHz, tamaño de bloque variable |
| Latencia | **47 muestras** (≈ 1,0 ms a 48 kHz) con cualquier sobremuestreo y carácter; con **Fase lineal** suma 1024 y queda en **1071** (≈ 22,3 ms). Informada al anfitrión |
| Fuera de alcance | Surround, AudioSuite, procesamiento de sibilancia como de-esser |

## Dónde se instala

| Formato | Carpeta |
| --- | --- |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/Dra. McPoti.vst3` |
| Audio Unit | `~/Library/Audio/Plug-Ins/Components/Dra. McPoti.component` |
| Presets del usuario | `~/Library/Application Support/FERB/DraMcPoti/Presets` (subcarpetas Vocal y Master) |

Cada compilación copia el plugin a esas carpetas y lo firma «ad hoc». Si el paquete se lleva a otro equipo, macOS puede bloquearlo por no estar notarizado; se desbloquea con `xattr -dr com.apple.quarantine <paquete>`. Tras actualizar la versión de un AU, el anfitrión puede necesitar volver a escanear los plugins.

> **Aviso.** Dra. McPoti no está notarizado ni firmado con Developer ID. Está pensado para uso personal en el equipo donde se compila. El formato AAX (Pro Tools) compila con el SDK de Avid, pero va sin firmar (PACE) y sin validar ni probar: Pro Tools de venta no lo cargará.

# Guía de uso {.pb}

## Primeros pasos

1. Cargue un preset (Vocal o Master) o parta de **Init**.
2. Elija el **Modo** (Vocal o Master) y la **Era** con las palancas, y el **carácter** (1 a 4) con el selector rotativo; los nombres cambian con la era.
3. Suba el **Drive** de las bandas que quiera excitar. Con **Delta** escucha solo lo añadido; con **Solo** oye una banda.
4. Active **Igualar** (por defecto está activo) para comparar con y sin el plugin al mismo volumen.

## Mapa de la interfaz

| Zona | Contenido |
| --- | --- |
| Cabecera | Logo, presets (anterior, nombre, siguiente, Guardar), A/B, deshacer, rehacer, joya de «Encendido» y Bypass del anfitrión |
| Izquierda | Palancas de Modo (Vocal / Master) y de Era (Vintage / Moderno), selector rotativo de Voicing, Entrada, Mezcla y Salida |
| Centro | Cuatro módulos de banda: válvula de vidrio, placa con el nombre y el rango, perilla grande de Drive, Dinámica, Nivel, ON y Solo; los tres cruces entre los módulos; la ventana de armónicos debajo |
| Derecha | Vúmetros de entrada y salida; Sintonización, Fase, Color, Suavizar, Guarda, Ancho, Esquina de aire, Enlace y Referencia VU |
| Pie | Oversampling (1× a 8×), palancas de Fase lineal, Delta, Igualar y Mono seguro, y los chips de tamaño (75, 100, 150 y 200 %) |

Lo que no actúa en el modo o carácter actual se ve atenuado: la **Guarda** en Master; **Ancho** y **Mono seguro** en Vocal; **Esquina de aire** salvo con el carácter Aire.

## Los ocho caracteres

| Era | Carácter | Qué hace | Armónicos |
| --- | --- | --- | --- |
| Vintage | **Clásico** | tanh asimétrico con un término par acotado | 2.º y 3.º, brillo inmediato |
| Vintage | **Cinta** | pre-énfasis de +6 dB desde 5 kHz, tanh, de-énfasis | Impares suaves |
| Vintage | **Válvula** | triodo asimétrico con polarización | Pares dominantes, cálido |
| Vintage | **Transistor** | recorte suave simétrico `u/√(1+u²)` | Impares, brillante y firme |
| Moderno | **Dinámico** | compander por banda: suma una copia comprimida (2:1 a 8:1) | Ninguno; realza el detalle suave |
| Moderno | **Adaptativo** | Válvula con el drive guiado por la razón Presencia/Medios (promedio de 2 s, ±6 dB) | Pares |
| Moderno | **Seda** | shelf de +15 dB en el borde inferior de cada banda, sumado a la señal intacta | Ninguno (solo realce) |
| Moderno | **Aire** | shelf de +10 dB en la esquina de aire que se aplana al comprimir | Ninguno |

En Clásico, Cinta, Válvula, Transistor y Adaptativo, **Drive** es la cantidad de armónicos (0 a 100). En Dinámico es la cantidad de la copia comprimida y **Dinámica** fija su relación. En Seda y Aire, Drive mezcla el realce del shelf (100 % = el shelf completo).

## Controles principales

- **Sintonización** (1,5 a 12 kHz): pasa-altos del que se alimenta el generador; en cada banda vale el mayor entre Sintonización y el borde inferior. Cuerpo la ignora.
- **Fase** (0 a 100 %): de cero a cuatro secciones pasa-todo (600, 1800, 5000 y 11000 Hz) solo en lo añadido, con transición continua.
- **Color** (−100 a +100): hacia −100 favorece los pares (cálido); hacia +100, los impares (presente).
- **Suavizar** (4 a 20 kHz): corte superior de lo añadido; a 20 kHz está apagado.
- **Guarda** (0 a 100 %, solo Vocal): reduce hasta 12 dB × Guarda la excitación de Presencia y Aire ante una sibilancia.
- **Ancho** (0 a 200 %) y **Mono seguro** (solo Master, estéreo).
- **Enlace** (0 a 100 %): enlace estéreo de las envolventes.
- **Cruces 1, 2 y 3:** 80 a 400 Hz, 600 a 4000 Hz y 3 a 12 kHz, con razón mínima 1,5 entre ellos.
- **Oversampling:** 1×, 2×, 4× (por defecto) y 8×.

## Modo Vocal y modo Master

Al cambiar de modo, los controles que siguen en el valor de la plantilla del modo anterior pasan a la del nuevo; los que usted movió se respetan.

| Control | Vocal | Master |
| --- | --- | --- |
| Sintonización | 4,0 kHz | 5,0 kHz |
| Guarda | 60 % | 0 % (no actúa) |
| Mono seguro | off | on |
| Cruces 1 / 2 / 3 | 150 / 1800 / 7000 Hz | 120 / 1600 / 6500 Hz |
| Referencia de dinámica | −24 dBFS | −18 dBFS |

Los valores de las plantillas son iniciales y se afinan de oído.

## Gestos y atajos

Perillas: arrastre vertical (Mayús para ajuste fino), rueda, doble clic para escribir el valor, Alt + clic para el valor por defecto, flechas, Re Pág/Av Pág, Inicio y Fin.

## Presets, A/B y deshacer

Los 20 presets de fábrica más **Init** son de solo lectura y aparecen también como programas del anfitrión. Los del usuario se guardan en las subcarpetas Vocal y Master. **A/B** guarda dos estados completos y copiar A a B es un clic; deshacer y rehacer recorren los cambios de parámetros. El bypass es el del anfitrión y no entra en los presets.

## Consejos de uso

- Para comparar, deje **Igualar** activo: así el volumen no decide.
- Con Delta oirá qué añade cada banda; con Solo, la banda completa excitada.
- En voces con muchas eses, suba **Guarda** antes de bajar el Drive de Presencia y Aire.
- Para material delicado o Drive alto, use **4×** u **8×**; con 2× el aliasing de peor caso queda en −61 dB.
- Con **Fase lineal** el cruce es más selectivo pero la latencia sube a ≈ 22 ms: use el modo normal al grabar.

# Referencia de parámetros

Todos los parámetros pasan por `AudioProcessorValueTreeState` con identificador de versión 1, son automatizables y se guardan en el estado. **No se renombran:** hacerlo rompe presets y sesiones guardadas. Las bandas son Lo (Cuerpo), Md (Medios), Hi (Presencia) y Ar (Aire). El bypass es el del anfitrión y no forma parte de los presets.

| ID | Nombre | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `mode` | Modo | Lista | Vocal, Master | Vocal | Cambia la plantilla de valores |
| `era` | Era | Lista | Vintage, Moderno | Vintage | |
| `voice` | Voicing | Lista | 1 a 4 | 1 | Carácter dentro de la era |
| `input` | Entrada | Decimal | −12 a +12 dB | 0 | Antes del cruce |
| `output` | Salida | Decimal | −12 a +12 dB | 0 | Después de todo |
| `mix` | Mezcla | Decimal | 0 a 100 % | 100 | Cantidad global de lo añadido |
| `tune` | Sintonización | Decimal | 1,5 a 12 kHz | 4 kHz | Pasa-altos del generador |
| `phase` | Fase | Decimal | 0 a 100 % | 40 | Red pasa-todo de lo añadido |
| `color` | Color | Decimal | −100 a +100 | 0 | Pares (−) a impares (+) |
| `soften` | Suavizar | Decimal | 4 a 20 kHz | 20 kHz | 20 kHz = apagado |
| `airCorner` | Esquina de aire | Decimal | 8 a 40 kHz | 16 kHz | Solo con el carácter Aire; se acota a 0,45 × la frecuencia de muestreo |
| `guard` | Guarda | Decimal | 0 a 100 % | 60 | Solo Vocal |
| `width` | Ancho | Decimal | 0 a 200 % | 100 | Solo Master, estéreo |
| `monoSafe` | Mono seguro | Interruptor | off, on | off | Solo Master, estéreo |
| `link` | Enlace | Decimal | 0 a 100 % | 100 | Enlace estéreo de las envolventes |
| `xo1` | Cruce 1 | Decimal | 80 a 400 Hz | 150 | Cuerpo / Medios |
| `xo2` | Cruce 2 | Decimal | 600 a 4000 Hz | 1800 | Medios / Presencia |
| `xo3` | Cruce 3 | Decimal | 3 a 12 kHz | 7000 | Presencia / Aire |
| `driveLo`, `driveMd`, `driveHi`, `driveAr` | Drive | Decimal | 0 a 100 | 20, 25, 40, 35 | Significado según el carácter |
| `dynLo`, `dynMd`, `dynHi`, `dynAr` | Dinámica | Decimal | −50 a +50 | 0 | − más en picos; + más en pasajes suaves; en Dinámico, relación 2:1 a 8:1 |
| `mixLo`, `mixMd`, `mixHi`, `mixAr` | Nivel de banda | Decimal | 0 a 100 % | 100 | |
| `onLo`, `onMd`, `onHi`, `onAr` | Activar banda | Interruptor | off, on | on | |
| `soloLo`, `soloMd`, `soloHi`, `soloAr` | Solo banda | Interruptor | off, on | off | Escucha la banda excitada |
| `os` | Oversampling | Lista | 1×, 2×, 4×, 8× | 4× | |
| `linPhase` | Fase lineal | Interruptor | off, on | off | Cruce FIR; suma 1024 muestras de latencia |
| `delta` | Delta | Interruptor | off, on | off | Escucha solo lo añadido |
| `match` | Igualar | Interruptor | off, on | on | Sonoridad K de 3 s, ±12 dB |
| `vuRef` | Referencia VU | Decimal | −24 a −12 dBFS | −18 | 0 VU en dBFS |
| `bypass` | Bypass | Interruptor | off, on | off | El del anfitrión |

# Documentación técnica

## Arquitectura de señal

La señal se divide en cuatro bandas; cada banda crea y modela sus armónicos, y lo añadido se suma en paralelo a un seco con **el mismo retardo**. El seco nunca se procesa:

`y[n] = x[n − D] + Mezcla · Σ (nivel de banda · añadido de la banda)`

D es la latencia declarada. El generador de cada banda es `HP → (sobremuestreo ↑) → no linealidad con ADAA → HP → (↓)`; Dinámico, Seda y Aire no usan generador no lineal y trabajan a la frecuencia base. Después de sumar las bandas, lo añadido pasa por Suavizar, por la red de Fase y, en Master estéreo, por Ancho y Mono seguro.

## Cruce

- **Linkwitz-Riley de 4.º orden** por defecto: se parte en el cruce 2 y cada mitad en el 1 y en el 3, con el paso-todo del cruce vecino en cada banda para que las cuatro sumen con magnitud plana. Medido: ±0,1 dB a 44,1, 48 y 96 kHz con tres juegos de cruces.
- **Fase lineal:** tres paso-bajos FIR de 2049 coeficientes (sinc con ventana de Blackman) en los tres cruces; las bandas son diferencias de paso-bajos y la última es la señal retardada menos el paso-bajo más alto, así que **suman exactamente la entrada retardada 1024 muestras** (medido: residuo < −100 dB). La transición del filtro es de ≈ 129 Hz a 48 kHz, así que con cruces muy bajos el reparto es menos nítido que con LR4.
- Los cruces y la Sintonización se suavizan 30 ms; con Fase lineal la FIR se rediseña como mucho cada 128 muestras mientras se mueven.

## Dinámica por banda

Cada banda mide su nivel (RMS con ataque de 5 ms y relajación de 120 ms) y ajusta su drive: `a = Drive/100 · 2^(−Dinámica · e / 50)`, con `e = clamp((L − Lref)/12, −1, 1)`. Lref es −24 dBFS en Vocal y −18 dBFS en Master. El nivel se enlaza entre canales según Enlace.

## Generadores y ADAA

Núcleo común: `h = HP(f_v(a_k · HP(x_b)))`. Las no linealidades `f_v` de los cuatro caracteres Vintage tienen antiderivada cerrada, así que todas usan **ADAA de primer orden** además del sobremuestreo. Color mueve el punto de trabajo (pares) o mezcla con la parte impar `(f(u) − f(−u))/2` (impares). Cinta añade un shelf de pre-énfasis de +6 dB desde 5 kHz antes y su inverso después, a la frecuencia sobremuestreada.

## Guarda de sibilancia

Detector propio y simplificado: compara la energía de 5 a 10 kHz con la de 300 Hz a 3 kHz (envolvente de 2 ms) con histéresis (se activa si la razón supera 0,5 y se suelta bajo 0,25). Cuando hay una «s» baja Presencia y Aire hasta 12 dB × Guarda, con ataque de 1 ms y relajación de 40 ms. El detector mira la entrada sin retrasar y lo añadido llega con la latencia del plugin, así que anticipa ≈ 1 ms.

## Igualar y medidores

Dos integradores de 3 s con ponderación K (la de `core/Loudness.h`) miden la entrada y la salida; la ganancia (±12 dB) lleva la salida a la sonoridad de la entrada, se suaviza y se apaga en Delta y Solo. Los vúmetros usan la balística de VU (65 ms, calibrada a RMS) dentro del motor; 0 VU es la Referencia VU.

## Latencia

Constante en 47 muestras con cualquier sobremuestreo y carácter (se declara la de 8× y los factores menores retrasan lo que falta; el seco y Solo se retrasan igual). **La Fase lineal es la única excepción:** suma 1024. Al activarla o desactivarla se funde toda la salida (5 ms), se realinean los retardos y se mantiene el silencio hasta que el seco vuelve a llenarse (≈ 25 ms). La latencia declarada coincide con la medida con un impulso para 1×, 4× y 8×, con y sin Fase lineal.

## Cambios sin clics

Los parámetros continuos se suavizan 20 ms. Cambiar el sobremuestreo o el carácter funde lo añadido a silencio (5 ms), reinicia filtros y ADAA, y vuelve. Cambiar la Fase lineal funde toda la salida. Hay una prueba con parámetros aleatorios con el audio sonando.

## Reglas de tiempo real

Sin asignaciones ni bloqueos en `processBlock`: todo se reserva en `prepareToPlay` (y los anillos de muestras en el constructor, porque un editor puede abrirse antes). `juce::ScopedNoDenormals` al inicio. Los punteros a los parámetros se resuelven una vez: buscarlos por texto en el hilo de audio reservaba memoria (lo detectó la prueba de cero reservas).

## Estado y presets

Los presets de fábrica van incrustados y son de solo lectura; los del usuario se guardan como XML del árbol de parámetros. El estado de sesión es el árbol completo; una sesión restaurada no recibe plantilla de modo.

## Estructura del código

`DraMcPoti/source/dsp/` (`McPotiEngine`, `LinearSplit`), `source/Parameters`, `source/Presets`, `source/PluginProcessor`, `source/PluginEditor`; `core/` aporta `Crossover`, `OversamplerN`, `Biquad`, `DelayLine`, `Loudness`, `VuBallistics`, `SampleRing`; `ui/vintage/` aporta los controles y vúmetros.

# Calidad y verificación

Medidas del 31 de octubre de 2026, Release arm64, 48 kHz, salvo que se indique. Reproducibles con `./build/DraMcPoti/DraMcPotiTests` (todas las pruebas) y `./build/DraMcPoti/DraMcPotiTests "[.report]"` (informes de CPU).

| Qué | Condición | Resultado |
| --- | --- | --- |
| Cruce LR4: suma | Senoides de 40 Hz a 15 kHz, tres juegos de cruces | ±0,1 dB |
| Cruce de fase lineal: suma | Ruido blanco | Entrada retardada 1024 muestras, residuo < −100 dB |
| Drive 0 o Mezcla 0 % | Ruido | Salida = entrada con la latencia declarada, < −90 dB |
| Seco + Delta | Ruido, Drive 70 | = salida completa, < −90 dB |
| Armónicos | Seno de 3 kHz a −18 dBFS, Drive 100 | Válvula: 2.º > 3.º; Transistor: 3.º > 4 × el 2.º; Clásico: ambos |
| **Aliasing, peor caso** | Senos de 5,0, 7,9 y 9,1 kHz a 0,5, Drive 100, los cuatro caracteres, hasta 20 kHz | 1×: **−41,6 dB**; 2×: **−61,1 dB**; 4× y 8×: **−96,5 dB** |
| Latencia | Impulso, 1× a 8×, con y sin Fase lineal | Coincide con la declarada |
| Salida acotada | Ruido a +12 dB, Drive 100, Dinámica extrema; cambios aleatorios de todo | Sin NaN y sin picos desbocados |
| Conmutaciones | Sobremuestreo, carácter y Fase lineal con audio | Sin clics (salto máximo ≤ 2 × el de la señal estable) |
| Guarda | Seno de 7 kHz y vocal de 800 Hz | La sibilancia baja más de 9 dB; la vocal no cambia (±5 %) |
| Igualar | Ruido, Drive 100 | Diferencia entrada–salida dentro de 1 dB (sin él, mayor) |
| Ancho y Mono seguro | Estéreo en Master | Ancho 0 %: lateral nulo; 200 %: lo duplica; Mono seguro: lateral a 60 Hz < 20 % |
| CPU de un núcleo | Estéreo, bloques de 256, Drive de fábrica | 1×: ≈ 0,8 %; 2×: ≈ 2,4 %; 4×: ≈ 3,5 %; 8×: ≈ 5,4 %; 4× con Fase lineal: ≈ 11,6 % |

Las pruebas automáticas (`DraMcPotiTests` y `DraMcPotiProcessorTests`) incluyen el procesador completo con editor: parámetros aleatorios, tasas de 44,1 a 192 kHz, bloques de 1 a 4096, mono y estéreo, estado, vida del editor y cero reservas en `processBlock`. **Lo medido no sustituye a la escucha:** son medidas de dirección y de orden de magnitud con señales sintéticas; los coeficientes y umbrales son valores iniciales.

# Compatibilidad y validación

| Validador | Resultado |
| --- | --- |
| `auval -v aufx McPo Ferb` | Pasa |
| `pluginval` nivel de rigor 10 (VST3 y AU, con pruebas de interfaz) | Pasa |
| `DraMcPotiTests` y `DraMcPotiProcessorTests` (ejecutadas el 31 de octubre de 2026) | Pasan |

Pendiente: comprobación manual en Ableton Live, Logic Pro y Pro Tools.

# Limitaciones y pendientes

- **Sin escucha ni pruebas manuales.** Falta oír los caracteres, la guarda y los presets, y probarlo en Ableton, Logic y Pro Tools. Todos los coeficientes (escalas de Drive, γ = 0,5 de lo añadido, esquinas de Seda y Aire, umbrales de la guarda, plantillas de modo, objetivo tonal de Adaptativo) son valores iniciales.
- **Aliasing a 2×:** −61,1 dB en el peor caso, por debajo de la meta de −70 dB. Por eso el sobremuestreo por defecto es 4×. Un ADAA de segundo orden queda descartado (la segunda antiderivada de tanh no tiene forma cerrada).
- **Aire:** trabaja a la frecuencia base y la esquina se acota a 0,45 × fs, así que las esquinas de 22 a 40 kHz no hacen lo que dice su rango. El 5 % de armónicos pares de la ficha original no está (a la frecuencia base crearía aliasing).
- **Dinámico:** el umbral del compander es fijo (−20 dB); la ficha original pedía que lo fijara Drive.
- **Guarda:** detector simplificado y sin planitud espectral; lookahead de ≈ 1 ms, no 2 ms. Sin afinar con voz real.
- **Fase lineal:** la FIR es fija (2049 coeficientes) a cualquier frecuencia de muestreo; su transición es de ≈ 516 Hz a 192 kHz. Cambia la latencia (+1024 muestras).
- **Interfaz: sigue la maqueta, con diferencias menores.** Coinciden la paleta pizarra con orejas de acero, la cabecera con presets, Guardar, A/B, deshacer y la joya roja de «Encendido»; las palancas cromadas de tuerca hexagonal (Modo, Era, Fase lineal, Delta, Igualar y Mono seguro); el selector rotativo de Voicing; los cuatro módulos con válvula de vidrio, placa con el nombre y el rango de frecuencias, Drive, Dinámica y Nivel, y las perillas de cruce entre ellos; las perillas negras con la escala de 0 a 10; los dos vúmetros con marco negro y la ventana de armónicos. **Todavía distinto:** el logotipo usa la tipografía del kit y no la cursiva de la maqueta; la pantalla de presets no lleva el número en ámbar («01 · …») ni el botón «A→B»; la ventana de armónicos está en el centro inferior y no arriba a la derecha, y no lleva el título «ARMÓNICOS · modo · era»; los tubos de vidrio son más sencillos (sin la rejilla de la base); falta la lectura de latencia al pie; el rotulado de Drive no cambia a «Ganancia del shelf» con Seda y Aire; las válvulas miden en dB absolutos, no relativos a la entrada de la banda; y el vúmetro de salida no indica si Igualar está activo.
- **Igualar:** integrador exponencial de 3 s; no se probó contra el criterio de 0,5 LU.
- **AAX (Pro Tools):** compila (SDK de AAX 2.8.0, arm64) y se instala sin firmar; **sin validar**, **sin firma PACE** y **sin probar en Pro Tools**.
- **Distribución:** firma ad hoc, sin notarización. Antes de distribuirlo hay que revisar las condiciones de licencia de JUCE.

# Historial de versiones

| Versión | Fecha | Cambios |
| --- | --- | --- |
| **0.9.0** | {{fecha}} | Edición preliminar. Motor completo: cruce LR4 y de fase lineal, ocho caracteres con ADAA y sobremuestreo de 1× a 8×, dinámica por banda, Color, Fase, Suavizar, guarda de sibilancia, Ancho y Mono seguro, Igualar, plantilla por modo; 44 parámetros, 21 presets, A/B, deshacer, vúmetros y ventana de armónicos. Interfaz aún provisional. |

El número de versión sigue SemVer y está en `cmake/Versions.cmake`: **mayor** si algo rompe presets o sesiones (IDs de parámetros, formato de estado), **menor** para funciones nuevas compatibles y **parche** para correcciones. La versión 1.0.0 llegará al cerrar la interfaz y escuchar el plugin.

# Avisos {.ap}

- **Licencias de software.** Dra. McPoti está construido con JUCE 8.0.15 y las pruebas con Catch2. Las condiciones de licencia de JUCE deben revisarse antes de cualquier distribución.
- **Tipografías.** Limelight, Josefin Sans y Oxanium, incrustadas en el plugin bajo sus licencias abiertas (ver `ui/vintage/fonts/LICENSE.md`).
- **Marcas de terceros.** Los nombres de productos de otras empresas (Aphex, Slate Digital, SPL, Kush Audio, Maag, iZotope, Waves, Oeksound, Universal Audio) se citan solo como referencia funcional y pertenecen a sus propietarios. FERB no está afiliado a ellas; Dra. McPoti toma sus principios, no su código.
- **Uso.** Software de uso personal, entregado tal cual, sin garantía.
