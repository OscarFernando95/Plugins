# Visión general {.pb}

**Nel** es un limitador de masterización con **true peak**, medición de sonoridad completa (ITU-R BS.1770, con sonoridad integrada, rango y relación pico a sonoridad) e interfaz de aparato analógico. Sube el nivel de una mezcla contra un techo sin que ninguna muestra —ni ningún pico entre muestras— lo supere.

Su referencia funcional es el limitador de masterización de gama alta (el FabFilter Pro-L 2); de otros limitadores toma ideas concretas (knee ajustable, control de carácter, historial de sonoridad, A/B). El algoritmo es propio y no copia el de ningún producto.

<div class="kpi">
<div><b>8</b><span>Modos de limitación medibles</span></div>
<div><b>1× a 32×</b><span>Sobremuestreo, 4× por defecto</span></div>
<div><b>1,6 %</b><span>CPU de un núcleo, 4×, estéreo</span></div>
<div><b>16</b><span>Presets de fábrica</span></div>
</div>

<figure class="shot"><img src="{{img}}/nel.png" style="width:100%"><figcaption>Interfaz de Nel (1040 × 640 px base; la ventana se redimensiona del 75 % al 200 %).</figcaption></figure>

## Características

- **Techo de true peak** (−30 a 0 dBTP, −1,0 por defecto) con sobremuestreo ajustable de 1× a 32× y un seguro de pico con margen adaptativo.
- **Ocho modos** (Moderno, Transparente, Punchy, Dinámico, Versátil, Agresivo, Bus y Seguro): cada uno es un conjunto de tiempos de lookahead, ataque, release y knee que se puede medir y corregir.
- **Dos etapas:** una de picos con lookahead, que garantiza el techo, y una de cuerpo, que hace el trabajo lento para que la primera solo recorte lo que sobra.
- **Carácter:** saturación suave y opcional antes del limitador (0 a 100 %). Con 0 % no hace nada.
- **Medición de sonoridad** M, S, integrada, LRA y PLR, con objetivo de LUFS y diferencia en LU; historial de hasta 10 horas.
- **Escucha:** Ganancia unitaria (comparar a igual volumen) y Audición (oír solo lo que el limitador elimina).
- **Dither** de 16 a 24 bits con cuatro perfiles de noise shaping, y filtro de continua opcional.
- **Presets**, A/B, deshacer y rehacer; ventana redimensionable.
- **Sin clics** al mover cualquier control y **cero reservas de memoria** en el hilo de audio.

## Cuándo usarlo y cuándo no

Nel sirve para el último paso de una mezcla o masterización (llevarla a un nivel y un techo de true peak definidos) y para limitar buses con carácter. No es un compresor de mezcla, ni un limitador multibanda, y en esta versión no tiene sidechain externo, mid/side ni limitador de agudos. Para dar sonoridad **antes** del limitador, Cremita es el compañero natural: Cremita antes, Nel al final.

# Instalación y compatibilidad

| Tema | Detalle |
| --- | --- |
| Sistema | macOS 11 o superior, binario universal (Apple Silicon e Intel) |
| Formatos | VST3 y AU (AUv2, tipo efecto) |
| Canales | Mono y estéreo (en mono el enlace estéreo no hace nada) |
| Frecuencias de muestreo | 44,1 a 192 kHz, tamaño de bloque variable |
| Sobremuestreo efectivo | A 96 kHz o más el máximo baja a 8×; a 176,4 kHz o más, a 4×. La interfaz lo indica («SOBREMUESTREO · EFECTIVO 8X») |
| Latencia | Variable: sobremuestreo + lookahead, informada al anfitrión (ver la sección de documentación técnica) |
| Fuera de alcance | Sidechain externo, surround y Atmos, mid/side, multibanda, limitador de agudos, AudioSuite, funciones con IA |

## Dónde se instala

| Formato | Carpeta |
| --- | --- |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/Nel.vst3` |
| Audio Unit | `~/Library/Audio/Plug-Ins/Components/Nel.component` |
| Presets del usuario | `~/Library/Application Support/FERB/Nel/Presets` |

Cada compilación copia el plugin a esas carpetas y lo firma «ad hoc». Si el paquete se lleva a otro equipo, macOS puede bloquearlo por no estar notarizado; se desbloquea con `xattr -dr com.apple.quarantine <paquete>`. Tras actualizar la versión de un AU, el anfitrión puede necesitar volver a escanear los plugins.

> **Aviso.** Nel no está notarizado ni firmado con Developer ID. Está pensado para uso personal en el equipo donde se compila. El formato AAX (Pro Tools) compila con el SDK de Avid, pero va sin firmar (PACE) y sin validar ni probar: Pro Tools de venta no lo cargará.

# Guía de uso {.pb}

## Primeros pasos

1. Inserta Nel al final de la cadena de la pista o del bus de mezcla. Con el preset «Init» no hay limitación hasta que subas la Ganancia.
2. Ajusta el **Techo** (−1,0 dBTP es el valor habitual para producción y el que recoge EBU R 128).
3. Sube la **Ganancia** y mira el vúmetro de reducción y la pantalla: la reducción de ganancia en ámbar muestra cuánto trabaja el limitador.
4. Elige un **modo** según el carácter que busques y fija el **objetivo** de sonoridad (−9, −14, −16, −23 o −24 LUFS) para ver cuántas LU te faltan o te sobran.
5. Para comparar a igual volumen, enciende **Ganancia unitaria**; para oír qué elimina el limitador, enciende **Audición Δ**.

## Mapa de la interfaz

| Zona | Contenido |
| --- | --- |
| Cabecera | Nombre y placa **BY FERB**; presets (anterior, nombre, siguiente), **GUARDAR**, A/B, deshacer y rehacer; Bypass |
| Centro | Medidor de **entrada**, pantalla con entrada con ganancia, salida y reducción, y medidor de **salida**. Bajo la pantalla: GR ACTUAL, GR MÁX, OVERS y ESCALA |
| Derecha | Vúmetro de **reducción** con aguja y panel **LOUDNESS** (columnas M y S, sonoridad integrada, Δ al objetivo, LRA, PLR, TP MÁX, objetivos y reinicio) |
| Placa inferior | Perillas grandes Ganancia y Techo; ocho botones de modo; Lookahead, Ataque, Release, Knee y enlaces; Carácter; interruptores; fila de Sobremuestreo, Fase, Render y Dither |

## Controles principales

| Control | Qué hace | Rango (defecto) |
| --- | --- | --- |
| **Ganancia** | Empuja la señal contra el techo | 0 a +24 dB (0) |
| **Techo** | Máximo true peak de salida | −30 a 0 dBTP (−1,0) |
| **Modo** | Elige uno de los ocho modos; al hacer clic se escriben sus valores en Lookahead, Ataque, Release y Knee | Moderno |
| **Carácter** | Saturación suave previa al limitador | 0 a 100 % (0) |
| **TRUE PEAK** | Limita el true peak y no solo el pico de muestra. Con 1× o 2× el rótulo cambia a «TRUE PEAK ~» (aproximado) | Encendido |
| **GANANCIA UNIT.** | Compensa la salida con la ganancia aplicada, para comparar a igual volumen | Apagado |
| **AUDICIÓN Δ** | Se oye solo la diferencia entre entrada y salida: lo que el limitador elimina | Apagado |
| **FILTRO DC** | Paso alto de primer orden a 5 Hz antes del limitador | Apagado |
| **Bypass** | Deja pasar la señal original, con la latencia compensada y un fundido de 10 ms | Apagado |

## Los ocho modos

Cada modo fija cuatro valores visibles (que el usuario puede cambiar después) y varios internos. Si se modifica un valor, el nombre del modo muestra un asterisco hasta que se vuelve a elegir.

| Modo | Carácter buscado | Lookahead (ms) | Ataque (ms) | Release (ms) | Knee (dB) | Recorte previo |
| --- | --- | --- | --- | --- | --- | --- |
| **Moderno** | Mucha sonoridad sin ensuciar; el modo por defecto | 1,0 | 3 | 500 | 3 | 20 % |
| **Transparente** | Lo más cercano al original, sin bombeo | 3,0 | 10 | 1200 | 6 | 0 % |
| **Punchy** | Pegada; bombea al forzarlo | 1,0 | 25 | 300 | 1 | 10 % |
| **Dinámico** | Conserva y realza transitorios (+2 dB antes de limitar) | 2,0 | 6 | 700 | 4 | 0 % |
| **Versátil** | Equilibrio para casi cualquier material | 2,0 | 8 | 800 | 4 | 5 % |
| **Agresivo** | Casi recorte, máxima sonoridad | 0,2 | 1 | 250 | 0 | 45 % |
| **Bus** | Pegamento; no es transparente (detector con paso alto a 80 Hz y saturación fija del 10 %) | 1,5 | 20 | 600 | 8 | 0 % |
| **Seguro** | Sin distorsión audible, para material delicado | 5,0 | 15 | 2000 | 10 | 0 % |

> **Nota.** Los valores de los modos son iniciales: se afinan de oído. Elegir un modo con un clic en la interfaz escribe sus valores en los controles avanzados; la **automatización** del parámetro Modo solo cambia los valores internos, para que un preset o una automatización no pisen lo que hayas guardado.

## Tiempos y enlace

| Control | Qué hace | Rango (defecto) |
| --- | --- | --- |
| **Lookahead** | Anticipación de la etapa de picos. Por debajo de 0,1 ms se comporta casi como un recorte duro | 0 a 5 ms (1,0) |
| **Ataque** | Ataque de la etapa de cuerpo | 0,1 a 50 ms (3,0) |
| **Release** | Release lento (el rápido es el lento dividido entre 12) | 10 a 2000 ms (500) |
| **Release auto** | El release real varía entre el rápido y el lento según lo sostenida que sea la reducción. En pantalla, una «A» tras el valor | Encendido |
| **Knee** | Codo de la etapa de cuerpo | 0 a 12 dB (3,0) |
| **Enl. trans.** | Enlace de canales en la etapa de picos. 100 % = el canal más alto controla a los dos | 0 a 100 % (75) |
| **Enl. rel.** | Enlace de canales en el release de la etapa de cuerpo | 0 a 100 % (100) |

## Sobremuestreo, calidad y salida

| Control | Qué hace | Opciones (defecto) |
| --- | --- | --- |
| **Sobremuestreo** | Evita el aliasing del recorte y permite controlar los picos entre muestras | 1×, 2×, 4×, 8×, 16×, 32× (4×) |
| **Fase** | Lineal: más latencia. Mínima: latencia baja | Lineal, Mínima (Lineal) |
| **Render (+1 escalón)** | En render offline sube un escalón de sobremuestreo (4× pasa a 8×). Con 1× no se aplica | Encendido |
| **Dither** | Cuantiza a la profundidad elegida con dither triangular | NO, 16, 18, 20, 22, 24 bits (NO) |
| **Perfil** | Noise shaping del dither: Plano, Suave, Medio, Fuerte (activo solo con dither) | Plano |

| Factor | Uso previsto | CPU medida (estéreo, 48 kHz, fase lineal) |
| --- | --- | --- |
| 1× | Mezcla con latencia mínima; el true peak no queda garantizado | 0,65 % |
| 2× | Mezcla y buses | 1,1 % |
| 4× | Defecto; masterización y uso general | 1,6 % |
| 8× | Master con material muy comprimido | 2,6 % |
| 16× | Revisión final | 4,2 % |
| 32× | Render final | 7,5 % |

La CPU de la tabla está medida el 31 de octubre de 2026 en este equipo (la fase mínima cuesta lo mismo); depende de la carga del sistema.

## Medidores

| Medidor | Qué muestra |
| --- | --- |
| **Escaleras de entrada y salida** | 48 segmentos de −48 a 0 dB: pico por canal (sube al instante, cae 15 dB por segundo, retención de 2 s) y RMS más brillante dentro (ventana de 300 ms). Un clic en la lectura de RMS suma 3 dB y rotula «AES17» |
| **Pantalla** | Entrada con ganancia, salida y reducción de ganancia, con 6 s de historia y la línea del techo. Un clic en el pie derecho recorre 6 s en vivo, 10 min, 2 h y 10 h de historial |
| **GR ACTUAL / GR MÁX / OVERS** | Reducción instantánea y máxima retenida, y cuántas muestras de true peak superaron el techo |
| **ESCALA** | Un clic recorre ESTÁNDAR, K-12, K-14 y K-20 (las escalas K colocan el 0 en −12, −14 o −20 dBFS con RMS AES17) |
| **Vúmetro de reducción** | Aguja con la reducción de ganancia en dB y luz de máximo |
| **LOUDNESS** | M (400 ms) y S (3 s) en columnas de LED con escala en LU respecto del objetivo; INT. (sonoridad integrada en LUFS), Δ OBJ, LRA, PLR y TP MÁX. Teclas de objetivo (−9, −14, −16, −23, −24), RESET, selector SAL/ENT y lámpara EBU |

La sonoridad integrada y el LRA se reinician a la vez con RESET. Al cambiar la fuente (SAL o ENT) la medida se reinicia. Durante los primeros 60 s el LRA se muestra como no estable (`--`).

## Gestos del ratón

| Dónde | Gesto | Efecto |
| --- | --- | --- |
| Perillas | Arrastre vertical (190 px = todo el recorrido; **Mayús** afina) | Cambia el valor |
| Perillas | Rueda · doble clic | Cambia el valor · valor por defecto |
| Perillas con foco | Flechas · Re Pág / Av Pág · Inicio / Fin | Paso fino · paso grande · mínimo y máximo |
| Selectores y botones de modo | Flechas | Elemento anterior o siguiente |

La ventana se redimensiona arrastrando la esquina inferior derecha, del 75 % al 200 % con proporción fija. El tamaño se guarda con la sesión.

## Presets, A/B y deshacer

El selector de la cabecera tiene flechas y un menú que agrupa los presets de fábrica por categoría (Utilidad, Streaming, Master, Bus, Mezcla, Vintage, Broadcast y Voz), seguido de los del usuario y las órdenes **Guardar**, **Guardar como…**, **Eliminar** y **Mostrar carpeta de presets**. Un asterisco indica un preset modificado. **A/B** guarda dos ranuras; **deshacer** y **rehacer** recorren los cambios. El A/B **no** se guarda con la sesión. Un preset guarda todos los controles salvo el Bypass y no guarda el objetivo de sonoridad.

Cada preset de fábrica escribe los valores avanzados de su modo, así que suena como el modo que nombra. La ganancia depende del material: son un punto de partida.

| Preset | Categoría | Modo | Ajustes clave |
| --- | --- | --- | --- |
| **Init** | Utilidad | Moderno | Ganancia 0, techo −1,0 dBTP, 4× |
| **Streaming −14** | Streaming | Moderno | Ganancia +4, techo −1,0, 2× |
| **Streaming prudente** | Streaming | Seguro | Ganancia +2, techo −1,5 |
| **Master pop brillante** | Master | Dinámico | Ganancia +5, Carácter 8 % |
| **Master rock** | Master | Versátil | Ganancia +6 |
| **Master electrónica** | Master | Agresivo | Ganancia +8, techo −0,3, Carácter 12 % |
| **Master acústico** | Master | Seguro | Ganancia +2 |
| **Pegamento de bus** | Bus | Bus | Ganancia +3, techo −0,5, Carácter 20 % |
| **Batería con pegada** | Bus | Punchy | Ganancia +4, techo −0,5, enlace de transitorios 50 % |
| **Voz clara** | Mezcla | Transparente | Ganancia +3 |
| **Cinta cálida** | Vintage | Versátil | Ganancia +4, Carácter 45 % |
| **Válvula años 60** | Vintage | Punchy | Ganancia +5, Carácter 60 % |
| **Radio saturada** | Vintage | Agresivo | Ganancia +9, Carácter 80 % |
| **Broadcast R 128** | Broadcast | Transparente | Ganancia 0 (objetivo −23 LUFS: se elige a mano) |
| **Podcast −16** | Voz | Transparente | Ganancia +6 (objetivo −16 LUFS: se elige a mano) |
| **Render para 16 bits** | Utilidad | Moderno | 32×, dither de 16 bits con perfil Medio |

Los presets del usuario son archivos `.ferbpreset` (XML) en la carpeta indicada arriba.

## Consejos de uso

- **Techo de −1,0 dBTP** para entregas a streaming: deja margen para la codificación. Para broadcast con R 128, −1 dBTP y objetivo de −23 LUFS.
- **Compara a igual volumen.** Activa Ganancia unitaria antes de juzgar un cambio de modo o de Carácter.
- **Audición Δ** ayuda a oír si el limitador está quitando transitorios o graves: si lo que se oye es música y no solo picos, hay demasiada reducción.
- **Más sobremuestreo no siempre hace falta.** 4× es el valor general; sube a 8× o más para material muy comprimido o una revisión final, y usa «Render +1 escalón» al exportar.
- **Latencia mínima para grabar:** 1×, lookahead 0 y True Peak apagado (el true peak deja de estar garantizado).
- **Dither** solo en el último paso, al reducir a 16 o 24 bits, y una sola vez en la cadena.

> **Nota.** El lookahead, los tiempos y el Carácter de cada modo son valores iniciales para afinar de oído.

# Referencia de parámetros

Son **21 parámetros** automatizables. Todos pasan por `AudioProcessorValueTreeState` con identificador de versión 1 y se guardan en el estado. **No se renombran:** hacerlo rompe presets y sesiones guardadas.

## Principales

| ID | Nombre en la interfaz | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `gain` | Ganancia | Decimal | 0 a +24 dB | 0 | Suavizado de 20 ms |
| `ceiling` | Techo | Decimal | −30 a 0 dBTP | −1,0 | Suavizado de 20 ms |
| `mode` | Modo | Opciones | Moderno, Transparente, Punchy, Dinámico, Versátil, Agresivo, Bus, Seguro | Moderno | Por automatización solo cambia los valores internos |
| `character` | Carácter | Decimal | 0 a 100 % | 0 | Con 0 el bloque se salta |

## Avanzados

| ID | Nombre en la interfaz | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `lookahead` | Lookahead | Decimal | 0 a 5 ms | 1,0 (Moderno) | Se redondea a muestras enteras |
| `attack` | Ataque | Decimal | 0,1 a 50 ms | 3,0 (Moderno) | Escala logarítmica |
| `release` | Release | Decimal | 10 a 2000 ms | 500 (Moderno) | Escala logarítmica; el rápido es el lento / 12 |
| `releaseAuto` | Release automático | Sí/No | apagado o encendido | Encendido | |
| `knee` | Knee | Decimal | 0 a 12 dB | 3,0 (Moderno) | |
| `linkTransient` | Enlace de transitorios | Decimal | 0 a 100 % | 75 | 100 % = el canal más alto controla a los dos |
| `linkRelease` | Enlace de release | Decimal | 0 a 100 % | 100 | |

## Calidad y salida

| ID | Nombre en la interfaz | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `truePeak` | True Peak | Sí/No | apagado o encendido | Encendido | Limita el true peak y no solo el pico de muestra |
| `oversample` | Sobremuestreo | Opciones | 1×, 2×, 4×, 8×, 16×, 32× | 4× | Fundido de 50 ms al cambiarlo |
| `osPhase` | Fase del sobremuestreo | Opciones | Lineal, Mínima | Lineal | |
| `osOffline` | Más calidad en render | Sí/No | apagado o encendido | Encendido | +1 escalón en render offline |
| `ditherBits` | Dither | Opciones | Apagado, 16, 18, 20, 22, 24 bits | Apagado | |
| `ditherShape` | Perfil de dither | Opciones | Plano, Suave, Medio, Fuerte | Plano | |
| `dcFilter` | Filtro de DC | Sí/No | apagado o encendido | Apagado | Paso alto de 1.er orden a 5 Hz |

## Escucha y paso

| ID | Nombre en la interfaz | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `unityGain` | Ganancia unitaria | Sí/No | apagado o encendido | Apagado | Suavizado de 20 ms |
| `audition` | Audición | Sí/No | apagado o encendido | Apagado | Suavizado de 10 ms |
| `bypass` | Bypass | Sí/No | apagado o encendido | Apagado | Es el bypass del anfitrión; no forma parte de los presets |

## Estado de la interfaz

No son automatizables y viajan con la sesión: `uiScale` (tamaño de la ventana), `presetName`, `lufsTarget` (objetivo de sonoridad), `meterScale` (ESTÁNDAR, K-12, K-14, K-20), `loudnessSource` (salida o entrada), `rmsRef` (plana o AES17) y `scopeSpan` (escala de tiempo de la pantalla). Además existen `lufsScale` y `advancedOpen`, reservadas.

# Documentación técnica

## Arquitectura de señal

{{fig:flujo}}

Orden: **ganancia → [filtro de DC] → sobremuestreo → Carácter y recorte previo → etapa B (cuerpo, a la frecuencia base) → etapa A (picos, con lookahead) → seguro de pico → submuestreo → dither.**

- **Enlace estéreo:** el detector combina los canales con `linkTransient` en la etapa A y con `linkRelease` en la etapa B.
- **Audición:** emite la entrada retrasada menos la salida, para oír solo lo que se elimina.
- **Ganancia unitaria:** resta a la salida la ganancia aplicada.
- **Bypass:** cruza al audio original retrasado lo mismo que la latencia total.
- **Medidores:** los datos pasan a la interfaz por atómicos y colas sin bloqueos; el audio nunca espera a la interfaz.

## Etapa A: picos con lookahead

1. **Nivel requerido.** `p[n]` es el pico combinado de los canales (mezcla entre el máximo común y el de cada canal según el enlace de transitorios). La ganancia requerida es `g[n] = min(1, techoInterno / p[n])`.
2. **Mínimo móvil** de `g` sobre `N = lookahead · fsOS` muestras, con una cola monótona de coste constante por muestra.
3. **Suavizado** con dos medias móviles en cascada de longitudes `N/2 + 1` y `N/2`. Todos los valores de esa ventana son menores o iguales que la ganancia requerida de la muestra a la que se aplican: la ganancia suavizada **nunca supera la requerida** y no queda sobrepaso. Esto se comprueba contra una referencia de fuerza bruta.
4. **Aplicación.** El audio se retrasa y se multiplica por la ganancia suavizada.

Con un lookahead por debajo de 0,1 ms la etapa se comporta casi como un recorte duro; los modos que lo usan (Agresivo) se apoyan en el recorte previo y el sobremuestreo para limitar el aliasing. El lookahead se redondea a muestras base enteras.

## Etapa B: cuerpo

Va antes que la A y trabaja a la frecuencia base: toma el pico de cada grupo de muestras sobremuestreadas y la reducción resultante se interpola de vuelta. La reducción en dB sigue un codo suave de relación infinita:

<div class="formula">x = nivel del pico enlazado − (techo − margen del modo), en dB<br>reducción(x) = 0 si 2x < −knee &nbsp;·&nbsp; (x + knee/2)² / (2·knee) si 2|x| ≤ knee &nbsp;·&nbsp; x en otro caso</div>

{{fig:knee}}

La reducción se sigue con un ataque fijo y un release entre rápido y lento. Con el release automático, los picos aislados usan el rápido (el lento dividido entre 12) y una reducción sostenida se acerca al lento; el umbral de 3 dB de reducción sostenida y la constante de la media son valores iniciales.

## Recorte previo y Carácter

Ambos corren sobre la señal sobremuestreada y con **antialiasing ADAA de primer orden**:

- **Recorte previo (por modo):** una curva idéntica a la entrada hasta el 60 % del techo y, por encima, una curva tipo `tanh` con pendiente continua que se acerca al techo sin pasarlo; se mezcla según la cantidad del modo. Que sea idéntica por debajo es lo que permite la transparencia exacta.
- **Carácter:** saturación `y = x + c·(tanh(k·x)/k − x)` con `c` de 0 a 1 según el control y `k = 1 + 2c`. Con `c = 0` el bloque se salta.

Medido: sin ADAA el aliasing del modo Moderno a 4× era −73 dBc a 14 kHz y el de Agresivo −61; con él, −88 y −74.

## Seguro de pico

Un recorte duro al techo interno, a la frecuencia sobremuestreada. En marcha normal no actúa (la etapa A ya garantiza el techo); es una red de seguridad. El techo interno es el techo menos un **margen adaptativo**: empieza en 0,1 dB; un estimador de true peak de 4× a la salida cuenta los «overs» y, si aparece alguno, el margen sube 0,05 dB hasta un máximo de 0,5 dB y después baja despacio, 0,01 dB por segundo. Es una heurística; el contador OVERS lo muestra.

## Sobremuestreo

Etapas de media banda FIR propias (Kaiser, fase lineal), de 1× a 32×. La fase mínima usa los mismos FIR pasados a fase mínima con el cepstro. El **true peak que muestran los medidores** se calcula con su propio sobremuestreo de 4×, independiente del selector. Con «Render +1 escalón» activo y el anfitrión en modo no tiempo real, se sube un factor.

## Medición de sonoridad

Sigue ITU-R BS.1770:

1. **Filtro K** a la frecuencia base: un shelving de agudos de unos +4 dB (1682 Hz) y un paso alto de unos 38 Hz, con coeficientes derivados para cualquier frecuencia de muestreo.
2. **Energía por bloque:** bloques de 400 ms con 75 % de solape (paso de 100 ms); sonoridad del bloque `−0,691 + 10·log10(Σ z)`.
3. **M y S:** M es el bloque más reciente (400 ms) y S la media de los últimos 30 pasos (3 s), sin puerta.
4. **Integrada:** puerta absoluta en −70 LUFS y relativa 10 LU por debajo, con un histograma de 0,1 LU (memoria constante en sesiones largas).
5. **LRA:** distribución de los valores de corto plazo, con puertas de −70 LUFS y −20 LU y la diferencia entre los percentiles 95 y 10 (EBU Tech 3342).
6. **PLR:** true peak máximo menos sonoridad integrada.
7. **Calibración:** una senoidal estéreo de 1 kHz a −18 dBFS marca −18,0 LUFS.

El hilo de audio solo filtra y acumula la energía de cada paso de 100 ms; los histogramas y los percentiles se calculan en la interfaz. El **historial** de M, S y true peak se guarda en el procesador (sigue llenándose con el editor cerrado) con tres niveles de resolución decreciente: 1 s hasta 10 min, 10 s hasta 2 h y 60 s hasta 10 h (el promedio es en energía y el true peak conserva el máximo).

## Latencia

La latencia declarada al anfitrión es la del sobremuestreo más el lookahead, en muestras base enteras. A diferencia de Tufi, Poty y Cremita, **no es constante**: cambia con el factor de sobremuestreo, la fase y el lookahead. El procesador la declara cuando uno de ellos cambia, y el motor adopta la nueva estructura cuando el valor lleva 150 ms estable, con un fundido de 50 ms, para no encadenar fundidos al arrastrar el lookahead.

**Latencia del sobremuestreo a 48 kHz** (a la que se suma el lookahead en muestras: 1 ms = 48 muestras):

| Factor | 1× | 2× | 4× | 8× | 16× | 32× |
| --- | --- | --- | --- | --- | --- | --- |
| Fase lineal | 0 | 39 | 45 | 47 | 47 | 48 |
| Fase mínima | 0 | 4 | 5 | 5 | 6 | 6 |

Con **«Render +1 escalón»** (por defecto) la latencia declarada es la del escalón siguiente, para que no cambie entre el tiempo real y el render (los anfitriones no siempre releen la latencia a mitad de un bounce): por ejemplo, con 2× lineal se declaran 45 muestras en vez de 39. Con 1× el escalón no se aplica y la latencia sigue siendo solo el lookahead. Con el lookahead de Moderno (48 muestras) y los ajustes por defecto, la latencia declarada es de 95 muestras (1,98 ms). La latencia real medida con un impulso coincide con la informada: exacta en fase lineal y con ±1 muestra en fase mínima.

> **Nota.** Los anfitriones tratan de forma distinta los cambios de latencia durante la reproducción. Para grabar con monitorización, usa la combinación de latencia mínima (1×, lookahead 0, True Peak apagado).

## Cambios sin clics

| Cambio | Mecanismo |
| --- | --- |
| Ganancia, Techo, Carácter | Suavizado de 20 ms |
| Ganancia unitaria, Audición | Suavizado de 20 ms y 10 ms |
| Bypass | Fundido de 10 ms con la latencia compensada |
| Sobremuestreo, fase, lookahead | Fundido a silencio de 50 ms, tras 150 ms estable |
| Modo, preset | Los avanzados se escriben a la vez con suavizado; el cambio de estructura usa el fundido |

## Reglas de tiempo real

- Sin asignaciones de memoria ni bloqueos en `processBlock`: todo se reserva en `prepare` para el máximo (32×, lookahead de 5 ms). Lo vigila una prueba con un asignador instrumentado.
- `juce::ScopedNoDenormals` al inicio y parámetros suavizados con `juce::SmoothedValue`.
- Las medias móviles son sumas recursivas con acumulador en `double`, recalculadas cada pocos segundos para evitar la deriva.
- Interfaz: un único temporizador a 30 Hz para medidores y pantalla.

## Estado y presets

El estado es el árbol de `AudioProcessorValueTreeState` más las propiedades de interfaz. Los presets guardan el valor real de cada parámetro; los de fábrica van en el código y los del usuario en archivos `.ferbpreset` (XML). Al restaurar un estado, los parámetros Bool y Choice se sincronizan con `ferb::syncParametersFromState` (corregido en los cinco plugins del repositorio tras una prueba de `pluginval`).

## Estructura del código

| Carpeta | Contenido |
| --- | --- |
| `Nel/source/dsp/` | `NelEngine` (motor independiente del anfitrión), `PeakStage` (etapa A), `BodyStage` (etapa B), `Modes` (tabla de modos), `Dither` |
| `Nel/source/ui/` | Escaleras, pantalla, vúmetro de reducción, panel de sonoridad, selector de modo y tiras de lectura |
| `core/` | DSP común sin JUCE: sobremuestreador de 1× a 32×, medidor de sonoridad BS.1770, medidores de nivel, retardos, filtros |
| `ui/` y `ui/vintage/` | Presets, A/B, escalado y el kit de interfaz «aparato analógico» compartido |

# Calidad y verificación

Medidas a 48 kHz, estéreo y bloque de 256. Los resultados de la ficha son del 31 de octubre de 2026; la CPU y el aliasing de las dos últimas filas de abajo se repitieron el mismo día con el informe `[.report]`.

| Prueba | Meta | Resultado |
| --- | --- | --- |
| Techo −1 dBTP con +24 dB, ocho modos y cuatro señales (ruido de programa, senoidales de 1 y 15 kHz, impulsos), 4× | Ningún over | Peor true peak −0,998 dBTP medido a 4× (el estándar); −0,92 dBTP con un medidor de 8×, más estricto |
| Techo con 1× (pico de muestra) y con 32× | Ningún over | Cumple |
| Transparencia (ganancia 0, señal bajo el techo, Carácter 0) | < −120 dBFS | 1×: < −130 dBFS. Con sobremuestreo: < −90 dBFS (rizado de los filtros) |
| Latencia, impulso | Medida igual a la informada | Exacta en fase lineal; ±1 muestra en fase mínima, en todos los factores y lookaheads |
| Sobremuestreador | Banda de paso plana; imágenes < −80 dB | ±0,05 dB (lineal) y ±0,15 dB (mínima) hasta 17 kHz; imágenes cumplen en todos los factores y fases |
| Aliasing, senoidal 6 dB sobre el techo, 10 a 18 kHz | −80 dBc con 4× | Con 4×, Moderno llega a −80 dBc o mejor hasta 16 kHz y −78,4 dBc a 18 kHz; Agresivo −72 dBc hasta 16 kHz y −69,7 dBc a 18 kHz. Con 8× todos los modos cumplen (peor caso −90 dBc) |
| Lookahead | La reducción llega antes del pico | Verificado de 0,5 a 5 ms con 1× y 4×: al llegar el pico, la ganancia ya está en el techo sin pasarlo |
| True peak del medidor | Dentro de +0,2 / −0,4 dB | Cumple con senoidales a fs/4 con fase de 45° de −12 a −1,5 dBTP (el pico de muestra lee 3 dB menos) |
| Historial de sonoridad | Hasta 10 h | Probado con 11 h simuladas: memoria fija y el máximo de true peak se conserva |
| Sin clics (14 controles y cambios de estructura) | ≤ 1,3 veces la señal estable | Cumple |
| Robustez | Sin NaN ni picos | Silencio, continua, NaN, infinitos y subnormales; 44,1 a 192 kHz; bloques de 1 a 4096 |
| `processBlock` | Sin reservas de memoria | 0 reservas (mono y estéreo, parámetros aleatorios) |
| Procesador completo | Sin fallos | 18 millones de comprobaciones: tasas, bloques, canales, estado, editor, latencia declarada, presets, A/B y deshacer |
| CPU (31 oct 2026, % de un núcleo, fase lineal) | ≤ 4 % con 4× | 1× 0,65 % · 2× 1,1 % · **4× 1,6 %** · 8× 2,6 % · 16× 4,2 % · 32× 7,5 % |
| Aliasing, informe del 31 oct 2026 (6 dB sobre el techo) | — | Moderno a 14 kHz: 1× −38 · 2× −80 · **4× −88** · 8× −96 dBc. Agresivo a 14 kHz: 1× −31 · 2× −69 · **4× −74** · 8× −91 dBc |

> **Nota.** La meta de −80 dBc de aliasing con 4× no se cumple en el peor caso (Agresivo y frecuencias de 16 a 18 kHz); con 8× sí. El resto es rizado de ganancia que cualquier limitador muestreado produce al ver los picos en una rejilla. En la ficha la CPU a 4× se había medido en 1,1 %; la medida de hoy es mayor, aún muy por debajo de la meta.

> **Nota.** La lectura de los medidores se verificó con señales sintéticas construidas con la misma idea de las pruebas de la EBU (Tech 3341 y 3342): senoidal estéreo de 1 kHz a −18, −23 y −33 dBFS, puertas con tramos de −36 y −23 LUFS, dos niveles separados 10 LU para el LRA y senoidales a fs/4 con fase de 45° para el true peak. **No** se contrastó con los archivos oficiales de la EBU, que no se pudieron descargar.

## Cómo repetir las medidas

```bash
ctest --test-dir build                          # todas las pruebas
./build/Nel/NelTests "[.report]"                # informe de aliasing y CPU
./build/Nel/NelProcessorTests                   # procesador completo con editor
```

# Compatibilidad y validación

| Validador | Resultado |
| --- | --- |
| `auval -v aufx Nel1 Ferb` | Pasa |
| `pluginval` nivel de rigor 10 (VST3 y AU, con pruebas de interfaz) | Pasa |
| `NelTests` y `NelProcessorTests` (ejecutadas el 31 de octubre de 2026) | Pasan |

Pendiente: comprobación manual en Ableton Live, Logic Pro y Pro Tools.

# Limitaciones y pendientes

- **Sin prueba manual en anfitriones.** Falta probarlo a mano en Ableton, Logic y Pro Tools, y ver cómo cada uno trata los cambios de latencia.
- **AAX (Pro Tools):** compila (SDK de AAX 2.8.0, probado en arm64) y se instala sin firmar; **sin validar** (falta el AAX Validator), **sin firma PACE** y **sin probar en Pro Tools**. Pro Tools de venta solo carga AAX firmados.
- **Latencia variable.** Cambia con el sobremuestreo, la fase y el lookahead (a diferencia de Tufi, Poty y Cremita).
- **Aliasing con 4×** por encima de la meta en el peor caso (ver la nota de calidad).
- **Mediciones de sonoridad** verificadas con señales sintéticas, no con los archivos oficiales de la EBU.
- **Valores iniciales por afinar de oído:** los parámetros de los modos, los perfiles de dither, el umbral del release automático y el margen del seguro de pico.
- **Escalas de medidor:** solo ESTÁNDAR, K-12, K-14 y K-20 (la escalera siempre abarca −48 a 0 dB); no hay las escalas estándar de −16 y −32 dB.
- **Presets de usuario** sin categorías (los de fábrica sí van agrupados) y **A/B** que no se guarda con la sesión.
- **Fuera de alcance:** sidechain externo, surround, mid/side, multibanda, limitador de agudos, AudioSuite, IA.
- **Distribución:** firma ad hoc, sin notarización. Antes de distribuirlo hay que revisar las condiciones de licencia de JUCE.

# Historial de versiones

| Versión | Fecha | Cambios |
| --- | --- | --- |
| **1.0.0** | {{fecha}} | Primera edición numerada. Reúne los hitos 1 a 14: limitador de dos etapas con ocho modos, sobremuestreo de 1× a 32× con fase lineal o mínima, seguro de pico, Carácter y recorte previo con ADAA, dither, medición de sonoridad con historial de 10 h, 21 parámetros, 16 presets, A/B, deshacer e interfaz vintage; sin clics y sin reservas en `processBlock`. |

El número de versión sigue SemVer y está en `cmake/Versions.cmake`: **mayor** si algo rompe presets o sesiones (IDs de parámetros, formato de estado), **menor** para funciones nuevas compatibles y **parche** para correcciones.

# Avisos {.ap}

- **Licencias de software.** Nel está construido con JUCE 8.0.15 y las pruebas con Catch2. Las condiciones de licencia de JUCE deben revisarse antes de cualquier distribución.
- **Tipografías.** Limelight, Josefin Sans y Oxanium, incrustadas en el plugin bajo sus licencias abiertas (ver `ui/vintage/fonts/LICENSE.md`).
- **Marcas de terceros.** Los nombres de productos de otras empresas se citan solo como referencia funcional y pertenecen a sus propietarios. FERB no está afiliado a ellas.
- **Normas.** Las mediciones siguen las recomendaciones ITU-R BS.1770 y las especificaciones EBU R 128 y Tech 3341 y 3342; Nel no está certificado por ninguna de esas entidades.
- **Uso.** Software de uso personal, entregado tal cual, sin garantía.
