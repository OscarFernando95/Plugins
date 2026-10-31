# Visión general {.pb}

**ChauS** es un de-esser con EQ dinámico que sigue a la sibilante. En vez de bajar el volumen de toda la voz, baja **solo la banda de la «s»**, y la pantalla enseña dónde y cuánto actúa: el espectro de la voz, la campana que se mueve con cada «s» y, abajo, una línea de tiempo donde cada sibilante queda marcada.

Su referencia funcional es el de-esser de gama alta de FabFilter (Pro-DS); de otros productos toma ideas concretas (umbral que se adapta a la interpretación, sibilantes suaves y duras, corte dinámico de banda). El algoritmo es propio y no copia el de ningún producto.

<div class="kpi">
<div><b>0 a 15 ms</b><span>Lookahead, con la latencia informada</span></div>
<div><b>0,6 %</b><span>CPU de un núcleo, estéreo a 48 kHz</span></div>
<div><b>24</b><span>Parámetros automatizables</span></div>
<div><b>10</b><span>Presets de fábrica</span></div>
</div>

<figure class="shot"><img src="{{img}}/chaus.png" style="width:100%"><figcaption>Interfaz de ChauS (960 × 580 px base; la ventana se redimensiona del 75 % al 200 %). Captura con una voz sintética con eses.</figcaption></figure>

## Características

- **Detecta sibilantes, no solo agudos.** Un clasificador por rasgos espectrales (seis, ver la documentación técnica) separa una «s» de un brillo natural de la voz.
- **Sigue la s.** La campana dinámica rastrea la frecuencia de cada sibilante (normalmente entre 5 y 10 kHz, distinta en cada cantante), con un ancho que se calcula solo o se fija.
- **Umbral adaptativo.** Aprende el nivel típico de las eses de esa voz (percentil 90 de los últimos 5 s), así el mismo ajuste sirve para frases suaves y fuertes.
- **Dos sensibilidades:** una para las eses suaves y otra para las duras.
- **Lookahead** de 0 a 15 ms, para que la reducción llegue antes que la «s».
- **Estéreo:** enlace de 0 a 100 % y modos Mid y Side; **sidechain externo** opcional.
- **Escucha** del sidechain filtrado y del disparo, para ajustar HP, LP y umbral de oído.
- **Modo General** para mezclas completas (platillos, batería): sin clasificador, compara solo la energía de la banda con el umbral.
- Presets, A/B, deshacer y rehacer; **sin clics** al mover cualquier control y **cero reservas de memoria** en el hilo de audio.

## Cuándo usarlo y cuándo no

ChauS sirve para domar eses duras en una pista de voz (cantada o hablada), en coros y en podcast, y, en modo General, para suavizar el exceso de agudos de un bus. **No** trata plosivas, ruido de respiración ni resonancias de sala, y **no sustituye la edición manual** de eses muy marcadas. El detector se ajustó con señales sintéticas: con voces reales conviene escuchar con *Escuchar disparo* y, si hace falta, bajar el Umbral o ajustar HP y LP (ver los límites en la sección de calidad).

# Instalación y compatibilidad

| Tema | Detalle |
| --- | --- |
| Sistema | macOS 11 o superior, binario universal (Apple Silicon e Intel) |
| Formatos | VST3 y AU (AUv2, tipo efecto) |
| Canales | Mono y estéreo; entrada de sidechain opcional (apagada, mono o estéreo) |
| Frecuencias de muestreo | 44,1 a 192 kHz, tamaño de bloque variable |
| Latencia | La del lookahead (0 a 15 ms), informada al anfitrión; el EQ dinámico no añade latencia propia |
| Fuera de alcance | Surround y Atmos, AudioSuite, funciones con IA |

## Dónde se instala

| Formato | Carpeta |
| --- | --- |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/ChauS.vst3` |
| Audio Unit | `~/Library/Audio/Plug-Ins/Components/ChauS.component` |
| Presets del usuario | `~/Library/Application Support/FERB/ChauS/Presets` |

Cada compilación copia el plugin a esas carpetas y lo firma «ad hoc». Si el paquete se lleva a otro equipo, macOS puede bloquearlo por no estar notarizado; se desbloquea con `xattr -dr com.apple.quarantine <paquete>`. Tras actualizar la versión de un AU, el anfitrión puede necesitar volver a escanear los plugins.

> **Aviso.** ChauS no está notarizado ni firmado con Developer ID. Está pensado para uso personal en el equipo donde se compila. El formato AAX (Pro Tools) compila con el SDK de Avid, pero va sin firmar (PACE) y sin validar ni probar: Pro Tools de venta no lo cargará.

# Guía de uso {.pb}

## Primeros pasos

1. Inserta ChauS en la pista de voz, **antes** de la compresión final y de los efectos de brillo.
2. Elige un preset cercano (Voz masculina, Voz femenina, Locución…) o parte de «Init».
3. Enciende **Escuchar disparo** y reproduce una frase con eses: debes oír las «s» y poco más. Ajusta **Filtro HP** y **Filtro LP** hasta que la banda cubra las sibilantes de esa voz.
4. Apaga la escucha y baja el **Umbral** hasta que la reducción (campana coral y barras ámbar abajo) actúe sobre las eses duras. **Rango** limita cuánto baja como máximo.
5. Con **S suaves** y **S duras** sube o baja la sensibilidad para cada clase de sibilante.

## Mapa de la interfaz

| Zona | Contenido |
| --- | --- |
| Cabecera | Nombre y placa **BY FERB**; presets (anterior, nombre, siguiente), A/B, deshacer y rehacer; Bypass |
| Pantalla superior | Espectro de 200 Hz a 20 kHz: **entrada** (violeta), **salida** (crema), la franja de sibilante entre HP y LP, **lo que se quita** (coral) y la campana dinámica con su manejador y su lectura (frecuencia y reducción) |
| Pantalla inferior | Unos 3 s de historia: nivel de la banda de sibilante, **umbral efectivo** (coral), **reducción** (barras ámbar desde arriba) y una marca sobre cada sibilante detectada |
| Columna derecha | Perillas **Umbral** y **Rango**; selectores **Modo**, **Banda** y **Estéreo**; interruptores **Escuchar disparo**, **Umbral adaptativo** y **Seguir la s**; **SC EXT.**, **OÍR SC** y el menú **Ajustes avanzados** |
| Fila inferior | Siete perillas: **Filtro HP**, **Filtro LP**, **Lookahead**, **S suaves**, **S duras**, **Enlace** y **Salida** |

## Controles principales

| Control | Qué hace | Rango (defecto) |
| --- | --- | --- |
| **Umbral** | Con *Umbral adaptativo*, −30 dB significa «justo en la referencia» (las eses típicas de esa voz) y cada dB menos baja el umbral un dB; el valor de fábrica, −36, trata también las eses algo por debajo de la típica. Sin adaptativo es el nivel absoluto de la banda | −60 a 0 dB (−36) |
| **Rango** | Reducción máxima | 0 a 24 dB (10) |
| **Modo** | *Voz sola* usa el clasificador de sibilantes; *General* solo compara la energía de la banda con el umbral | Voz sola |
| **Banda** | *Dividida* baja solo la banda de la «s» con un filtro; *Ancha* baja todo el audio | Dividida |
| **Estéreo** | L/R (cada canal con su detector, enlazados), Mid o Side (se procesa solo ese componente) | L/R |
| **Escuchar disparo** | Oyes la banda del detector solo mientras hay sibilante: lo que ChauS «ve» | Apagado |
| **Umbral adaptativo** | La referencia sigue al percentil 90 de los picos de sibilante de los últimos 5 s | Encendido |
| **Seguir la s** | La campana sigue la frecuencia detectada; apagado, queda en el centro de la banda | Encendido |
| **Bypass** | El del anfitrión: deja pasar la entrada retrasada lo mismo que el lookahead, con un fundido de 20 ms | Apagado |

## Banda, sensibilidad y tiempos

| Control | Qué hace | Rango (defecto) |
| --- | --- | --- |
| **Filtro HP** | Límite inferior de la banda de sibilante | 2 a 12 kHz (4,0) |
| **Filtro LP** | Límite superior | 5 a 20 kHz (12,0) |
| **Lookahead** | Retarda el audio, no el detector, para que la reducción llegue antes que la «s». Suma esa latencia | 0 a 15 ms (10) |
| **S suaves** | Desplaza el umbral para las sibilantes cuyo nivel no pasa de la referencia | −6 a +6 dB (0) |
| **S duras** | Ídem para las que la pasan | −6 a +6 dB (0) |
| **Enlace** | 100 %: los dos canales reciben la misma reducción (la mayor); 0 %: cada uno la suya | 0 a 100 % (100) |
| **Salida** | Ganancia final, con suavizado de 20 ms | −12 a +12 dB (0) |

## Ajustes avanzados, sidechain y escuchas

El botón **Ajustes avanzados** abre un menú con lo que la maqueta no tiene como perilla:

| Ajuste | Opciones (defecto) |
| --- | --- |
| **Forma** | Campana, Estante o Auto (Auto) |
| **Ancho (Q)** | Auto o 1; 1,5; 2; 3; 4; 6; 8 (Auto: se calcula con la dispersión del espectro de la s, entre 2 y 6) |
| **Liberación** | Auto o 10; 20; 40; 60; 100 ms (Auto: un cuarto de la duración de la s, entre 10 y 60 ms) |

El menú ofrece valores en pasos; los valores intermedios se pueden fijar con la automatización del anfitrión.

- **SC EXT.** usa la entrada de sidechain del anfitrión como señal de detección (la reducción se aplica al audio principal).
- **OÍR SC** reproduce el sidechain ya filtrado entre HP y LP (con **Escuchar disparo** también encendido, manda el sidechain).

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

El selector de la cabecera tiene flechas y un menú con los presets de fábrica, los del usuario y las órdenes **Guardar**, **Guardar como…**, **Eliminar** y **Mostrar carpeta de presets**. Un asterisco indica que el preset cargado se modificó. **A/B** guarda dos ranuras para comparar ajustes; **deshacer** y **rehacer** recorren los cambios. El A/B **no** se guarda con la sesión. Un preset guarda todos los controles salvo el Bypass.

| Preset | Ajustes respecto de Init |
| --- | --- |
| **Init** | Valores de fábrica |
| **Voz masculina** | HP 3,5 kHz, LP 11 kHz (Umbral −36 y Rango 10, los de Init) |
| **Voz femenina** | Rango 11 dB, HP 5 kHz, LP 14 kHz |
| **Locución** | Umbral −40, Rango 8, S suaves −1, Lookahead 12 ms |
| **Rap** | Umbral −38, Rango 12, LP 13 kHz, S duras +1, Lookahead 8 ms |
| **Coros** | Umbral −38, Rango 9, HP 4,5 kHz |
| **Voz agresiva** | Umbral −32, Rango 16, HP 4,5 kHz, LP 13 kHz, S duras +2, Lookahead 8 ms |
| **Podcast** | Umbral −39, Rango 9, HP 3,5 kHz, LP 11 kHz, Lookahead 12 ms |
| **Batería (platillos)** | Modo General, Umbral absoluto −26, Rango 6, HP 6 kHz, LP 16 kHz, sin seguir la s, Lookahead 5 ms |
| **Mezcla completa** | Modo General, banda ancha, Umbral −28, Rango 5, HP 6 kHz, LP 16 kHz, Lookahead 6 ms |

Los presets del usuario son archivos `.ferbpreset` (XML) en la carpeta indicada arriba. Los presets de voz parten de los valores de la maqueta de diseño con el Umbral 6 dB más bajo y el Rango 2 dB más alto (con los de la maqueta la reducción sobre una voz real era de 1 dB de media); Batería y Mezcla completa quedan como en la maqueta; el resto son valores iniciales. Un preset carga solo los parámetros que menciona; el resto vuelve a su valor de fábrica.

## Consejos de uso

- **Empieza con el oído:** *Escuchar disparo* enseña si la banda y el umbral están bien. Si oyes sílabas «t», «k» o consonantes, sube el Umbral o acerca HP y LP a la zona de la «s».
- **Más o menos reducción:** el valor de fábrica (Umbral −36, Rango 10) reduce unos 3 dB de media las eses de una voz cantada. Baja el Umbral para tratar más eses y sube el Rango para permitir más reducción; con −30, solo las que pasan de la referencia (el percentil 90) se reducen del todo.
- **Voces femeninas o muy brillantes:** sube HP y LP (5 y 14 kHz) y prueba la forma Estante si la «s» cae por encima de 9 kHz.
- **Mezclas:** modo General con banda ancha y Rango bajo para suavizar el exceso de agudos de un bus (la sonoridad general baja con la reducción).
- **Lookahead:** 10 ms suele ir bien en voz. Con 0 el detector necesita unos 5 ms para reconocer la «s» y el principio de cada una pasa sin tratar.
- **Consejo de mezcla:** compara con el A/B y a igual volumen; una buena reducción no se nota cuando está bien puesta.

> **Nota.** Los valores de los presets y los pesos del clasificador son iniciales: se afinan de oído y con grabaciones de voz.

# Referencia de parámetros

Todos los parámetros pasan por `AudioProcessorValueTreeState` con identificador de versión 1, son automatizables y se guardan en el estado. **No se renombran:** hacerlo rompe presets y sesiones guardadas. El bypass es el del anfitrión y no forma parte de los presets.

| ID | Nombre | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `mode` | Modo | Lista | Voz sola / General | Voz sola | General solo compara energía con el umbral |
| `thr` | Umbral | Decimal | −60 a 0 dB | −36 | Con *Adaptativo*, −30 = en la referencia |
| `adaptive` | Umbral adaptativo | Sí/No | | Encendido | Referencia: percentil 90 de las s de 5 s |
| `range` | Rango | Decimal | 0 a 24 dB | 10 | Reducción máxima |
| `hp` | Filtro HP | Decimal | 2 a 12 kHz | 4000 | Escala logarítmica |
| `lp` | Filtro LP | Decimal | 5 a 20 kHz | 12000 | Escala logarítmica |
| `track` | Seguir la s | Sí/No | | Encendido | Apagado: centro de la banda |
| `q` | Ancho (Q) | Decimal | 1 a 8 | 3 | Solo con *Ancho automático* apagado |
| `qAuto` | Ancho automático | Sí/No | | Encendido | Q entre 2 y 6 según la dispersión de la s |
| `band` | Banda | Lista | Dividida / Ancha | Dividida | Ancha baja todo el audio |
| `shape` | Forma | Lista | Campana / Estante / Auto | Auto | Auto: estante sobre 9 kHz con Q baja |
| `sensSoft` | S suaves | Decimal | −6 a +6 dB | 0 | Desplaza el umbral |
| `sensHard` | S duras | Decimal | −6 a +6 dB | 0 | Desplaza el umbral |
| `lookOn` | Lookahead activo | Sí/No | | Encendido | Sin control propio en la interfaz: la perilla dice «APAGADO» |
| `look` | Lookahead | Decimal | 0 a 15 ms | 10 | Suma latencia |
| `release` | Liberación | Decimal | 10 a 100 ms | 40 | Solo con *Liberación automática* apagada |
| `releaseAuto` | Liberación automática | Sí/No | | Encendido | Un cuarto de la duración de la s, de 10 a 60 ms |
| `link` | Enlace estéreo | Decimal | 0 a 100 % | 100 | |
| `ms` | Mid/Side | Lista | Apagado / Mid / Side | Apagado | La interfaz lo muestra como L/R, Mid y Side |
| `scExt` | Sidechain externo | Sí/No | | Apagado | |
| `listenTrig` | Escuchar disparo | Sí/No | | Apagado | Oye lo que el detector «ve» |
| `listenSC` | Escuchar sidechain | Sí/No | | Apagado | Oye el sidechain filtrado entre HP y LP |
| `output` | Salida | Decimal | −12 a +12 dB | 0 | |
| `bypass` | Bypass | Sí/No | | Apagado | Del anfitrión; fundido de 20 ms |

# Documentación técnica

## Arquitectura de señal

{{fig:flujo}}

El audio pasa por un retardo de lookahead y, en paralelo, el detector analiza la señal sin retardo: así la reducción llega antes que la sibilante. Cada canal procesado tiene su detector, su referencia adaptativa, su balística y su filtro. En Mid o Side hay un solo carril sobre ese componente y el otro solo se retrasa.

## Detector de sibilantes

Dos caminos en paralelo, por canal:

1. **Nivel de la banda**, en el dominio del tiempo: paso alto y paso bajo de segundo orden en HP y LP y una envolvente RMS de 1 ms. Es el nivel que usan la referencia adaptativa y el cálculo de la reducción.
2. **Clasificador por cuadros** (solo en *Voz sola*): 512 muestras con salto de 128 a 44,1 y 48 kHz (el doble a 96 kHz y el cuádruple a 192 kHz: mismo ancho de bin, ≈ 94 Hz, y misma duración), ventana de Hann y FFT. Seis rasgos:

| Rasgo | Qué mide |
| --- | --- |
| Energía de la banda | Nivel eficaz entre HP y LP |
| Razón | Banda frente a la zona de 300 Hz a 3 kHz |
| Planitud espectral | En la banda: una «s» se parece a ruido, un tono no |
| Centroide | Frecuencia media de la banda, como cercanía al centro geométrico de HP y LP |
| Pendiente | De 2 a 4 kHz, de la densidad espectral (dB por octava) |
| **Aire** | Densidad espectral de 12 a 18 kHz frente a 5 a 9 kHz (platillos y hi-hats son planos hasta arriba; una «s» cae). **Añadido a los cinco de la ficha original** |

Una **regresión logística** combina los rasgos en una puntuación de 0 a 1; se suaviza 5 ms y pasa por una **histéresis** (enciende en 0,6, apaga en 0,4). Con la banda en silencio (por debajo de −75 dBFS) el cuadro vale cero. Los pesos son **valores iniciales**: se ajustaron con señales sintéticas (regularización L2, sesgo bajado 0,3 sobre el ajuste) y se reajustarán con voces grabadas. En *General* no hay clasificador ni FFT: la banda siempre está «activa».

## Seguimiento de la s

La frecuencia central es el centroide del cuadro, suavizado 20 ms mientras hay sibilante; sin ella, vuelve a la **mediana** de los últimos 750 cuadros con sibilante (unos 2 s) con una constante de 200 ms. Con *Seguir la s* apagado, la campana queda en el centro geométrico de HP y LP. El ancho Q automático es `0,5 / σ` (σ, la dispersión del espectro de la banda en octavas), limitado a 2–6.

## Umbral adaptativo

Se guarda el pico de nivel de banda de cada evento (una racha de «activo», de al menos 10 ms) y el **percentil 90** de los de los últimos 5 s (rango más cercano) es la referencia, suavizada 250 ms. El umbral efectivo es `referencia + (Umbral + 30)`; sin adaptativo es el Umbral en dBFS. Sin eventos en 5 s, la referencia conserva el último valor (arranca en −30 dBFS). Una sibilante es «dura» si su nivel pasa de la referencia y «suave» si no (mezcla lineal de −6 a +6 dB sobre ella), y su clase desplaza el umbral con **S suaves** o **S duras**.

## Reducción y balística

`reducción = exceso (nivel − umbral efectivo)` con una rodilla suave de 3 dB, limitada por Rango, y solo mientras la sibilante está activa (en *General*, siempre). Con dos canales, la reducción de cada uno es una mezcla de la propia y la mayor de las dos, según el Enlace. La reducción se suaviza en dB: **ataque de 0,5 ms** y liberación de 10 a 60 ms (un cuarto de la duración de la s) o la fijada.

**Entrada progresiva.** La reducción de una sibilante se multiplica por `(edad del evento / 8 ms)²` (hasta 1), donde la edad es el tiempo seguido «activo». Un chasquido o una plosiva de menos de 8 ms casi no se trata; una «s» de 100 ms recibe casi toda su reducción. Con audio real: la reducción de pico en eventos de menos de 20 ms baja de 0,92 a 0,25 dB y en las «s» (≥ 80 ms) pasa de 4,21 a 4,17 dB; con 15 ms y 30 ms la protección es mayor pero se pierde demasiado al principio de cada «s» (el faltante sintético sube de 3,3 a 4,7 dB con 15 ms).

## Aplicación

- **Banda dividida:** una **campana** (biquad RBJ) o un **estante** de agudos en la frecuencia seguida, con la ganancia `−reducción`. Los coeficientes se recalculan cada 16 muestras y se interpolan. En *Auto* se usa el estante si la frecuencia pasa de 9 kHz y la Q es baja, con un fundido de 30 ms entre campana y estante (se calculan las dos). No añade latencia propia (es IIR).
- **Banda ancha:** una ganancia sobre todo el audio con la misma reducción.
- **Estéreo:** en L/R, un carril por canal; en Mid o Side, un carril sobre ese componente (el otro solo se retrasa).
- **Escuchas:** *sidechain* = la banda filtrada que oye el detector; *disparo* = esa banda por una puerta de 2 ms que sigue a «activo». Ambas con el mismo retardo que el audio.

## Latencia

La latencia es exactamente el lookahead en muestras enteras (480 a 48 kHz con 10 ms; 720 con 15; 0 con el lookahead apagado). **No es constante:** cambia con el lookahead. El procesador la declara en cuanto cambia `look` o `lookOn`; el motor adopta el nuevo retardo cuando el valor lleva 150 ms estable, con un fundido de 50 ms entre dos tomas del retardo, para no encadenar fundidos al arrastrar la perilla.

> **Nota.** Los anfitriones tratan de forma distinta los cambios de latencia durante la reproducción. Para grabar con monitorización, apaga el lookahead.

## Cambios sin clics

| Cambio | Mecanismo |
| --- | --- |
| Umbral, Rango, HP, LP, sensibilidades, Enlace, Q, liberación | La reducción pasa por la balística (ataque de 0,5 ms): la ganancia no cambia más rápido |
| Frecuencia y Q de la campana | Suavizado de 20 ms y coeficientes interpolados cada 16 muestras |
| Forma | Fundido de 30 ms entre campana y estante |
| Salida, bypass, escuchas | Suavizado de 20 ms, 20 ms y 10 ms (en doble precisión) |
| Banda (dividida o ancha) y estéreo (L/R, Mid, Side) | Fundido a silencio de 5 ms con `ModeSwitch` |
| Lookahead | Fundido de 50 ms entre dos tomas, tras 150 ms estable |

## Reglas de tiempo real

- Sin asignaciones de memoria ni bloqueos en `processBlock`: todo se reserva en `prepare` (retardos para 15 ms, FFT, anillos). Lo vigila una prueba con un asignador instrumentado, también con los anillos de la pantalla.
- `juce::ScopedNoDenormals` al inicio.
- La entrada con NaN o infinito se sustituye por 0 antes de entrar a los filtros.
- La FFT de la pantalla (2048) corre en el hilo de la interfaz; los datos pasan por anillos sin bloqueos.

## Datos para la pantalla

El hilo de audio escribe, sin bloquearse, la entrada y la salida (suma a mono) en dos anillos de 16 384 muestras y, cada 8 ms, un punto de telemetría (nivel de banda, umbral efectivo, reducción, frecuencia de la campana, evento). La interfaz, a 30 Hz, calcula el espectro por bandas de 1/6 de octava (FFT de 2048, cuatro ventanas por lectura, solape del 75 %) y dibuja la respuesta real del filtro en uso. Los anillos se reservan en el constructor del procesador porque un validador puede abrir el editor antes de `prepareToPlay`.

## Estado y presets

El estado es el árbol de `AudioProcessorValueTreeState` más las propiedades de interfaz (`uiScale`, nombre del preset). Los presets guardan el valor real de cada parámetro; los de fábrica van en el código y los del usuario en archivos `.ferbpreset` (XML). Al restaurar un estado, los Bool y Choice se sincronizan con `ferb::syncParametersFromState`.

## Estructura del código

| Carpeta | Contenido |
| --- | --- |
| `ChauS/source/dsp/` | `SibilanceDetector` (rasgos, clasificador, umbral adaptativo), `ChausEngine` (reducción, seguimiento, EQ, lookahead, estéreo, telemetría), `EqMath` |
| `ChauS/source/ui/` | Analizador de espectro, pantalla superior y línea de tiempo |
| `ChauS/tests/` | Pruebas del motor y del procesador, señales sintéticas, ajuste de pesos (`[.train]`) e informes (`[.report]`) |
| `core/`, `ui/`, `ui/vintage/` | DSP común, presets, A/B, escalado y el kit de interfaz «aparato analógico» compartido |

# Calidad y verificación

Medidas a 48 kHz, estéreo y bloque de 256 salvo que se diga. Las señales de prueba son **sintéticas** (voz con formantes y eses de ruido de banda, platillos, hi-hats y «t» modelados) y las semillas de las pruebas son distintas de las del ajuste de los pesos. Resultados del 31 de octubre de 2026.

| Prueba | Meta | Resultado |
| --- | --- | --- |
| Sibilante sintética (ruido de 6 a 9 kHz con envolvente, sobre voz), activo en el pico | 95 % o más | 215 de 216 (99,5 %) a 44,1, 48 y 96 kHz |
| Voz sintética sin eses | Sin falsos positivos | 0,0 % del tiempo activo |
| Platillo sintético | Menos de 5 % | 1,6 % del tiempo activo; 3 % de los platillos activan alguna vez |
| **Hi-hat sintético** | Menos de 5 % | **No se cumple:** 12,5 % del tiempo y 33 % de los golpes (hi-hat metálico); 35 % y 77 % si lleva mucho ruido blanco filtrado |
| **«t» sintética** | Menos de 5 % | **No se cumple:** 10,9 % del tiempo de la ráfaga; 70 % de las «t» activan alguna vez |
| Voces del sistema (macOS `say`, español; 188 palabras) | Informe | Activa en las 72 palabras con «s» (100 %); en 25 de las 92 sin «s» (27 %: sobre todo «t», «k», «p», «d») con 0,7 % del tiempo activo; en 14 de 24 con «f» o «j» |
| **Audio real** (14 pistas de una sesión, 12 min de voz y 15 min de instrumentos; sin etiquetas) | Informe | Voces: 13 a 24 eventos por minuto (≈ 125 ms, 7–9 kHz); una regla independiente de ráfagas halló 121 candidatos y el detector cogió 115 (95 %). Instrumentos: 0 activaciones. Karaoke con batería real (269 s): el hi-hat no activa nada; 12 activaciones con platillos crash. Con los valores de fábrica, reducción media de 2,4 a 3,0 dB en las eses de las voces (picos de 10 dB) |
| Seguimiento de una s que sube de 6 a 9 kHz | Error medio < 300 Hz | 84 Hz (44,1 kHz), 95 Hz (48 kHz) y 189 Hz (96 kHz) |
| Rango 0 (todas las combinaciones de banda, estéreo y forma) | Salida idéntica a la entrada retrasada | Por debajo de −110 dBFS |
| Rango | La reducción nunca lo supera | Máximo ≤ Rango + 0,001 dB; llega a Rango − 0,5 dB con eses fuertes |
| Banda dividida, Rango 24 | Fuera de banda, < 0,5 dB | Por debajo de 2 kHz: −0,02 dB; la banda de la s baja 13,0 a 13,2 dB |
| Umbral adaptativo, misma frase a −6 y −12 dB | Diferencia < 1,5 dB | 0,00 dB (44,1 kHz) y 0,04 dB (48 kHz) |
| Lookahead de 10 ms: 90 % de la meseta antes del pico de la s | Todas | Todas las eses medidas, a 44,1, 48 y 96 kHz |
| **Retraso entre el inicio de la s y el 90 % de la reducción** | 0 ms | **No se cumple (+18,5 ms con 10 ms de lookahead):** la reducción es 1:1 sobre el exceso y la s de mi modelo tarda ≈ 30 ms en subir. Lo que falta por quitar en cada instante baja de 6,8 dB (sin lookahead) a 3,7 dB (10 ms), con 1,5 ms por encima de 3 dB |
| Enlace 100 %; Mid y Side | Misma reducción en L y R; solo cambia el componente elegido | Idéntica muestra a muestra; el otro componente sale idéntico (< −110 dBFS) |
| Latencia, impulso | Medida igual a la declarada | Exacta con 0, 4, 10 y 15 ms a 44,1, 48 y 96 kHz |
| Sin clics (20 controles y cambios de estructura) | ≤ 1,3 veces la señal estable | Cumple |
| Robustez | Sin NaN ni picos | Silencio, continua, NaN, infinitos y subnormales; 44,1 a 192 kHz; bloques de 1 a 4096; el resultado no depende del tamaño del bloque |
| Espectro de la pantalla (senoidal de −23,01 dBFS eficaces) | Nivel y frecuencia | Dentro de ±3,1 dB desde 1 kHz (3,6 dB a 300 Hz) |
| `processBlock` | Sin reservas de memoria | 0 reservas (mono y estéreo, parámetros aleatorios y estructura cambiando) |
| Procesador completo | Sin fallos | 18 millones de comprobaciones: tasas, bloques, canales, estado, editor (también sin `prepareToPlay`), latencia, presets, A/B y deshacer |
| CPU, estéreo (31 oct 2026, % de un núcleo, lookahead 10 ms) | Menos de 2 % a 48 kHz | 44,1 kHz 0,57 % · **48 kHz 0,6 %** (banda ancha 0,58 %; General 0,2 %) · 96 kHz 1,2 % · 192 kHz 2,6 % |
| Coste de pintar la ventana entera (software, peor caso) | Menos de 33 ms | 75 %: 4,1 ms · 100 %: 5,8 ms · 200 %: 17,5 ms |

> **Nota.** **Los porcentajes de detección salen de señales que escribí yo**, no de voces grabadas. La meta de 95 % de aciertos sobre eses reales y los falsos positivos de hi-hat y «t» quedan pendientes de una pasada de ajuste con grabaciones (hito M6 de la ficha). Con estos seis rasgos espectrales, un hi-hat de ruido blanco filtrado es indistinguible de una «s» aguda: es un límite del método, no solo del ajuste. En la práctica, una «t» o un hi-hat activarán el detector y recibirán algo de reducción; el modo *Voz sola* es para pistas de voz.

## Cómo repetir las medidas

```bash
ctest --test-dir build                               # todas las pruebas
./build/ChauS/ChauSTests "[.report]"                 # informes: sesgo, lookahead, CPU, seguimiento
./build/ChauS/ChauSTests "[.train]"                  # reproduce el ajuste de los pesos
./build/ChauS/ChauSProcessorTests                    # procesador completo con editor
CHAUS_TTS_DIR=<carpeta con los WAV> ./build/ChauS/ChauSTests "[.report]"   # voces del sistema (macOS «say»)
```

# Compatibilidad y validación

| Validador | Resultado |
| --- | --- |
| `auval -v aufx ChaS Ferb` | Pasa |
| `pluginval` nivel de rigor 10 (VST3 y AU, con pruebas de interfaz) | Pasa |
| `ChauSTests` y `ChauSProcessorTests` (ejecutadas el 31 de octubre de 2026) | Pasan |

Pendiente: comprobación manual en Ableton Live, Logic Pro y Pro Tools.

# Limitaciones y pendientes

- **Detector sin validar con voz real.** Las cifras de detección son sobre señales sintéticas y voces del sistema; los pesos son iniciales. Falta ajustarlos con grabaciones de voz con eses duras.
- **Hi-hat y «t»:** activan el detector (ver la sección de calidad); el hi-hat de ruido blanco es indistinguible de una «s» aguda con estos rasgos.
- **La reducción no llega «a tiempo»** respecto del inicio de la s: el lookahead recorta el faltante, no lo elimina; con lookahead 0 el principio de cada «s» pasa sin tratar.
- **Sin prueba manual en anfitriones.** Falta probarlo a mano en Ableton, Logic y Pro Tools.
- **AAX (Pro Tools):** compila (SDK de AAX 2.8.0, probado en arm64) y se instala sin firmar; **sin validar** (falta el AAX Validator), **sin firma PACE** y **sin probar en Pro Tools**. Pro Tools de venta solo carga AAX firmados.
- **Latencia variable** con el lookahead (a diferencia de Tufi, Poty y Cremita).
- **Ajustes avanzados en menú:** Forma, Ancho y Liberación no tienen perilla; el menú ofrece pasos.
- **HP y LP** solo se ajustan con las perillas (arrastrarlos sobre la pantalla queda como mejora). El estado de interfaz `displayZoom` de la ficha no está implementado.
- **Daltonismo:** la legibilidad del coral sobre negro no se comprobó (la lectura numérica de la campana ayuda).
- **Valores iniciales por afinar de oído:** el ancho Q automático, la liberación automática, la clase de sibilante (suave o dura) y los presets que no vienen de la maqueta.
- **A/B** que no se guarda con la sesión.
- **Distribución:** firma ad hoc, sin notarización. Antes de distribuirlo hay que revisar las condiciones de licencia de JUCE.

# Historial de versiones

| Versión | Fecha | Cambios |
| --- | --- | --- |
| **1.0.0** | {{fecha}} | Primera edición numerada. Reúne los hitos M0 a M5: detector de sibilantes (seis rasgos, regresión logística con histéresis), umbral adaptativo, EQ dinámico con seguimiento de frecuencia y Q, lookahead de 0 a 15 ms, enlace estéreo y Mid/Side, sidechain externo, escuchas, 24 parámetros, 10 presets, A/B, deshacer e interfaz vintage con espectro y línea de tiempo; sin clics y sin reservas en `processBlock`. |

El número de versión sigue SemVer y está en `cmake/Versions.cmake`: **mayor** si algo rompe presets o sesiones (IDs de parámetros, formato de estado), **menor** para funciones nuevas compatibles y **parche** para correcciones.

# Avisos {.ap}

- **Licencias de software.** ChauS está construido con JUCE 8.0.15 y las pruebas con Catch2. Las condiciones de licencia de JUCE deben revisarse antes de cualquier distribución.
- **Tipografías.** Limelight, Josefin Sans y Oxanium, incrustadas en el plugin bajo sus licencias abiertas (ver `ui/vintage/fonts/LICENSE.md`).
- **Marcas de terceros.** Los nombres de productos de otras empresas se citan solo como referencia funcional y pertenecen a sus propietarios. FERB no está afiliado a ellas.
- **Uso.** Software de uso personal, entregado tal cual, sin garantía.
