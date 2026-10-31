# Visión general {.pb}

**Maru** quita el ruido de fondo constante (ventilador, aire acondicionado, siseo, zumbido de sala) de voces e instrumentos con **una sola perilla grande**, y suma un gate clásico opcional. Se ve como una unidad de rack analógica: frente verde salvia con orejas y tornillos, vúmetro de atenuación, palancas y lámpara de joya.

Su referencia funcional es el supresor de un solo control de Waves (NS1); de él toma la perilla única, el medidor de atenuación y las versiones mono y estéreo. El algoritmo es propio y no copia el de ningún producto.

<div class="kpi">
<div><b>1 perilla</b><span>Supresión: mueve umbral, reducción y agresividad</span></div>
<div><b>0,4 %</b><span>CPU de un núcleo, estéreo a 48 kHz</span></div>
<div><b>19</b><span>Parámetros automatizables</span></div>
<div><b>10</b><span>Presets de fábrica</span></div>
</div>

<figure class="shot"><img src="{{img}}/maru.png" style="width:100%"><figcaption>Interfaz de Maru (1260 × 300 px base; la ventana se redimensiona del 75 % al 200 %). Captura con una voz sintética con pausas y ruido de fondo.</figcaption></figure>

## Características

- **Dos motores.** *Supresión* (espectral, el motor por defecto) baja el ruido sin cortar la señal; *Gate* es una puerta con histéresis, hold y lookahead para amplificadores, batería y pausas limpias.
- **Un control manda.** La perilla *Supresión* (0 a 100 %) mueve a la vez el umbral, la reducción máxima y la agresividad. El resto es opcional.
- **El piso de ruido, a la vista.** La ventana de cristal muestra el espectro de entrada y de salida y una línea ámbar con el piso de ruido que aprende el estimador.
- **Escuchar lo quitado:** oyes la diferencia entre entrada y salida, para comprobar que no se pierde voz.
- **Transparencia antes que fuerza:** es preferible dejar algo de ruido que producir «ruido musical» o cortar el final de las palabras.
- **Barato en CPU:** sin redes neuronales. Un motor neuronal opcional queda como idea futura.
- Mono y estéreo, **enlace estéreo** de 0 a 100 %, **sidechain externo** (Gate), latencia **Alta** o **Baja**, presets, A/B, deshacer y rehacer.
- **Sin clics** al mover cualquier control y **cero reservas de memoria** en el hilo de audio.

## Cuándo usarlo y cuándo no

Maru sirve para ruido de fondo **estacionario**: ventilador, aire acondicionado, siseo de micrófono, zumbido de sala. **No** hace dereverberación, no quita clics ni ruidos impulsivos y no sigue bien un ruido muy cambiante (tráfico, público). Con voz continua el estimador sube su piso unos 7 dB (ver los límites): conviene tener pausas de al menos medio segundo en el material.

# Instalación y compatibilidad

| Tema | Detalle |
| --- | --- |
| Sistema | macOS 11 o superior, binario universal (Apple Silicon e Intel) |
| Formatos | VST3 y AU (AUv2, tipo efecto) |
| Canales | Mono y estéreo; entrada de sidechain opcional (apagada, mono o estéreo) |
| Frecuencias de muestreo | 44,1 a 192 kHz, tamaño de bloque variable |
| Latencia | Supresión: N muestras (1024 a 44,1 y 48 kHz, 21,3 ms; 512 con latencia Baja, 10,7 ms; el doble y el cuádruple a 96 y 192 kHz). Gate: su lookahead (0 a 10 ms). Informada al anfitrión |
| Fuera de alcance | Dereverberación, clics, ruido cambiante, surround, AudioSuite, redes neuronales |

## Dónde se instala

| Formato | Carpeta |
| --- | --- |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/Maru.vst3` |
| Audio Unit | `~/Library/Audio/Plug-Ins/Components/Maru.component` |
| Presets del usuario | `~/Library/Application Support/FERB/Maru/Presets` |

Cada compilación copia el plugin a esas carpetas y lo firma «ad hoc». Si el paquete se lleva a otro equipo, macOS puede bloquearlo por no estar notarizado; se desbloquea con `xattr -dr com.apple.quarantine <paquete>`. Tras actualizar la versión de un AU, el anfitrión puede necesitar volver a escanear los plugins.

> **Aviso.** Maru no está notarizado ni firmado con Developer ID. Está pensado para uso personal en el equipo donde se compila. El formato AAX (Pro Tools) compila con el SDK de Avid, pero va sin firmar (PACE) y sin validar ni probar: Pro Tools de venta no lo cargará.

# Guía de uso {.pb}

## Primeros pasos

1. Inserta Maru en la pista de voz o de instrumento, **antes** de la compresión.
2. Deja pasar unos segundos de **ruido solo** (una pausa): la ventana del piso de ruido y la ventana PISO se asientan. Con voz continua el piso se infla unos 7 dB.
3. Sube **Supresión** hasta que el ruido baje sin que se note en la voz. Con 50 % suele bastar; con ruido fuerte, 70 a 80 %.
4. Enciende **Escuchar lo quitado** (palanca QUITADO): debes oír el ruido y poca voz. Si oyes sílabas o respiraciones, baja la Supresión, la Sensibilidad o el Rango.
5. Para batería o amplificadores, cambia la palanca **MOTOR** a Gate.

## Mapa de la interfaz

| Zona | Contenido |
| --- | --- |
| Cabecera | Nombre y placa **BY FERB**; presets (anterior, nombre, siguiente), A/B, deshacer y rehacer; Bypass |
| Izquierda | Ventana de cristal del **piso de ruido**: barras de **entrada** (crema) y **salida** (verde pálido) por bandas, y la línea ámbar del piso. Con *Escuchar lo quitado* las barras muestran la diferencia en ámbar |
| Palancas | **MOTOR** (Supresión o Gate), **QUITADO** (escuchar lo quitado) y la perilla **ENLACE**. Con Gate aparecen además los interruptores **UMBRAL AUTO** y **SIDECHAIN EXT.** |
| Centro | Perilla grande **SUPRESIÓN** (0 a 100 %) |
| Fila de controles | Supresión: **Rango**, **Sensibilidad** y **Recuperación**. Gate: **Rango**, **Ataque**, **Hold**, **Histéresis** y **Release**, y una segunda fila con **Sensibilidad**, **Lookahead**, **SC HP**, **SC LP** y **Umbral** (manual; se atenúa con Auto) |
| Derecha | Vúmetro de **atenuación**, lámpara (*Trabajando* o *En reposo*; en Gate, *Puerta abierta* o *cerrada*), ventanas **PISO** y **REDUCC.**, y selector de **latencia** Baja o Alta |

## Controles principales

| Control | Qué hace | Rango (defecto) |
| --- | --- | --- |
| **Supresión** | Control principal: mueve a la vez la sobreestimación del ruido, la reducción máxima y el margen del Gate. 0 % = sin efecto | 0 a 100 % (50) |
| **Motor** | Supresión (espectral) o Gate | Supresión |
| **Rango** | Reducción máxima | −60 a −6 dB (−30) |
| **Sensibilidad** | Más positiva = más supresión (en Supresión sube la estimación del ruido; en Gate sube el margen) | −10 a +10 (0) |
| **Recuperación** | Cuánto tarda la supresión en volver tras la voz (en Gate es el release) | 10 a 1000 ms (120) |
| **Enlace** | 100 %: la misma ganancia en L y R (conserva la imagen); 0 %: cada canal por su lado | 0 a 100 % (100) |
| **Latencia** | Alta (N = 1024): más calidad; Baja (N = 512): la mitad de latencia | Alta |
| **Escuchar lo quitado** | Se oye la diferencia entre la entrada y la salida | Apagado |
| **Salida** | Ganancia final, con suavizado de 20 ms | −12 a +12 dB (0) |
| **Bypass** | El del anfitrión: deja pasar la entrada retrasada lo mismo que la latencia, con un fundido de 20 ms | Apagado |

## Mapeo del control principal

| Supresión | Sobreestimación β | Reducción máxima | Margen del Gate |
| --- | --- | --- | --- |
| 0 % | 1 | 0 dB (sin efecto) | 6 dB (el Gate no actúa) |
| 25 % | 1,5 | 6 dB | 10,5 dB |
| 50 % | 2 | 12 dB | 15 dB |
| 75 % | 2,5 | 21 dB | 19,5 dB |
| 100 % | 3 | Rango (−30 dB por defecto) | 24 dB |

La reducción máxima de la tabla se limita además por **Rango**. Son valores iniciales para afinar de oído.

## El Gate

| Control | Qué hace | Rango (defecto) |
| --- | --- | --- |
| **Umbral auto** | El umbral es el piso de ruido (medido en la banda del sidechain) más el margen de la tabla | Encendido |
| **Umbral** | Umbral manual, con Auto apagado | −80 a 0 dB (−50) |
| **Ataque** | Tiempo de apertura (rampa de coseno) | 0,1 a 50 ms (2) |
| **Hold** | Tiempo que sigue abierto tras bajar el nivel de umbral − histéresis | 0 a 500 ms (40) |
| **Histéresis** | Cierra por debajo de umbral − este valor | 0 a 12 dB (6) |
| **Release** (Recuperación) | Tiempo de cierre | 10 a 1000 ms (120) |
| **Lookahead** | Retrasa el audio, no el detector, para que la apertura llegue antes que el ataque. Suma esa latencia | 0 a 10 ms (3) |
| **SC HP / SC LP** | Filtros del detector (o del sidechain externo) | 20 a 2000 Hz (80) · 1 a 20 kHz (12 000) |
| **SIDECHAIN EXT.** | El detector escucha la entrada de sidechain del anfitrión | Apagado |

La reducción al cerrar es el **Rango**.

## Gestos y atajos

| Dónde | Gesto | Efecto |
| --- | --- | --- |
| Perillas | Arrastre vertical (190 px = todo el recorrido; **Mayús** afina) | Cambia el valor |
| Perillas | Rueda (**Mayús** afina) | Cambia el valor |
| Perillas | Doble clic | Valor por defecto |
| Perillas con foco | Flechas · Re Pág / Av Pág · Inicio / Fin | Paso fino · paso grande · mínimo y máximo |
| Selectores | Flechas | Elemento anterior o siguiente |
| A/B | **Mayús** + clic | Copia la ranura activa a la otra |

La ventana se redimensiona arrastrando la esquina inferior derecha, del 75 % al 200 % con proporción fija. El tamaño se guarda con la sesión.

## Presets, A/B y deshacer

El selector de la cabecera tiene flechas y un menú con los presets de fábrica, los del usuario y las órdenes **Guardar**, **Guardar como…**, **Eliminar** y **Mostrar carpeta de presets**. Un asterisco indica que el preset cargado se modificó. **A/B** guarda dos ranuras; **deshacer** y **rehacer** recorren los cambios. El A/B **no** se guarda con la sesión. Un preset guarda todos los controles salvo el Bypass.

| Preset | Ajustes |
| --- | --- |
| **Init** | Valores de fábrica (Supresión 50 %, Rango −30) |
| **Voz de podcast** | Supresión 55 %, Rango −30, Recuperación 120 ms |
| **Locución** | Supresión 40 %, Rango −24, Sensibilidad −2, Recuperación 200 ms |
| **Voz con ventilador** | Supresión 75 %, Rango −36, Sensibilidad +2, Recuperación 90 ms |
| **Zumbido de sala** | Supresión 65 %, Rango −32, Sensibilidad +1, Recuperación 150 ms |
| **Guitarra acústica** | Supresión 45 %, Rango −24, Recuperación 180 ms |
| **Amplificador (gate)** | Gate, Supresión 60 %, Rango −40, Recuperación 80 ms, Ataque 1 ms, Hold 80 ms, Histéresis 8 dB |
| **Batería (gate)** | Gate, Supresión 70 %, Rango −50, Sensibilidad +1, Recuperación 60 ms, Ataque 0,5 ms, Hold 30 ms |
| **Instrumento suave** | Supresión 30 %, Rango −18, Recuperación 250 ms |
| **Rescate fuerte** | Supresión 90 %, Rango −45, Sensibilidad +3, Recuperación 80 ms |

Los presets del usuario son archivos `.ferbpreset` (XML). Los cinco primeros con nombre de la maqueta (Voz de podcast, Locución, Voz con ventilador, Amplificador y Batería) usan sus valores; los otros son valores iniciales. Un preset carga solo los parámetros que menciona.

## Consejos de uso

- **Pausas:** el estimador aprende el piso en los silencios. Si la toma no tiene pausas de medio segundo, el piso estimado queda alto y la supresión, floja.
- **Voz sola:** Supresión de 40 a 60 % y Recuperación de 120 a 200 ms. Más recuperación = menos «bombeo», pero la cola del ruido tarda más en volver.
- **Comprueba con QUITADO:** si oyes consonantes en lo quitado, baja la Supresión o sube la Sensibilidad a valores negativos.
- **Latencia Baja** para monitorizar mientras grabas; **Alta** para mezcla.
- **Gate:** deja el umbral en Auto y sube la Supresión hasta que cierre entre golpes; ajusta Hold e Histéresis si «tartamudea».

> **Nota.** Los valores del mapeo y de los presets son iniciales: se afinan de oído.

# Referencia de parámetros

Todos los parámetros pasan por `AudioProcessorValueTreeState` con identificador de versión 1, son automatizables y se guardan en el estado. **No se renombran:** hacerlo rompe presets y sesiones guardadas. El bypass es el del anfitrión y no forma parte de los presets.

| ID | Nombre | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `amount` | Supresión | Decimal | 0 a 100 % | 50 | Control principal |
| `engine` | Motor | Lista | Supresión / Gate | Supresión | |
| `range` | Rango | Decimal | −60 a −6 dB | −30 | Reducción máxima |
| `sens` | Sensibilidad | Decimal | −10 a +10 | 0 | + = más supresión |
| `recovery` | Recuperación | Decimal | 10 a 1000 ms | 120 | Supresión: bajada lenta de la ganancia; Gate: release |
| `gateAuto` | Auto | Sí/No | | Encendido | Umbral = piso de ruido + margen |
| `gateThresh` | Umbral | Decimal | −80 a 0 dB | −50 | Solo Gate con Auto apagado |
| `attack` | Ataque | Decimal | 0,1 a 50 ms | 2 | Solo Gate |
| `hold` | Hold | Decimal | 0 a 500 ms | 40 | Solo Gate |
| `hyst` | Histéresis | Decimal | 0 a 12 dB | 6 | Solo Gate |
| `lookahead` | Lookahead | Decimal | 0 a 10 ms | 3 | Solo Gate; suma latencia |
| `scHP` | Sidechain HP | Decimal | 20 a 2000 Hz | 80 | Solo Gate |
| `scLP` | Sidechain LP | Decimal | 1 a 20 kHz | 12 000 | Solo Gate |
| `scExt` | Sidechain externo | Sí/No | | Apagado | Solo Gate |
| `link` | Enlace estéreo | Decimal | 0 a 100 % | 100 | |
| `latency` | Latencia | Lista | Baja / Alta | Alta | Cambia N de 512 a 1024 (a 48 kHz) |
| `listen` | Escuchar lo quitado | Sí/No | | Apagado | Diferencia entre entrada y salida |
| `output` | Salida | Decimal | −12 a +12 dB | 0 | |
| `bypass` | Bypass | Sí/No | | Apagado | Del anfitrión; fundido de 20 ms |

# Documentación técnica

## Arquitectura de señal

{{fig:flujo}}

El estimador de piso de ruido analiza la **suma mid** (o la señal, si es mono); la misma ganancia se aplica a L y R con enlace del 100 %, para no mover la imagen. Con enlace menor, cada canal tiene su propio estimador y ganancia, mezclados con la del mid.

## Estimador de piso de ruido (común a los dos motores)

- STFT con ventana raíz de Hann: N = 1024 muestras a 44,1 y 48 kHz (2048 a 96 kHz, 4096 a 192 kHz) y salto N/4, de modo que el salto dura 5,3 ms en todas las tasas. Latencia baja: N = 512 (a 48 kHz).
- **MCRA** (mínimos con probabilidad de presencia de voz, Cohen y Berdugo 2002): potencia suavizada 30 ms en el tiempo y 3 bins en frecuencia; mínimo de las tres últimas subventanas de 0,5 s (1 a 1,5 s); un bin cuenta como voz si su potencia suavizada pasa 5 veces el mínimo o si está 10 veces sobre el ruido estimado; la probabilidad de voz se suaviza (α = 0,2) y el ruido se promedia de forma recursiva (α = 0,95) solo donde no hay voz.
- El **piso mostrado** es la media cuadrática del espectro de ruido estimado, por Parseval con la ventana. Para el Gate se mide **en la banda del sidechain** (entre HP y LP).
- Límite conocido: la fuga de la voz que queda por debajo de 5 veces el ruido se toma como ruido, así que con voz continua el estimador se infla hasta unos 7 dB.

## Motor Supresión

- **Ganancia espectral** LSA de Ephraim y Malah con SNR a priori *decision-directed* (α = 0,98) y una sobreestimación del ruido β = 1 + 2·Supresión (más la Sensibilidad en dB). La ganancia mínima la fija la tabla de reducción por tramos (0, 6, 12, 21 dB y Rango).
- **Contra el ruido musical:** suavizado de la ganancia en frecuencia (3 bins) y en tiempo. Cuando aparece la señal, la ganancia sube rápido (2 ms) hacia valores altos; hacia valores bajos y al bajar, usa la constante de **Recuperación**, para que los picos aleatorios del ruido no abran el bin.
- **Transitorios:** un detector de flujo espectral sube el piso de ganancia 6 dB durante 30 ms y reduce el peso de la trama anterior en el *decision-directed* (de 0,98 a 0,3), para no ablandar consonantes ni pulsaciones.
- Reconstrucción por solapamiento y suma (WOLA, 75 % de solape). **Latencia = N muestras exactas.**
- *Escuchar lo quitado*: la salida es la entrada retrasada menos la salida procesada.

## Motor Gate

- **Detector:** paso alto y paso bajo de 2.º orden (SC HP y SC LP) y envolvente RMS de 3 ms, sobre el audio o el sidechain externo.
- **Umbral automático** = piso de ruido en la banda + 6 + 18·Supresión + Sensibilidad. Con Auto apagado, el umbral manual más la Sensibilidad. Durante los primeros 0,5 s de análisis el Gate queda abierto.
- **Histéresis, hold y release:** abre sobre el umbral; cierra cuando el nivel lleva *hold* ms por debajo de umbral − histéresis. La apertura y el cierre son rampas lineales en el tiempo (Ataque y Release) y la ganancia es un coseno elevado, sin clics. La reducción al cerrar es el Rango.
- **Lookahead:** el audio se retrasa; el detector no. La latencia es exactamente el lookahead.
- **Estéreo:** detector y estado sobre la suma mid y por canal; ganancia = enlace · mid + (1 − enlace) · propia.

## Latencia

La latencia es la del motor activo: **N muestras** en Supresión (1024 o 512 a 48 kHz) y el **lookahead** en Gate. **No es constante:** cambia con el motor, la calidad y el lookahead. El procesador la declara en cuanto cambian; el motor adopta el cambio con un fundido a silencio de 5 ms.

> **Nota.** Los anfitriones tratan de forma distinta los cambios de latencia durante la reproducción. Cambia de motor y de calidad con la reproducción parada.

## Cambios sin clics

| Cambio | Mecanismo |
| --- | --- |
| Supresión, Rango, Sensibilidad, Recuperación | La ganancia por bin se suaviza en tiempo y en frecuencia |
| Gate (umbral, ataque, hold, release…) | Rampa lineal en el tiempo con ganancia de coseno elevado |
| Salida, bypass, escuchar lo quitado | Suavizado de 20 ms, 20 ms y 10 ms (en doble precisión) |
| Motor y calidad | Fundido a silencio de 5 ms con `ModeSwitch` |

## Reglas de tiempo real

- Sin asignaciones de memoria ni bloqueos en `processBlock`: todo se reserva en `prepare` (las dos STFT, los retardos, los estimadores). Lo vigila una prueba con un asignador instrumentado.
- FFT con `juce::dsp::FFT` (Accelerate en macOS). `juce::ScopedNoDenormals` al inicio.
- La entrada con NaN o infinito se sustituye por 0 antes de entrar a los filtros.
- La pantalla recibe sus datos por telemetría del motor (32 bandas de entrada, salida y piso, el piso global y la reducción), sin FFT en la interfaz.

## Estado y presets

El estado es el árbol de `AudioProcessorValueTreeState` más las propiedades de interfaz (`uiScale`, nombre del preset). Los presets guardan el valor real de cada parámetro; los de fábrica van en el código y los del usuario en archivos `.ferbpreset` (XML). Al restaurar un estado, los Bool y Choice se sincronizan con `ferb::syncParametersFromState`.

## Estructura del código

| Carpeta | Contenido |
| --- | --- |
| `Maru/source/dsp/` | `MaruEngine`: estimador de ruido, ganancia espectral, STFT, Gate, telemetría |
| `Maru/source/ui/` | Ventana del piso de ruido y lámpara de joya |
| `Maru/tests/` | Pruebas del motor, del Gate y del procesador, señales sintéticas e informes (`[.report]`) y estudio con audio real (`[.real]`) |
| `core/`, `ui/`, `ui/vintage/` | DSP común, presets, A/B, escalado y el kit de interfaz «aparato analógico» compartido |

# Calidad y verificación

Medidas a 48 kHz, estéreo y bloque de 256 salvo que se diga. Las señales de prueba son **sintéticas** (ruido blanco, voz sintética con formantes, pulsaciones) más un estudio con grabaciones reales. Resultados del 31 de octubre de 2026.

| Prueba | Meta | Resultado |
| --- | --- | --- |
| Supresión 0 %: salida = entrada retrasada (todas las tasas, alta y baja latencia) | Idéntica | Por debajo de −110 dBFS |
| Latencia, impulso | Medida igual a la declarada | Exacta: N en Supresión y el lookahead en Gate |
| Estimador, ruido blanco de −70, −50 y −30 dBFS | ±1,5 dB en 3 s | Cumple a 44,1, 48 y 96 kHz |
| Estimador con voz sintética encima | — | Mediana a menos de 6 dB del piso real; **se infla hasta ≈ 7 dB** con voz continua |
| Reducción de ruido blanco a −40 dBFS, Rango 30 | ≥ 20 dB al 100 % | 0 / 5,5 / 11,4 / 18,7 / **21,4 dB** al 0 / 25 / 50 / 75 / 100 %. **No llega al Rango** (con Rango 60, ≈ 21,5 dB) |
| Voz sintética con ruido | La voz no baja más de 1 dB al 50 % | −0,02 dB; el ruido entre palabras baja más de 11 dB |
| Pulsación brillante de 8 ms sobre ruido | Pico dentro de 1,5 dB al 50 % | Cumple |
| Ruido musical (estadística de picos aislados del espectrograma) | Sin picos aislados | No más de 3 veces las del ruido original (más 0,2 %); **falta la escucha** |
| Gate: histéresis, hold, ataque, umbral automático, lookahead, sidechain externo, enlace | Según la ficha | Cumplen |
| Sin clics (Gate y cambio de motor) | ≤ 1,3 veces la señal | Cumple |
| Robustez | Sin NaN ni picos | Silencio, continua, NaN, infinitos y subnormales; 44,1 a 192 kHz; bloques de 1 a 4096; el resultado no depende del tamaño del bloque |
| `processBlock` | Sin reservas de memoria | 0 reservas (mono y estéreo, con motor y controles cambiando) |
| Procesador completo | Sin fallos | 18 millones de comprobaciones: tasas, bloques, canales, estado, editor (también sin `prepareToPlay`), latencia, presets, A/B y deshacer |
| **Audio real:** 5 voces cantadas, Supresión 50 % | La voz no se toca | Ganancia mediana −0,02 a −0,04 dB en los tramos cantados (peor ventana −1,0 a −1,6 dB, y −4,1 dB en una de los refuerzos); las pausas bajan 8 a 11 dB |
| **Audio real:** Supresión 80 % y Gate automático al 60 % | — | Pausas −13 a −20 dB (Supresión 80 %) y −30,0 dB (Gate); voz 0,00 a −0,05 dB |
| CPU Supresión estéreo (% de un núcleo) | < 3 % a 48 kHz | 44,1 kHz 0,37 · **48 kHz 0,41** (baja 0,40) · 96 kHz 0,82 · 192 kHz 1,65 |
| CPU Gate estéreo a 48 kHz | < 0,5 % | 0,41 % |

> **Nota.** El STOI de la ficha (pérdida de inteligibilidad menor de 0,03) **no se midió**: no hay una implementación en el repositorio. Como indicador de transparencia se usa la ganancia de la voz (−0,02 dB de mediana, en señales sintéticas y reales). Tampoco se midió contra el NS1.

> **Nota.** Las pistas reales no tienen un ruido de fondo continuo de ventilador sobre el que medir los 20 dB de la meta; esa cifra es con ruido blanco sintético.

## Cómo repetir las medidas

```bash
ctest --test-dir build                               # todas las pruebas
./build/Maru/MaruTests "[.report]"                   # informe: reducción por posición de la perilla, voz, CPU
./build/Maru/MaruProcessorTests                      # procesador completo con editor
MARU_REAL_FILE=<wav> MARU_REAL_OUT=<prefijo> ./build/Maru/MaruTests "[.real]"   # estudio con una grabación (CSV y audio procesado)
```

# Compatibilidad y validación

| Validador | Resultado |
| --- | --- |
| `auval -v aufx Maru Ferb` | Pasa |
| `pluginval` nivel de rigor 10 (VST3 y AU, con pruebas de interfaz) | Pasa |
| `MaruTests` y `MaruProcessorTests` (ejecutadas el 31 de octubre de 2026) | Pasan |

Pendiente: comprobación manual en Ableton Live, Logic Pro y Pro Tools.

# Limitaciones y pendientes

- **Sin escucha ni pruebas manuales.** Falta oír el resultado (ruido musical, burbujeo, cortes de colas) y probarlo en Ableton, Logic y Pro Tools.
- **Estimador con voz continua:** se infla hasta ≈ 7 dB mientras suena la voz y vuelve en las pausas; sin pausas de medio segundo la supresión es floja.
- **El Rango no se alcanza:** con el 100 % y Rango −60 el ruido blanco baja ≈ 21,5 dB, no 60.
- **STOI y comparación con NS1:** no medidos.
- **AAX (Pro Tools):** compila (SDK de AAX 2.8.0, probado en arm64) y se instala sin firmar; **sin validar** (falta el AAX Validator), **sin firma PACE** y **sin probar en Pro Tools**. Pro Tools de venta solo carga AAX firmados.
- **Latencia variable** con el motor, la calidad y el lookahead (a diferencia de Tufi, Poty y Cremita).
- **Valores iniciales por afinar de oído:** el mapeo de la perilla, el piso de ganancia, la protección de transitorios y los presets que no vienen de la maqueta.
- **Interfaz:** el enlace es una perilla (no la palanca de la maqueta) y el Gate tiene una segunda fila de controles que la maqueta no traía.
- **Fuera de alcance:** dereverberación, clics y ruidos impulsivos, ruido muy cambiante, redes neuronales (fase 2).
- **A/B** que no se guarda con la sesión.
- **Distribución:** firma ad hoc, sin notarización. Antes de distribuirlo hay que revisar las condiciones de licencia de JUCE.

# Historial de versiones

| Versión | Fecha | Cambios |
| --- | --- | --- |
| **1.0.0** | {{fecha}} | Primera edición numerada. Reúne los hitos M0 a M5: estimador de piso de ruido (MCRA), motor de Supresión espectral (STFT con LSA y decision-directed, protección de transitorios), motor Gate (umbral automático, histéresis, hold, lookahead, sidechain externo), 19 parámetros, 10 presets, A/B, deshacer e interfaz de rack con ventana del piso de ruido; sin clics y sin reservas en `processBlock`. |

El número de versión sigue SemVer y está en `cmake/Versions.cmake`: **mayor** si algo rompe presets o sesiones (IDs de parámetros, formato de estado), **menor** para funciones nuevas compatibles y **parche** para correcciones.

# Avisos {.ap}

- **Licencias de software.** Maru está construido con JUCE 8.0.15 y las pruebas con Catch2. Las condiciones de licencia de JUCE deben revisarse antes de cualquier distribución.
- **Tipografías.** Limelight, Josefin Sans y Oxanium, incrustadas en el plugin bajo sus licencias abiertas (ver `ui/vintage/fonts/LICENSE.md`).
- **Marcas de terceros.** Los nombres de productos de otras empresas se citan solo como referencia funcional y pertenecen a sus propietarios. FERB no está afiliado a ellas.
- **Uso.** Software de uso personal, entregado tal cual, sin garantía.
