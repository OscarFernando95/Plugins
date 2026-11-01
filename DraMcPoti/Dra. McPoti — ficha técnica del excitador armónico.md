# Dra. McPoti — ficha técnica del excitador armónico

Oct 31, 2026 · @Oscar Fernando

## 1. Resumen y concepto

Dra. McPoti es un excitador armónico multibanda y dinámico, con cara de aparato de rack, que reúne en un solo plugin el sonido de los excitadores clásicos y las exigencias de mastering de hoy. Un interruptor elige el mundo (Vocal o Master) y otro elige la era (Vintage o Moderno); cada era ofrece cuatro caracteres distintos, ocho en total, disponibles en ambos modos.

| Aspecto | Decisión |
| --- | --- |
| Nombre | Dra. McPoti (nombre elegido por ti; la doctora del brillo) |
| Formatos | VST3, AU y AAX (macOS), más Standalone para pruebas |
| Códigos | AU aufx, subtipo McPo, fabricante Ferb |
| Motor | 4 bandas (Cuerpo, Medios, Presencia, Aire) con crossover Linkwitz-Riley o de fase lineal |
| Modos | Vocal (guarda de sibilancia, fase libre) y Master (M/S, mono-seguro, level match) |
| Eras | Vintage (4 caracteres) y Moderno (4 caracteres) |
| Apariencia | Rack de válvulas retro de 3U: placa gris pizarra con orejas de acero, perillas negras estriadas con escala numérica, interruptores cromados con etiquetas negras, selector de pala, cuatro válvulas incandescentes y dos vumétros de aguja en marco negro |
| Uso | Personal, sin licencias; firma ad-hoc en macOS |

### Las dos ideas centrales

1. **Armónicos creados, no solo realzados.** El núcleo sigue el principio de los excitadores clásicos: un pasa-altos alimenta un generador de armónicos, un desfase controlado separa lo añadido de lo original, y el resultado se mezcla en paralelo. Por eso puede dar aire a una grabación que no lo tiene.
2. **Dinámica por banda.** Cada banda reacciona al nivel: puede dar más brillo en pasajes suaves (efecto abierto y sin dureza) o más en los picos (punch). Es lo que hace moderno al motor sin quitarle el carácter vintage.

### Qué debe sonar

- **Vocal Vintage:** brillo inmediato y cálido, tipo años 70.
- **Vocal Moderno:** aire sedoso y constante, sin sibilancia.
- **Master Vintage:** cinta o válvula con pegamento sutil, sin cambiar el balance.
- **Master Moderno:** apertura limpia, ancho controlado y compatibilidad mono.

## 2. Posicionamiento de mercado

Ningún producto revisado reúne el carácter vintage, la dinámica por banda y las herramientas de mastering en un solo aparato; esa combinación es el hueco que ocupa Dra. McPoti. El análisis completo está en el estudio de la conversación; aquí va lo que se toma de cada referencia.

| Referencia | Qué la hace valiosa | Qué toma Dra. McPoti |
| --- | --- | --- |
| Aphex Aural Exciter (y su versión de Waves) | Pasa-altos, generador de armónicos y desfase; mezcla baja; Big Bottom en el Type III | Carácter Clásico, mando de sintonización y banda Cuerpo |
| Slate Fresh Air | Dos mandos, saturación que depende del nivel | Dinámica por banda y simplicidad en modo Vocal |
| SPL Vitalizer | Desenmascaramiento, ancho estéreo, saturación suave | Control de ancho y filosofía de apertura en Master |
| Kush Clariphonic | Shelves en paralelo sobre señal intacta; solo suma | Carácter Seda (sin armónicos) |
| Maag EQ4 Air Band | Shelf solo de subida de 2.5 a 40 kHz, hasta +20 dB | Carácter Aire con esquina variable |
| iZotope Ozone Exciter | Hasta 4 bandas, modo por banda, oversampling, filtro posterior | Modo por banda, oversampling y filtro Suavizar |
| Waves Vitamin / Sugar | Multibanda con drive y mezcla por banda, level match | Estructura de 4 bandas con mezcla propia |
| Oeksound Bloom | Realce adaptativo según el balance tonal | Carácter Adaptativo |
| UAD A-Type Multiband Dynamic Enhancer | Compander de 4 bandas inspirado en la reducción de ruido Dolby A; modos Excite, Expand, Air, Crush y Gated; Excite no añade armónicos; Air suma un shelf de unos 10 dB que se aplana al comprimir | Carácter Dinámico (compander por banda), Aire (shelf dinámico) y rangos de cruce parecidos |

### Diferenciadores

- **Un solo aparato, dos mundos.** Vocal y Master cambian rangos, valores iniciales, guarda de sibilancia, M/S y medidores, sin cambiar de plugin.
- **Vintage real con control moderno.** Los clásicos no tienen oversampling, M/S ni compensación de nivel; Dra. McPoti sí.
- **Guarda de sibilancia propia.** Un detector, heredado de la lógica de ChauS, baja la excitación cuando hay una S.
- **Medición honesta.** Muestra cuánto armónico añade cada banda, ofrece escucha solo de lo añadido (Delta) y compensa el nivel para comparar sin trampa.

Limitación de la investigación: no se revisaron manuales internos de Waves ni Slate, ni Dangerous BAX, bx\_saturator o HG-2; los algoritmos de los productos comerciales no son públicos, así que Dra. McPoti toma sus principios, no su código.

### Fuentes consultadas

- [Ozone, módulo Exciter](https://downloads.izotope.com/docs/ozone8/exciter/index.html)
- [Maag EQ4, guía de usuario](https://www.audiosteps.io/wp-content/uploads/Maag-Audio-EQ4-User-Guide.pdf)
- [Sound On Sound, reseña de Kush Clariphonic](https://www.soundonsound.com/reviews/kush-audio-clariphonic)
- [Unison, mejores plugins excitadores](https://unison.audio/best-exciter-plugins/)

* [UA, manual del A-Type Multiband Enhancer](https://help.uaudio.com/hc/en-us/articles/34757792026004-A-Type-Multiband-Enhancer-Manual)
* [Sound On Sound, reseña del UAD A-Type](https://www.soundonsound.com/reviews/universal-audio-type?amp)

## 3. Arquitectura DSP

La señal se divide en cuatro bandas, cada una crea y modela sus propios armónicos, y lo añadido se suma en paralelo a un seco con el mismo retardo; el seco nunca se procesa.

&#91;embedded content: cadena de señal · 4 bandas en paralelo\]

El camino de la izquierda lleva el seco, retardado lo mismo que la ruta procesada; el Generador, resaltado, es el único bloque no lineal.

### Cruce

- Crossover Linkwitz-Riley de 4.º orden (LR4) por defecto, con puntos en xo1, xo2 y xo3; suma plana con Drive 0.
- Opción de fase lineal con FIR de 2048 coeficientes, que añade latencia (sección 8).
- Rangos tomados de la lógica del A-Type: bajos 80–400 Hz, medios 600 Hz–4 kHz, altos 3–12 kHz.

### Dinámica por banda

Cada banda mide su nivel con un RMS de 20 ms (ataque 5 ms, relajación 120 ms) y ajusta su drive efectivo:

```latex
e_b = \mathrm{clamp}\Big(\frac{L_b - L_{ref}}{12},\,-1,\,1\Big)
```

```latex
a_b = \frac{\text{drive}_b}{100} \cdot 2^{-\,\text{dyn}_b\,e_b/50}
```

Con Dinámica positiva la banda aporta más en pasajes suaves y menos en picos (apertura sin dureza); con negativa, al revés (punch). L\_ref es −24 dBFS en Vocal y −18 dBFS en Master.

### Los caracteres no generadores

- **Dinámico (compander, estilo A-Type).** Una copia comprimida de cada banda se suma al original. Drive fija el umbral con ganancia automática y Dinámica la relación de 2:1 a 8:1. No crea armónicos; realza el detalle suave.
- **Aire (shelf dinámico).** Con la señal por debajo del umbral hay un shelf de pre-énfasis de unos 10 dB que se aplana a medida que la compresión actúa, parecido a un EQ dinámico, con esquina entre 8 y 40 kHz (se calcula en el dominio sobremuestreado).
- **Seda (shelf paralelo).** Shelf de segundo orden por banda que se suma a la señal intacta; solo realza y nunca resta.
- **Adaptativo.** Mide la razón de energía entre Presencia y Medios con un promedio de 2 s y mueve el drive hasta ±6 dB hacia un objetivo tonal; la lentitud evita el bombeo.

### Guarda de sibilancia (Vocal)

Un detector de la lógica de ChauS, simplificado, compara la energía de 5–10 kHz con la de 300 Hz–3 kHz y la planitud espectral, con histeresis. Cuando hay una S, baja Presencia y Aire hasta 12 dB multiplicados por Guarda, con ataque de 1 ms, relajación de 40 ms y lookahead de 2 ms.

### Fase y estéreo

- **Fase:** hasta cuatro secciones de pasa-todo de segundo orden (600 Hz, 1.8 kHz, 5 kHz y 11 kHz) solo en la ruta de lo añadido; el mando mezcla de cero a cuatro secciones con transición continua.
- **Ancho (Master):** escala el canal lateral de lo añadido de 0 a 200 %.
- **Mono seguro (Master):** lo añadido por debajo de 150 Hz pasa a mono.
- En Vocal el procesado es dual-mono con enlace de envolventes (parámetro Enlace).

### Salida

```latex
y[n] = x[n - D] + \text{mix}\cdot\sum_{b}\ell_b\,h_b[n]
```

D es la latencia total declarada. Con Delta activo se escucha solo la suma de h\_b. Igualar aplica una ganancia de hasta ±12 dB calculada con un integrador K-weighted de 3 s, como en Cremita. Oversampling: juce::dsp::Oversampling polifásico, solo en los bloques no lineales y en los shelves de esquina alta.

## 4. Voicings: los ocho caracteres

Cada era ofrece cuatro caracteres; el mando Voicing los recorre y la palanca Era decide cuál de los dos juegos se usa. Los coeficientes son valores iniciales para afinar de oído y con mediciones, no cifras de un producto concreto.

| Era | Carácter | Generador | Armónicos | Fase | Funciona mejor en |
| --- | --- | --- | --- | --- | --- |
| Vintage | Clásico | Pasa-altos, no linealidad asimétrica, pasa-altos de nuevo | 2.º y 3.º, brillo inmediato | Sí (red de pasa-todo) | Vocal |
| Vintage | Cinta | Pre-énfasis, tanh, de-énfasis | Impares suaves, compresión leve | Opcional | Master y Vocal |
| Vintage | Válvula | Triodo asimétrico con polarización | Pares dominantes, cálido | Opcional | Vocal y Master |
| Vintage | Transistor | Recorte suave simétrico de orden 2 | Impares, brillante y firme | Opcional | Master (percusión, mezcla) |
| Moderno | Dinámico | Compander por banda tipo Dolby A: copia comprimida de cada banda sumada al original | Ninguno; realza el detalle suave | No | Vocal |
| Moderno | Adaptativo | Válvula ligera con drive guiado por el balance tonal medido | Pares, ajustados a un objetivo | No | Mezclas y Master |
| Moderno | Seda | Shelf en paralelo sobre la señal intacta | Ninguno (solo realce) | Mínima | Vocal y Master |
| Moderno | Aire | Shelf de pre-énfasis de 10 dB en esquina alta (8–40 kHz) que se aplana al comprimir, más 5 % de pares | Casi ninguno | Mínima | Master |

### Núcleo común de los caracteres generadores

Para cada banda b, con frecuencia de sintonización fc (mando Sintonización):

```latex
h_b[n] = \mathrm{AP}_{\varphi}\Big\{ \mathrm{HP}_{f_c}\big( f_v( a_b \cdot \mathrm{HP}_{f_c}(x_b[n]) ) \big) \Big\}
```

```latex
y_b[n] = x_b[n] + \ell_b \cdot \mu_b \cdot \gamma \cdot h_b[n]
```

Donde f\_v es la no linealidad del carácter, a\_b el drive efectivo, AP la red de pasa-todo que fija la fase (mando Fase), l\_b el nivel de la banda, mu\_b su mezcla, y gamma una normalización para que la ganancia de lo añadido sea comparable entre caracteres.

### No linealidades iniciales

```latex
f_{\text{clásico}}(u) = \tanh(1.5u) + 0.25\,(u^2 - \overline{u^2})
```

```latex
f_{\text{válvula}}(u) = \tanh\big(k(u+\beta)\big) - \tanh(k\beta),\quad k = 1.2,\ \beta = 0.2
```

```latex
f_{\text{transistor}}(u) = \frac{u}{(1+|u|^{2})^{1/2}}
```

Cinta aplica un shelf de pre-énfasis de +6 dB desde 5 kHz, luego tanh(g·u) y un shelf inverso. El único término par de Clásico se centra restando su media para no generar corriente continua. Todas las funciones se evalúan con ADAA de primer orden cuando existe forma cerrada (tanh la tiene: antiderivada ln cosh) y con oversampling cuando no.

### Coloración par/impar

El mando Color (−100 a +100) mezcla en cada carácter dos ramas: una simétrica (impares) y otra asimétrica (pares). Con 0 se usa el reparto propio del carácter; hacia −100 se favorecen los pares (cálido) y hacia +100 los impares (presente).

### Qué significa Drive en cada carácter

- En Clásico, Cinta, Válvula, Transistor y Adaptativo, Drive es la cantidad de armónicos creados (0–100). En Dinámico, Drive es la cantidad del compander (umbral con ganancia automática, como el mando Amount del A-Type) y Dinámica fija su relación de 2:1 a 8:1.
- En Seda y Aire, Drive es la ganancia del shelf (0 a +15 dB) y no se crean armónicos; la esquina del shelf la fija la banda (Seda) o el mando Esquina de aire (Aire).

## 5. Parámetros

Son 44 parámetros expuestos al host mediante AudioProcessorValueTreeState; los identificadores no deben cambiar entre versiones para no romper las sesiones guardadas. Las bandas se llaman Lo (Cuerpo), Md (Medios), Hi (Presencia) y Ar (Aire).

| ID | En pantalla | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- |
| mode | Modo | Vocal, Master | Vocal | Cambia la plantilla de valores, la guarda y los medidores |
| era | Era | Vintage, Moderno | Vintage | Selecciona el juego de cuatro caracteres |
| voice | Voicing | 1 a 4 | 1 | Carácter dentro de la era (sección 4) |
| input | Entrada | −12 a +12 dB | 0 | Antes del crossover |
| output | Salida | −12 a +12 dB | 0 | Después de todo |
| mix | Mezcla | 0 a 100 % | 100 | Cantidad global de lo añadido |
| tune | Sintonización | 1.5 a 12 kHz | 4 kHz | Pasa-altos del que se alimenta el generador; en cada banda vale max(tune, borde inferior). Cuerpo lo ignora |
| phase | Fase | 0 a 100 % | 40 | Cantidad de desfase de lo añadido (red de pasa-todo) |
| color | Color | −100 a +100 | 0 | Pares (−) a impares (+) |
| soften | Suavizar | 4 a 20 kHz (20 = off) | 20 kHz | Corte superior de lo añadido |
| airCorner | Esquina de aire | 8 a 40 kHz | 16 kHz | Solo en el carácter Aire; usa oversampling ≥ 4× por encima de la frecuencia de Nyquist |
| guard | Guarda | 0 a 100 % | 60 | Reduce la excitación en Presencia y Aire ante sibilancia (hasta 12 dB). Solo en Vocal |
| width | Ancho | 0 a 200 % | 100 | Escala el lateral de lo añadido. Solo en Master |
| monoSafe | Mono seguro | off, on | on en Master | Lo añadido por debajo de 150 Hz va en mono |
| link | Enlace | 0 a 100 % | 100 | Enlace estéreo de las envolventes |
| xo1 | Cruce 1 | 80 a 400 Hz | 150 Hz | Cuerpo / Medios |
| xo2 | Cruce 2 | 600 a 4000 Hz | 1800 Hz | Medios / Presencia |
| xo3 | Cruce 3 | 3 a 12 kHz | 7 kHz | Presencia / Aire. Se impone xo1 < xo2 < xo3 con razón mínima 1.5 |
| driveLo, driveMd, driveHi, driveAr | Drive | 0 a 100 | 20, 25, 40, 35 | Significado según el carácter (sección 4) |
| dynLo, dynMd, dynHi, dynAr | Dinámica | −50 a +50 | 0 | − más en picos (punch); + más en pasajes suaves (abierto) |
| mixLo, mixMd, mixHi, mixAr | Nivel de banda | 0 a 100 % | 100 | Mezcla propia de cada banda |
| onLo, onMd, onHi, onAr | Activar banda | off, on | on | Lámpara ámbar |
| soloLo, soloMd, soloHi, soloAr | Solo banda | off, on | off | Escucha la banda excitada |
| os | Oversampling | 1×, 2×, 4×, 8× | 4× | Decidido tras medir el aliasing (sección 11): 2× no cumple la meta de −70 dBFS. Master puede ir a 8× |
| linPhase | Fase lineal | off, on | off | Crossover FIR; añade latencia |
| delta | Delta | off, on | off | Escucha solo lo añadido |
| match | Igualar | off, on | on | Compensación de sonoridad K-weighted, integrador de 3 s, ±12 dB |
| vuRef | Referencia VU | −24 a −12 dBFS | −18 | 0 VU en dBFS |
| bypass | Bypass | off, on | off | Con cruce de 10 ms sin clic |

Los 44 parámetros salen de contar cada fila de banda por sus cuatro bandas. Al cambiar de modo, si el usuario no ha tocado un control desde la última carga de preset, el plugin aplica la plantilla del modo (tune, guard, width, monoSafe, xo, os); un control tocado se respeta.

## 6. Interfaz: rack analógico

Dra. McPoti se ve como un aparato de rack de válvulas de los años 60 y 70: placa frontal gris pizarra con textura cepillada, orejas de acero con ranuras, rotulado serigrafiado blanco hueso y una doble línea roja bajo el nombre, perillas de baquelita negra con borde estriado, centro hundido, marca blanca y escala numerada de 0 a 10 alrededor (inspiradas en los ecualizadores y compresores ópticos clásicos), interruptores cromados con tuerca hexagonal y etiquetas negras que se iluminan en rojo al activarse, un selector de pala negra para el Voicing, cuatro válvulas de vidrio que se encienden con el calor, dos vumétros de aguja en marco negro con esfera crema y una joya roja de encendido. El ámbar de válvulas y ventana es el único color cálido. No copia ningún modelo ni rotulado de otra marca; mezcla la sobriedad de un excitador clásico con el panel de un enhancer multibanda de rack.

Tamaño base 1400 por 600 píxeles (540 en la primera versión de la ficha). El redimensionado va del 75 % al 200 % con proporción fija (ComponentBoundsConstrainer) y se guarda con la sesión.

| Zona | Contenido |
| --- | --- |
| Cabecera (64 px) | Logo Dra. McPoti, barra de presets (anterior, nombre, siguiente, guardar), A/B, deshacer y rehacer |
| Izquierda (240 px) | Interruptor Modo (Vocal / Master) e interruptor Era (Vintage / Moderno), con etiquetas negras que se encienden en rojo; selector de pala Voicing de cuatro posiciones numeradas cuyos nombres cambian con la era; perillas Entrada, Mezcla y Salida |
| Centro (780 px) | Cuatro módulos de banda (Cuerpo, Medios, Presencia, Aire): una válvula de vidrio que brilla según lo añadido, placa de aluminio con el nombre, rango de frecuencias, perilla grande Drive, perillas pequeñas Dinámica y Nivel, y pulsadores ON y Solo; entre módulos, perillas de cruce |
| Derecha (260 px) | Dos vumétros de aguja (entrada y salida) en marco negro; ventana de armónicos con trazo ámbar sobre vidrio oscuro; seis perillas: Sintonización, Fase, Color, Suavizar, Guarda o Ancho (según el modo) y Esquina |
| Pie (44 px) | Oversampling 1× a 8×, Fase lineal, Delta, Igualar, Mono seguro, Bypass y chips de tamaño |

### Comportamiento

- **Modo:** en Vocal se ilumina la perilla Guarda y se atenuan Ancho y Mono seguro; en Master ocurre lo contrario.
- **Era y Voicing:** la perilla Voicing muestra cuatro nombres; al cambiar de era se reescriben. Las perillas Drive cambian su rotulado a Ganancia del shelf cuando el carácter es Seda o Aire, y Esquina de aire solo se enciende con Aire.
- **Ventana de armónicos:** dibuja con trazo tenue el espectro de entrada y con trazo ámbar el de salida; las zonas de las bandas aparecen como cortes verticales y la Sintonización como una línea punteada. Se alimenta por una FIFO sin bloqueos y una FFT de 2048 puntos en el hilo de interfaz.
- **Válvulas:** cada una mide su banda; el filamento se calienta al abrir el plugin y brilla más cuanto más se añade, en dB relativos a la entrada de esa banda (de −30 a 0 dB). Con Bypass quedan a media luz.
- **VU:** balística de 300 ms, 0 VU en el valor de Referencia VU; el de salida indica si Igualar está activo.
- **Perillas:** arrastre vertical, doble clic para el valor por defecto, rueda del ratón, y Mayus para ajuste fino.
- **Presets:** barra con anterior y siguiente, guardar como y carpetas Vocal y Master; A/B copia el estado completo.

La maqueta interactiva (sin audio) está publicada en https://claude.ai/artifact/8skaEbKHTDhGuGoqMPXyAC y es la referencia visual para el desarrollo.

## 7. Presets

Dra. McPoti trae 20 presets de fábrica, cinco por cada cuadrante (Vocal o Master, Vintage o Moderno), más Init. Los valores son puntos de partida; se ajustan escuchando sobre material real.

| N.º | Nombre | Modo | Carácter | Drive Lo / Md / Hi / Ar | Sintonización | Para qué |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | Radio setentas | Vocal | Clásico | 10 / 20 / 55 / 40 | 4.5 kHz | Voz brillante e inmediata |
| 2 | Susurro cálido | Vocal | Válvula | 25 / 30 / 35 / 25 | 3.5 kHz | Voz íntima y cercana |
| 3 | Presencia de cinta | Vocal | Cinta | 15 / 25 / 45 / 30 | 4 kHz | Voz principal de canción |
| 4 | Rap al frente | Vocal | Transistor | 10 / 30 / 55 / 35 | 5 kHz | Voz que corta en una mezcla densa |
| 5 | Aire de estudio | Vocal | Clásico | 0 / 15 / 35 / 60 | 6 kHz | Aire en coros y voces opacas |
| 6 | Seda pop | Vocal | Seda | 0 / 10 / 35 / 50 | 4 kHz | Lustre sin sibilancia |
| 7 | Dinámico limpio | Vocal | Dinámico | 15 / 25 / 40 / 40 | 4 kHz | Brillo estable en voces con mucha dinámica |
| 8 | Adaptativo coro | Vocal | Adaptativo | 10 / 20 / 35 / 35 | 4 kHz | Grupos de voces |
| 9 | Locución nítida | Vocal | Dinámico | 0 / 35 / 45 / 20 | 3 kHz | Podcast y narración |
| 10 | Aire a 20 kHz | Vocal | Aire | 0 / 0 / 20 / 55 | 6 kHz | Aire extra suave |
| 11 | Cinta suave | Master | Cinta | 15 / 15 / 25 / 30 | 5 kHz | Cohesión y calor sutil |
| 12 | Válvula calidez | Master | Válvula | 30 / 20 / 20 / 20 | 4 kHz | Mezclas frías o digitales |
| 13 | Transistor firme | Master | Transistor | 20 / 25 / 35 / 25 | 5 kHz | Pop y rock con pegada |
| 14 | Clásico apertura | Master | Clásico | 0 / 10 / 30 / 45 | 6 kHz | Abrir una mezcla apagada |
| 15 | Cuerpo y brillo | Master | Cinta | 40 / 20 / 30 / 35 | 5 kHz | Peso en graves y brillo en agudos |
| 16 | Apertura limpia | Master | Seda | 0 / 10 / 30 / 50 | 5 kHz | Mezcla pulida y transparente |
| 17 | Aire 40 kHz | Master | Aire | 0 / 0 / 15 / 60 | 6 kHz | Pop moderno; usa 8× y esquina de 30 kHz |
| 18 | Adaptativo balance | Master | Adaptativo | 10 / 20 / 30 / 30 | 5 kHz | Corregir balance tonal con suavidad |
| 19 | Dinámico mastering | Master | Dinámico | 10 / 20 / 30 / 35 | 5 kHz | Brillo sin dureza en picos |
| 20 | Ancho controlado | Master | Dinámico | 5 / 15 / 30 / 40 | 5 kHz | Ensancha solo lo añadido (Ancho 130 %) |

El preset Init deja Modo Vocal, Era Vintage, Clásico, los Drive por defecto de la sección 5 y Mezcla al 100 %.

### Gestor de presets

- Los de fábrica van incrustados (BinaryData) y son de solo lectura; los del usuario se guardan como XML del árbol de parámetros en la carpeta Application Support de By Ferb, subcarpetas Vocal y Master.
- Cada preset guarda todos los parámetros, incluidos Modo, Era y Voicing, y el tamaño de ventana no entra en el preset.
- Una marca de cambio avisa cuando el estado difiere del preset cargado; guardar sobre un preset de fábrica crea una copia del usuario.
- A/B guarda dos estados completos; copiar A a B es un clic.
- Para AU y VST3 se exponen los 20 presets como programas del host (getNumPrograms, setCurrentProgram).

## 8. Rendimiento, latencia y formatos

Objetivos iniciales, a validar con mediciones en tu Mac: Vocal a 48 kHz con 4× de oversampling (el valor por defecto) por debajo del 6 % de un núcleo, y Master a 4× con fase lineal por debajo del 15 %.

| Configuración | Latencia objetivo | Origen |
| --- | --- | --- |
| Vocal, 1×, crossover LR4 | ≤ 100 muestras | Lookahead de 2 ms de la guarda |
| Vocal, 2× | ≤ 140 muestras a 48 kHz | Guarda más filtros del oversampling |
| Master, 4×, LR4 | ≤ 80 muestras | Filtros del oversampling |
| Master, 4×, fase lineal | ≈ 1100 muestras (≈ 23 ms a 48 kHz) | FIR del crossover (2048 coeficientes) más oversampling |

La latencia se calcula en prepareToPlay, se comunica con setLatencySamples y updateHostDisplay, y la vía seca lleva un retardo igual para sumar en fase. Cambiar Fase lineal u Oversampling recalcula y avisa al host.

### Reglas de tiempo real

- Sin asignaciones ni bloqueos en processBlock; todos los búferes se reservan en prepareToPlay.
- ScopedNoDenormals y suavizado de 20 ms en todos los parámetros continuos.
- Frecuencias de muestreo de 44.1 a 192 kHz; layouts mono y estéreo (en mono, Ancho y Mono seguro no se aplican).
- Los coeficientes de los filtros se recalculan cada 16 muestras cuando el parámetro cambia.
- La FIFO del analizador es de un solo productor y un solo consumidor.

### Formatos

| Formato | Detalles |
| --- | --- |
| VST3 | Categoría Fx y Distortion; validación con pluginval nivel 10 |
| AU | Tipo aufx, subtipo McPo, fabricante Ferb; validación con auval |
| AAX | Categoría Harmonic; requiere cuenta de desarrollador de Avid, AAX SDK y firma PACE/iLok; sin firma solo carga en las versiones de desarrollo de Pro Tools |
| Standalone | Para pruebas rápidas fuera del DAW |

Firma ad-hoc con codesign para uso personal en macOS (VST3 y AU).

```cmake
juce_add_plugin(DraMcPoti
    COMPANY_NAME "By Ferb"
    PLUGIN_MANUFACTURER_CODE Ferb
    PLUGIN_CODE McPo
    FORMATS VST3 AU AAX Standalone
    PRODUCT_NAME "Dra. McPoti"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES "Fx" "Distortion"
    AAX_CATEGORY AAX_ePlugInCategory_Harmonic)
```

## 9. Plan de pruebas y mediciones

Las pruebas automatizadas con Catch2 cubren lo medible; el oído decide el resto. Los umbrales son iniciales y se revisan tras la primera medición real.

| Prueba | Método | Criterio |
| --- | --- | --- |
| Armónicos por carácter | Seno de 3 kHz a −18 dBFS, FFT de 65536 puntos, ventana Blackman-Harris | Pares o impares dominantes según la sección 4 |
| Aliasing | Seno de 9 a 20 kHz con Drive 100, medir energía no armónica | Por debajo de −80 dBFS a 4×; 2× a −70 dBFS |
| Reconstrucción del crossover | Ruido blanco, bandas sumadas con Drive 0 | Error de magnitud inferior a 0.1 dB; fase lineal exacta salvo retardo |
| Seco intacto | Mezcla 0 % | Salida idéntica a la entrada con la latencia declarada |
| Delta | Salida con Delta igual a la suma de lo añadido | Diferencia menor a −90 dB |
| Compatibilidad mono | Master, Mono seguro activo, suma L+R | Sin caída mayor a 1 dB por debajo de 150 Hz |
| Dinámica | Seno que sube de −40 a 0 dBFS | Drive efectivo cambia según Dinámica con la pendiente prevista |
| Guarda de sibilancia | Voz con S marcadas | Presencia y Aire bajan hasta 12 dB en S; sin cambios fuera de ellas |
| Level match | Mismo material con Igualar activo | Diferencia de sonoridad menor a 0.5 LU |
| Latencia | Impulso a través de cada configuración | Coincide con setLatencySamples |
| Estabilidad | 100 mil bloques con cambios aleatorios de parámetros, silencio, NaN | Sin clics, NaN ni consumo creciente |
| Hosts | pluginval nivel 10, auval, AAX Validator | Sin errores |
| Escucha A/B ciega | Con y sin Igualar, en ocho presets | Anotar preferencias y ajustar caracteres |

Mediciones a guardar en el repositorio como CSV: espectro de armónicos por carácter, curvas de fase y consumo de CPU por configuración.

## 10. Hitos, CLAUDE.md y primer prompt

El desarrollo va en seis fases; cada una termina cuando pasa su prueba de la sección 9 y no se empieza la siguiente antes.

&#91;embedded content: hoja de ruta · 6 fases con criterio de salida\]

Los rombos marcan el criterio que cierra cada fase. Los umbrales son los de la sección 9 y se revisan tras la primera medición real.

### Bloque para CLAUDE.md

```markdown
# Dra. McPoti (By Ferb)

Excitador armónico multibanda con modos Vocal y Master y eras Vintage y Moderno.
Formatos: VST3, AU, AAX y Standalone. JUCE 8 con CMake (FetchContent).

## Reglas
- Sin asignaciones ni bloqueos en processBlock; ScopedNoDenormals; suavizado de 20 ms.
- Los IDs de parámetros de la sección 5 de la ficha no se renombran.
- Target CMake DraMcPoti, producto "Dra. McPoti", AU aufx McPo / Ferb.
- La latencia se declara con setLatencySamples y updateHostDisplay; el seco lleva el mismo retardo.
- Los coeficientes de los caracteres son valores iniciales: medir antes de afinar.
- Los presets de fábrica salen de la tabla de la sección 7.

## Estructura
Source/DSP (Crossover, Generators, Dynamics, Compander, Shelves, Guard, PhaseNetwork, Stereo, LevelMatch)
Source/UI (maqueta de rack: perillas numeradas, válvulas, vumétros, ventana de armónicos)
Source/Presets, Tests (Catch2)

## Validación
pluginval nivel 10, auval, AAX Validator y las pruebas de la sección 9.
```

### Primer prompt para Claude Code

```text
Lee la ficha técnica de Dra. McPoti y la maqueta HTML adjunta. Empieza solo por la fase 1:
1. Crea el proyecto JUCE 8 con CMake y los formatos VST3, AU y Standalone (AAX queda para la fase 6).
2. Declara los 44 parámetros de la sección 5 con AudioProcessorValueTreeState y sus IDs exactos.
3. Implementa la interfaz con el aspecto de la maqueta: rack gris pizarra, perillas con escala 0 a 10, cuatro válvulas, dos vumétros, ventana de armónicos y redimensionado del 75 % al 200 %.
4. Implementa el gestor de presets con los 20 de fábrica y Init, y A/B.
5. El audio pasa sin procesar; todavía no hay DSP.
Cuando compile, pásame la salida de pluginval nivel 5 antes de seguir con la fase 2.
```

## 11. Estado y cambios sobre la ficha

Esta sección manda sobre el resto del texto. Estado a 2026-10-31: **fase 1 hecha** (esqueleto, 44 parámetros, presets, panel provisional) y **fase 2 avanzada** (cruce de cuatro bandas, caracteres Vintage con ADAA y sobremuestreo de 1× a 8×, los cuatro caracteres Modernos, latencia constante). Las fases 3 a 6 están pendientes.

### Hecho en la fase 1
- Carpeta `DraMcPoti/` (sin punto ni espacio) con la estructura del monorepo; versión **0.1.0** en `cmake/Versions.cmake` (1.0.0 al cerrar la fase 6). Target CMake `DraMcPoti`, producto «Dra. McPoti», AU `aufx` / `McPo` / `Ferb`.
- 44 parámetros con los IDs de la sección 5 (el 44.º es `bypass`, el del anfitrión, que no entra en los presets). `voice` es un Choice de cuatro valores (índice 0 a 3); los nombres que muestra la interfaz dependerán de la era.
- 20 presets de fábrica más Init, también expuestos como programas del anfitrión. Los de Vocal llevan Guarda 60 %; los de Master, Guarda 0 %.
- Panel provisional con el kit `ui/vintage/` (paleta de Cremita): los 44 controles, sin válvulas, vúmetros ni ventana de armónicos (fase 5).
- Pruebas `DraMcPotiProcessorTests`: parámetros aleatorios, tasas y bloques extremos, estado, vida del editor, canales, cero reservas en `processBlock`, 44 parámetros, los 21 programas cargan y el audio pasa idéntico con latencia 0.

### Hecho de la fase 2 (parcial)
- `ferb::FourBandSplit` en `core/Crossover.h`: cuatro bandas LR4 (se parte en xo2 y cada mitad en xo1 y xo3, con el paso todo del cruce vecino en cada banda). Prueba: la suma tiene magnitud plana dentro de ±0,1 dB en 44,1, 48 y 96 kHz y con tres juegos de cruces (cumple el criterio de la sección 9).
- `McPotiEngine` (`source/dsp/`): entrada y salida, cruce, generador `HP → f_v → HP` de los cuatro caracteres Vintage (Clásico, Cinta simplificada, Válvula y Transistor), dinámica por banda con la fórmula de la sección 3 (RMS enlazado con Enlace), Nivel de banda, Activar, Solo, Delta y Mezcla. Cruces y Sintonización suavizados (30 ms).
- Pruebas `DraMcPotiTests` (7): Drive 0 y Mezcla 0 % dejan la señal intacta (< −90 dB); seco + Delta = salida completa (< −90 dB); Válvula con 2.º armónico mayor que el 3.º, Transistor con el 3.º más de 4 veces el 2.º, Clásico con ambos; la dinámica sube lo añadido en pasajes suaves con + y en picos con −; Solo; sin NaN ni clics al mover Drive, Dinámica y cruces.
- El procesador resuelve los punteros a los parámetros en el constructor: buscarlos por texto en `processBlock` reservaba memoria (lo detectó la prueba de cero reservas).
- **Sobremuestreo y ADAA (2026-10-31):** el generador (subir → no linealidad con ADAA de primer orden → pasa-altos → bajar) trabaja a 1×, 2×, 4× u 8× con `core/OversamplerN.h` (fase lineal). Las cuatro no linealidades Vintage tienen antiderivada cerrada. Latencia **constante** de 47 muestras (la de 8×): el seco, Solo y los factores menores se retrasan lo que falta. Cambiar el sobremuestreo o el Voicing con audio funde lo añadido a silencio (5 ms), cambia y vuelve: sin clics.
- **Aliasing medido** (seno de 5, 7,9 y 9,1 kHz a 0,5, Drive 100 en las cuatro bandas, los cuatro caracteres, hasta 20 kHz): 1× **−41,6 dB**; 2× **−63,3 dB**; 4× y 8× **−96,5 dB**. **Meta de la ficha: −70 a 2× y −80 a 4×. A 2× NO se cumple** (faltan unos 7 dB en este peor caso); 4× y 8× cumplen con margen. El 2× es el valor por defecto de la ficha: decidir si pasa a 4× o se sube el orden del ADAA. La prueba exige −60 a 2×, el valor medido con margen, no la meta.
- **CPU medida** (Release, arm64, 48 kHz, estéreo, bloques de 256, Drive de fábrica, Válvula; `DraMcPotiTests "[.report]"`): 1× 0,7 %, 2× 2,0 %, 4× 3,0 %, 8× 4,8 % de un núcleo, incluyendo el ruido de prueba. Cumple el objetivo Vocal 2× (< 6 %). Sin fase lineal ni Master completo todavía.
- **Decisión (2026-10-31): Oversampling por defecto = 4×** (antes 2×), en el parámetro y en los 21 presets (el de «Aire 40 kHz» sigue en 8×). Con 4× el aliasing medido es −96,5 dB (meta −80) y la CPU de 3,0 % de un núcleo, dentro del objetivo del 6 %. 2× sigue disponible (−63,3 dB, por debajo de su meta de −70); la interfaz definitiva debería avisarlo.
- **ADAA de segundo orden: descartado por ahora.** Necesita la segunda antiderivada de cada no linealidad. Solo el Transistor la tiene cerrada (`(u·√(1+u²) + asinh u)/2`); para `tanh` (Clásico, Cinta y Válvula) es la dilogaritmia, sin forma elemental, y exigiría una tabla numérica. Con 4× por defecto no hace falta; reabrir solo si se quiere 2× a −70 dB.
- **Seda (era Moderno, voicing 3) hecho (2026-10-31):** shelf de segundo orden de +15 dB en el borde inferior de cada banda (Cuerpo: 100 Hz; Medios, Presencia y Aire: xo1, xo2 y xo3), sumado como realce a la señal intacta; no sobremuestrea. **Drive mezcla el realce de forma lineal** (100 = shelf completo, +15 dB), no es una ganancia en dB, para no recalcular coeficientes por muestra. Medido: a 12 kHz con Drive 100 el realce total es de 13,1 dB (Aire, shelf en 7 kHz), sin armónicos (< 1 % del fundamental) y con Drive 0 no añade nada; con latencia constante. El shelf se rediseña al mover los cruces (suavizados). Esquinas y +15 dB son valores iniciales.
- **Dinámico, Adaptativo y Aire hechos (2026-10-31):**
  - **Dinámico:** compander por banda sin generador: suma una copia comprimida de la banda al original (umbral fijo de −20 dB sobre la envolvente de la banda; relación de 2:1 a 8:1 según Dinámica; Drive = cantidad de la copia). Medido: bajo el umbral la copia suma ≈ la banda entera; a −6 dB suma menos del 70 % de eso; no crea armónicos; con 8:1 comprime más que con 2:1. **Diferencia con la ficha:** el umbral es fijo, no lo fija Drive con ganancia automática.
  - **Aire:** shelf de +10 dB en la esquina de aire, que se aplana cuando la envolvente de la banda supera −24 dB (relación 4:1). Medido con la esquina en 8 kHz: a 12 kHz realza más de 1× lo de entrada con la señal suave y menos del 80 % de eso con la fuerte; graves casi sin realce; sin armónicos. **Diferencias con la ficha:** (1) trabaja a la frecuencia base y la esquina se acota a 0,45·fs (21,6 kHz a 48 kHz), así que las esquinas de 22 a 40 kHz no hacen lo que dice la ficha hasta que el shelf pase al dominio sobremuestreado; (2) el 5 % de armónicos pares no está.
  - **Adaptativo:** Válvula con el drive multiplicado por una ganancia de hasta ±6 dB según la razón Presencia/Medios (promedio de 2 s) frente a un objetivo de −9 dB (valor inicial). Medido con una señal apagada (presencia 20 dB bajo el medio) el 2.º armónico sube más de 1,3× respecto de la Válvula fija, y con una brillante baja a menos de 0,8×.
  - Los tres llegan con latencia constante y fundido sin clics al cambiar de carácter (Dinámico y Aire no sobremuestrean). Las medidas de las pruebas son de dirección y orden de magnitud, sin afinar de oído.
- **Color, Fase, Suavizar, Ancho y Mono seguro hechos (2026-10-31):**
  - **Suavizar:** paso bajo de 2.º orden sobre lo añadido (suavizado 30 ms); a 20 kHz queda apagado. Medido: con corte en 4 kHz el 3.er armónico de un seno de 3 kHz cae a menos del 30 %.
  - **Fase:** cuatro secciones pasa-todo (600, 1800, 5000 y 11000 Hz) solo en lo añadido; el mando interpola entre la etapa k y la k+1 (transición continua). Medido: misma energía (±0,5 dB) con 0 % y 100 %, otra forma de onda, y 1 % más de Fase casi no cambia nada. **Consecuencia:** a mitad de una transición el módulo baja un poco a algunas frecuencias (se suman dos copias con fases distintas); las pruebas de los caracteres fijan Fase en 0.
  - **Color:** hacia −100 desplaza el punto de trabajo de la no linealidad (sesgo de 0,35·|Color|, suma pares) y hacia +100 mezcla con la parte impar `(f(u) − f(−u))/2` (una segunda rama con ADAA propio). Medido en Transistor: con −100 el 2.º armónico es más de 10 veces el de Color 0; con +100 no sube. **Diferencia con la ficha:** no es un reparto «propio de cada carácter» sino este sesgo y esta mezcla, iguales para los cuatro generadores. No actúa en Dinámico, Seda ni Aire (no generan armónicos).
  - **Ancho y Mono seguro (solo Master, estéreo):** M/S sobre lo añadido; Ancho escala el lateral de 0 a 200 % (medido: 0 % deja el lateral nulo y 200 % lo duplica) y Mono seguro quita del lateral lo que queda bajo 150 Hz (medido: a 60 Hz el lateral baja a menos del 20 %). El seco no se toca.
- **Guarda de sibilancia e Igualar hechos (2026-10-31):**
  - **Guarda (solo Vocal):** detector propio y simplificado (no el de ChauS): compara la energía de 5–10 kHz con la de 300 Hz–3 kHz (envolvente de 2 ms) con histéresis (activa si la razón supera 0,5, se suelta bajo 0,25) y baja lo añadido en Presencia y Aire hasta 12 dB × Guarda, con ataque de 1 ms y relajación de 40 ms. **Diferencias con la ficha:** sin planitud espectral y **sin lookahead propio de 2 ms**: el detector mira la entrada sin retrasar y lo añadido llega con los 47 muestras de latencia constante (≈ 1 ms a 48 kHz), así que anticipa 1 ms, no 2. Umbrales sin afinar con voz real. Medido: un seno de 7 kHz a 0,2 baja más de 9 dB con Guarda 100 % y una vocal de 800 Hz queda igual (±5 %).
  - **Igualar:** integradores de 3 s con ponderación K (la de `core/Loudness.h`) de la entrada y de la salida; la ganancia (±12 dB) lleva la salida a la sonoridad de la entrada, se suaviza y se apaga en Delta y Solo. La interfaz puede leerla con `McPotiEngine::matchGainDb()`. Medido con ruido y Drive 100 en Transistor: con Igualar la diferencia queda dentro de 1 dB y sin él es mayor. **Diferencia con la ficha:** el integrador es un promedio exponencial de 3 s, no el de ventana de Cremita, y no está probado contra el criterio de 0,5 LU.
- **Fase lineal hecha (2026-10-31):** `LinearSplit` (`source/dsp/LinearSplit.h`): tres paso-bajos FIR de **2049 coeficientes** (sinc con ventana de Blackman) en xo1, xo2 y xo3; las bandas son diferencias de paso-bajos y la última es la entrada retardada menos el paso-bajo más alto, así que **suman exactamente la entrada retardada 1024 muestras** (medido: residuo < −100 dB). Cada banda domina en su zona con un margen mayor que el LR4 (el vecino queda por debajo del 10 %). Convolución **directa** (no por FFT: tres productos punto de 2049 muestras por canal, vectorizados), con lo que no hay particiones ni latencia extra; la FIR se rediseña como mucho cada 128 muestras mientras se mueven los cruces (ya suavizados 30 ms). **CPU medida** (Release, arm64, 48 kHz, estéreo, 4×, Drive de fábrica): 3,6 % sin fase lineal y **11,6 %** con ella (objetivo: < 15 %).
  - **Excepción a la latencia constante (decisión):** la fase lineal **sí cambia la latencia** (+1024 muestras ≈ 21 ms a 48 kHz: 1071 en total); el sobremuestreo y el carácter siguen sin cambiarla. Declararla siempre habría añadido 21 ms a todo uso en directo. El procesador la recalcula, llama a `setLatencySamples` y a `updateHostDisplay`, y la prueba con impulso confirma que la declarada coincide con la medida para 1×, 4× y 8×, con y sin fase lineal. Al activarla o desactivarla se funde TODA la salida (5 ms), se realinean los retardos y se mantiene el silencio hasta que el seco vuelve a llenarse (≈ 25 ms); sin clics (prueba con seno y conmutaciones cada 0,4 s). La guarda se retrasa lo mismo que lo añadido.
  - **Límites:** la transición de la FIR es de ≈ 5,5·fs/2049 (129 Hz a 48 kHz, 516 Hz a 192 kHz), así que con cruces bajos (80 a 150 Hz) el reparto entre Cuerpo y Medios es menos nítido que con LR4; la FIR de 2049 coeficientes es fija a cualquier frecuencia de muestreo.
- **Plantilla por modo hecha (2026-10-31):** al cambiar el Modo, `DraMcPotiProcessor::syncModeTemplate` mueve Sintonización, Guarda, Ancho, Mono seguro, los tres cruces y el Oversampling a los valores del nuevo modo (Vocal: 4 kHz, 60 %, 100 %, off, 150/1800/7000 Hz, 4×; Master: 5 kHz, 0 %, 100 %, on, 120/1600/6500 Hz, 4×; valores iniciales por afinar de oído). **Definición de «tocado» (diferencia con la ficha):** un control cuenta como tocado si ya no vale lo que valía en la plantilla del modo anterior. Así no hace falta guardar qué se tocó, funciona igual tras restaurar una sesión y un preset que fija un valor propio (p. ej. «Radio setentas» con 4,5 kHz) se respeta. Una sesión restaurada no recibe plantilla. Probado en el procesador: un control movido se respeta y vuelve a la plantilla al volver al modo original los que no se tocaron.
- **Cinta con pre-énfasis hecha (2026-10-31):** shelf de +6 dB desde 5 kHz antes de la no linealidad (tanh con ADAA) y shelf inverso después, ambos a la frecuencia sobremuestreada. Efecto medido: el aliasing de peor caso a 2× pasó de −63,3 a **−61,1 dB** (4× y 8× siguen en −96,5 dB); la prueba exige −60 dB a 2×.
- **5 % de pares de Aire: descartado (decisión).** Sale de elevar al cuadrado la banda en la frecuencia base: una banda de 7 a 20 kHz al cuadrado ocupa hasta 40 kHz y pliega aliasing dentro de la banda audible, justo lo que el diseño evita. Hacerlo bien exigiría llevar el shelf de Aire al dominio sobremuestreado, junto con las esquinas de 22 a 40 kHz, que tampoco funcionan hoy. Se deja como trabajo futuro conjunto.
- **Sin implementar todavía (motor):** nada de lo que la ficha pide salvo lo descartado arriba (5 % de pares y esquinas de Aire sobre Nyquist).

### Interfaz (fase 5, parcial, 2026-10-31)
- **Hecho:** cuatro válvulas de vidrio que brillan con lo que añade cada banda (`McPotiEngine::addedLevel`; escala inicial de −60 a −20 dB de RMS absoluto, **no** en dB relativos a la entrada de la banda como pide la ficha), nombre del carácter bajo el selector (cambia con la era) y atenuación de lo que no actúa: Guarda en Master; Ancho y Mono seguro en Vocal; Esquina de aire sin el carácter Aire. Un temporizador de 20 Hz lo actualiza. Comprobado con la captura de `DraMcPotiSnapshot`.
- **Vúmetros hechos:** dos `VuNeedle` del kit (entrada y salida) alimentados por `core/VuBallistics.h` dentro del motor (65 ms, calibrado a RMS); 0 VU = parámetro Referencia VU. La salida mide después de Igualar y del mando Salida. **Diferencia con la ficha:** el vúmetro de salida aún no indica si Igualar está activo. Comprobado solo con la captura (aguja en reposo con el audio de prueba), no con señal de nivel conocido.
- **Ventana de armónicos hecha:** `Harmonics` (cristal del kit) con FFT de 2048 puntos y ventana de Hann en el hilo de interfaz, alimentada por dos `SampleRing` (entrada y salida en mono) que el procesador reserva en el constructor y llena sin asignar. Traza tenue = entrada, ámbar = salida, cortes verticales en los tres cruces y línea punteada en la Sintonización; eje logarítmico de 40 Hz a 20 kHz y rango de −90 a 0 dB. **Diferencia con la ficha:** la FFT usa el máximo por columna (sin suavizado espacial) y un promedio temporal simple (0,6).
- **Cambio de tamaño base:** la interfaz pasa de 1400 × 540 a **1400 × 600** para alojar la ventana; el redimensionado de 75 % a 200 % sigue igual.
- **Pendiente de la interfaz:** rotulado de Drive como «Ganancia del shelf» con Seda y Aire (la perilla del kit no permite cambiar su etiqueta en marcha), perillas de cruce entre los módulos (hoy están bajo ellos), selector de pala, marco y rotulado fieles a la maqueta con paleta propia (hoy usa la de Cremita), y el rotulado de Drive. **Hechos después:** chips de tamaño (75, 100, 150 y 200 %) y válvulas a media luz con Bypass.

### Cambios sobre la ficha
- **Clásico acotado:** `f_clásico` usa `0,25·tanh²(u)` en lugar de `0,25·(u² − media)`. El término cuadrático de la ficha no tiene tope y con Drive alto y ruido a +12 dB llegaba a amplitud 70. `tanh²` coincide con `u²` para señales pequeñas, está acotado y su antiderivada es exacta (`u − tanh u`). El DC lo quita el pasa-altos posterior.
- **Latencia constante** (regla del repo, manda sobre la sección 8; con la excepción de la fase lineal, ver arriba): se declara la del caso máximo y Oversampling y carácter no la cambian; lo que falte se retrasa en el seco. Las cifras de la tabla de la sección 8 son objetivos de CPU y de estimación, no latencias que cambien con el modo.
- **Motor reutilizado, no reescrito:** sobremuestreo con `core/OversamplerN.h` (no `juce::dsp::Oversampling`), Igualar con `core/Loudness.h`, VU con `core/VuBallistics.h`, ADAA con `core/Adaa.h`, cruce con `core/Crossover.h`, analizador con `core/Fft.h` y cambios de estructura con `ModeSwitch`. La guarda sale de ChauS y la estructura multibanda, de Poty (por revisar al empezar la fase 2).
- **Fase lineal:** una FIR directa de 2048 coeficientes por banda no cabe en el 15 % de CPU; usar convolución por FFT particionada. Mover un cruce con fase lineal rediseña la FIR: hacerlo con fundido (`ModeSwitch`) para evitar clics.
- **Esquina de aire:** acotarla según el sobremuestreo activo (con 1× o 2× una esquina de 40 kHz cae sobre Nyquist) y mostrarlo en la interfaz.
- **Seco intacto:** la fórmula `y_b = x_b + …` de la sección 4 describe la ruta de cada banda; a la salida el seco nunca se procesa y solo se suma lo añadido (sección 3).
- **Mono seguro** arranca en off (el modo por defecto es Vocal); los presets Master lo encienden. El modo Master aplica la plantilla de la sección 5.
- **Plantilla por modo** («un control tocado se respeta»): requiere guardar qué controles tocó el usuario, también en el estado de la sesión. Sin implementar.
- **Aspecto:** pizarra, no vino; el panel provisional usa la paleta de Cremita hasta la fase 5.
- Validación a **nivel 10** de pluginval (el prompt original decía 5).

### Pendiente
Terminar la fase 2 y las fases 3 a 6 de la sección 10; afinar de oído los coeficientes iniciales; probar en Ableton, Logic y Pro Tools; documentación en PDF; AAX sin validar ni firmar.
