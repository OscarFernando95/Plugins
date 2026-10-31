# Nel — ficha técnica del limitador

Oct 31, 2026 · @Oscar Fernando

## Resumen

Nel es un limitador de masterización con true peak, medición de sonoridad completa e interfaz de equipo analógico, en VST3, AU y AAX para macOS, de uso personal. Su referencia funcional es el FabFilter Pro-L 2; de otros limitadores toma las ideas que se indican en la sección de mercado.

Principios de diseño:

- **Sin aliasing ni sobre inter-muestra:** sobremuestreo ajustable, 4× por defecto, y limitación de true peak.
- **Modos con carácter definido:** cada modo es un conjunto medible de tiempos de ataque, liberación y suavizado, no un nombre de marketing.
- **Medición que se puede confiar:** LUFS según ITU-R BS.1770, verificados contra señales de prueba de la EBU.
- **Pocos controles, bien definidos:** lo básico a la vista y lo avanzado en un panel plegable.
- **Bajo consumo de CPU** y latencia informada al anfitrión.
- **Interfaz** de aparato analógico vintage en morado, redimensionable.

Nombre del producto: **Nel**. Etiqueta del creador, como en Tufi y Poty: **By FERB**.

## Mercado: qué toma Nel de cada limitador

Pro-L 2 es la referencia funcional. Para decidir el resto se revisaron ocho limitadores; la última columna dice qué entra en Nel y qué se deja fuera.

| Limitador | Qué lo distingue | Qué toma Nel |
| --- | --- | --- |
| [FabFilter Pro-L 2](https://www.fabfilter.com/help/pro-l) | Ocho estilos (Transparent, Punchy, Dynamic, Allround, Aggressive, Modern, Bus y Safe), lookahead, ataque y release ajustables, enlace de canales separado para transitorios y para release, true peak, oversampling de 2× a 32×, dither con tres perfiles de noise shaping, Unity Gain, Audition Limiting, filtro de DC, medición LUFS (momentary, short term, integrated y LRA), escalas K-System, deshacer y A/B | El núcleo completo: ocho modos, doble enlace, Audición, Ganancia unitaria, oversampling de 1× a 32× y medición LUFS |
| [Sonnox Oxford Limiter V3](https://www.production-expert.com/production-expert-1/7-superb-plugin-limiters-worth-checking-out-in-2023) | Función Enhance que limita a nivel de muestra para subir la sonoridad percibida, soft knee ajustable, dither con cuatro curvas de noise shaping y medidores en modo true peak. Una [prueba comparativa](https://www.sageaudio.com/articles/top-5-mastering-limiters-tested) lo describe como el más cercano a un limitador analógico | Knee ajustable, una curva de realce dentro del control Carácter y noise shaping en el dither |
| [TDR Limiter 6 GE](https://www.production-expert.com/production-expert-1/7-superb-plugin-limiters-worth-checking-out-in-2023) | Cadena de clipper, compresor y dos limitadores; un limitador de agudos con shelf de 1,5 a 18 kHz contra hi-hats y sibilancia; limitador de picos de una o varias bandas, brickwall o no | Recorte suave previo dentro de cada modo. El limitador de agudos queda para la versión 2 |
| [Avid Pro Limiter](https://www.production-expert.com/production-expert-1/7-superb-plugin-limiters-worth-checking-out-in-2023) | Solo AAX; release manual o automático, control Character de saturación analógica e histograma de sonoridad en el tiempo | Control Carácter y vista histórica de sonoridad |
| [Sonible smart:limit y pure:limit](https://www.production-expert.com/production-expert-1/7-superb-plugin-limiters-worth-checking-out-in-2023) | Análisis con IA de un género o de una referencia, objetivo en LUFS de streaming y comprobación de calidad contra bases de datos; pure:limit añade el control Inflate | Objetivo de LUFS con la diferencia en LU. No lleva IA |
| [Signum Audio BUTE Loudness Suite 2](https://www.production-expert.com/production-expert-1/7-superb-plugin-limiters-worth-checking-out-in-2023) | Limitador de solo tres controles que usa waveshaping en lugar de un seguidor de envolvente; historial de true peak, short term, momentary y RMS de hasta diez horas | Historial de sonoridad de hasta 10 h y la idea de pocos controles principales |
| [Flux Elixer](https://www.sageaudio.com/articles/top-5-mastering-limiters-tested) | Hasta cuatro etapas de limitación: más etapas dan una limitación más precisa; mezcla A/B | A/B integrado. Nel usa dos etapas más un seguro de pico |
| [Melda MLimiterX](https://www.sageaudio.com/articles/top-5-mastering-limiters-tested) | Tres secciones (Maximizer, Pumping y Phattening) y un Drive con timbres Clean, Punchy y Hard; proceso mid/side | Un timbre limpio, punchy o duro asociado a cada modo. Mid/side queda para la versión 2 |

Lo que reúne Nel, sin prometer que sea mejor que ninguno de ellos antes de medirlo:

- Sobremuestreo de 4× por defecto, con limitación de true peak.
- Medidor de sonoridad verificable con las señales de prueba de la EBU.
- Historial de sonoridad y relación pico a sonoridad (PLR).
- Interfaz de aparato analógico con VU de aguja y perillas de baquelita, redimensionable.
- Control Carácter para que la saturación sea una decisión del usuario y no un efecto oculto.

Fuentes de esta sección: [ayuda de FabFilter Pro-L](https://www.fabfilter.com/help/pro-l), [ajustes avanzados de Pro-L 2](https://www.fabfilter.com/help/pro-l/using/advancedsettings), [reseña de MusicRadar](https://www.musicradar.com/reviews/fabfilter-pro-l-2), [7 limitadores en Production Expert](https://www.production-expert.com/production-expert-1/7-superb-plugin-limiters-worth-checking-out-in-2023) y [Top 5 de Sage Audio](https://www.sageaudio.com/articles/top-5-mastering-limiters-tested).

## Alcance y formatos

| Tema | Decisión |
| --- | --- |
| Sistema | macOS 11 o superior, binario universal (arm64 y x86\_64) |
| Formatos | VST3, AU (AUv2, tipo efecto) y AAX Native para Pro Tools |
| Canales | Mono y estéreo en la versión 1 |
| Frecuencias de muestreo | 44,1 a 192 kHz, bloque de tamaño variable |
| Ventana | Redimensionable de 75 % a 200 %, con proporción fija |
| Presets | Banco de fábrica, presets de usuario, A/B, deshacer y rehacer |
| Distribución | Solo uso personal |
| Fuera de alcance en la versión 1 | Sidechain externo, surround y Atmos, mid/side, multibanda, limitador de agudos, AudioSuite y funciones con IA |

### AAX es el formato con más fricción

Para compilar AAX hace falta el AAX SDK de Avid, que se obtiene con una cuenta del programa de desarrolladores de Avid, sujeta a su aprobación. Un AAX sin firmar solo carga en las compilaciones de desarrollo de Pro Tools; las versiones comerciales lo rechazan. Para firmarlo se usa la herramienta wraptool de PACE, con cuenta y licencia de iLok y una licencia de desarrollador de Avid; en macOS puede hacer falta además un certificado de Apple. Según el [hilo de los foros de JUCE](https://forum.juce.com/t/building-aax-code-signing/14055), parte de estos detalles está bajo acuerdo de confidencialidad y se consulta en el foro de desarrolladores de Avid.

Para VST3 y AU de uso personal basta con firma ad-hoc, como en Tufi y Poty; AAX no funciona así.

**Decisión:** VST3 y AU se construyen primero y AAX es el último hito, después de confirmar las condiciones de Avid. Hasta entonces, la lógica de audio no depende del formato, para que el AAX se añada al final sin tocar el DSP.

## Parámetros

Todos pasan por `AudioProcessorValueTreeState` con `ParameterID{id, 1}`. Son 21 parámetros automatizables; el estado de la interfaz se guarda aparte. Los tiempos usan escala logarítmica en el control.

**Principales**

| ID | Nombre en la interfaz | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `gain` | Ganancia | Float | 0 a +24 dB | 0 | Empuja la señal contra el techo |
| `ceiling` | Techo | Float | −30 a 0 dBTP | −1,0 | Máximo true peak de salida. −1 dBTP es el límite que recoge EBU R 128 para producción |
| `mode` | Modo | Choice | Moderno, Transparente, Punchy, Dinámico, Versátil, Agresivo, Bus, Seguro | Moderno | Al cambiarlo se cargan los valores del modo en los parámetros avanzados |
| `character` | Carácter | Float | 0 a 100 % | 0 | Saturación suave previa al limitador |

**Avanzados** (los valores iniciales de cada modo están en la sección siguiente)

| ID | Nombre en la interfaz | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `lookahead` | Lookahead | Float | 0 a 5 ms | Según el modo | Por debajo de 0,1 ms se comporta casi como un recorte duro |
| `attack` | Ataque | Float | 0,1 a 50 ms | Según el modo | Ataque de la etapa de cuerpo |
| `release` | Release | Float | 10 a 2000 ms | Según el modo | Constante lenta; la rápida es la lenta dividida entre 12 |
| `releaseAuto` | Release automático | Bool | apagado o encendido | Encendido | Release que depende del programa |
| `knee` | Knee | Float | 0 a 12 dB | Según el modo | Codo de la etapa de cuerpo |
| `linkTransient` | Enlace de transitorios | Float | 0 a 100 % | 75 | 100 % = el canal más alto controla a los dos |
| `linkRelease` | Enlace de release | Float | 0 a 100 % | 100 | Empezar en 100 % |

**Calidad y salida**

| ID | Nombre en la interfaz | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `truePeak` | True Peak | Bool | apagado o encendido | Encendido | Limita el true peak y no solo el pico de muestra |
| `oversample` | Sobremuestreo | Choice | 1×, 2×, 4×, 8×, 16×, 32× | 4× | Ver sección de sobremuestreo |
| `osPhase` | Fase del sobremuestreo | Choice | Lineal, Mínima | Lineal | Lineal = más latencia; mínima = latencia baja |
| `osOffline` | Más calidad en render | Bool | apagado o encendido | Encendido | En render offline sube un escalón de sobremuestreo |
| `ditherBits` | Dither | Choice | Apagado, 16, 18, 20, 22 y 24 bits | Apagado |  |
| `ditherShape` | Perfil de dither | Choice | Plano, Suave, Medio, Fuerte | Plano | Noise shaping progresivo |
| `dcFilter` | Filtro de DC | Bool | apagado o encendido | Apagado | Paso alto de primer orden a 5 Hz antes del limitador |

**Escucha y paso**

| ID | Nombre en la interfaz | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `unityGain` | Ganancia unitaria | Bool | apagado o encendido | Apagado | Compensa la salida con la ganancia aplicada, para comparar a igual volumen |
| `audition` | Audición | Bool | apagado o encendido | Apagado | Se oye solo la diferencia entre entrada y salida, es decir, lo que el limitador elimina |
| `bypass` | Bypass | Bool | apagado o encendido | Apagado | Fundido de 10 ms con la latencia compensada |

**Estado de la interfaz** (se guarda con el proyecto, no se automatiza)

| Clave | Valores | Defecto |
| --- | --- | --- |
| `lufsTarget` | −30 a 0 LUFS; atajos −9, −14, −16, −23 y −24 | −14 |
| `lufsScale` | Absoluta (LUFS) o relativa (LU) | Absoluta |
| `meterScale` | Estándar de −16, −32 o −48 dB; K-12, K-14 o K-20 | Estándar de −32 dB |
| `loudnessSource` | Salida o entrada | Salida |
| `rmsRef` | Plana o AES17 (+3 dB) | Plana |
| `uiScale` | 0,75 a 2,0 | 1,0 |
| `advancedOpen`, `abSlot`, `presetName` | Estado del panel avanzado, ranura A o B y nombre del preset | Cerrado, A, «Init» |

## Modos de limitación

Nel tiene ocho modos con los mismos caracteres que describe FabFilter para los estilos de Pro-L 2 ([ajustes avanzados](https://www.fabfilter.com/help/pro-l/using/advancedsettings)), pero cada uno se define aquí como un conjunto de números que se puede medir y corregir. Los valores son iniciales: se afinan de oído y con las pruebas de la sección de validación.

Cada modo fija estos parámetros visibles (el usuario puede cambiarlos después) y tres internos que no se muestran: el recorte suave previo, el margen del cuerpo respecto del techo y un extra propio del modo.

| Modo | Carácter buscado | Lookahead (ms) | Ataque (ms) | Release lento (ms) | Knee (dB) | Recorte suave previo | Extra interno |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Moderno | Mucha sonoridad sin ensuciar; modo por defecto | 1,0 | 3 | 500 | 3 | 20 % | Margen del cuerpo de 1 dB |
| Transparente | Lo más cercano al original, sin bombeo | 3,0 | 10 | 1200 | 6 | 0 % | Suavizado de ganancia al máximo |
| Punchy | Pegada; bombea al forzarlo | 1,0 | 25 | 300 | 1 | 10 % | El ataque lento deja pasar el transitorio hasta la etapa de picos |
| Dinámico | Conserva y realza transitorios | 2,0 | 6 | 700 | 4 | 0 % | Realce de transitorios de +2 dB antes de limitar |
| Versátil | Equilibrio para casi cualquier material | 2,0 | 8 | 800 | 4 | 5 % |  |
| Agresivo | Casi recorte, máxima sonoridad | 0,2 | 1 | 250 | 0 | 45 % | Margen del cuerpo de 0 dB |
| Bus | Pegamento; no es transparente | 1,5 | 20 | 600 | 8 | 0 % | Detector con paso alto a 80 Hz y saturación fija del 10 % |
| Seguro | Sin distorsión audible, para material delicado | 5,0 | 15 | 2000 | 10 | 0 % | Etapa de picos con ventana completa y sin recorte |

Los tiempos de la tabla se refieren al release lento. Con el release automático encendido, el release real varía entre el lento y un rápido igual al lento dividido entre 12, según lo sostenida que sea la reducción de ganancia.

Al cambiar de modo se escriben en los parámetros avanzados los valores de la tabla, con un fundido de 50 ms en la ganancia para evitar clics. Si el usuario modifica un valor avanzado, el nombre del modo se muestra con un asterisco hasta que lo vuelva a elegir.

## Arquitectura DSP

&#91;embedded content: flujo de señal de Nel · 2 etapas, seguro de pico y medidores\]

La etapa de picos (A) es la que garantiza el techo; la etapa de cuerpo (B) hace el trabajo lento para que A solo recorte lo que sobra. Todo lo que está dentro del recuadro punteado corre a la frecuencia sobremuestreada.

- **Orden:** ganancia y carácter, sobremuestreo, etapa B, etapa A, seguro de pico, submuestreo y dither.
- **Enlace estéreo:** el detector combina los canales con `linkTransient` en la etapa A y con `linkRelease` en la etapa B.
- **Audición:** emite la entrada retrasada menos la salida, para oír solo lo que se elimina.
- **Ganancia unitaria:** resta a la salida la ganancia aplicada, para comparar a igual volumen.
- **Medidores:** la copia de la salida pasa a la interfaz por una cola sin bloqueos; el audio nunca espera a la interfaz.
- **Bypass:** cruza al audio original retrasado lo mismo que la latencia total.

## Algoritmo del limitador

Todo se calcula a la frecuencia sobremuestreada `fsOS`, salvo que se indique otra cosa. Las fórmulas y valores son una propuesta de diseño para validar con las pruebas; no describen el algoritmo de ningún producto comercial.

### Etapa A: picos con lookahead

1. **Nivel requerido.** `p[n]` es el pico combinado de los canales: mezcla entre el máximo común y el de cada canal, según `linkTransient`. La ganancia requerida es `g[n] = min(1, techoInterno / p[n])`.
2. **Mínimo móvil.** Se toma el mínimo de `g` sobre `N = lookahead · fsOS` muestras con una cola monótona, de coste constante por muestra.
3. **Suavizado.** Dos medias móviles en cascada, de longitudes `N/2 + 1` y `N/2`, que juntas abarcan `N` muestras. Los pesos suman 1 y todos los valores de esa ventana son menores o iguales que la ganancia requerida de la muestra a la que se aplican, así que la ganancia suavizada nunca supera la requerida y no queda sobrepaso.
4. **Aplicación.** El audio se retrasa `N − 1` muestras y se multiplica por la ganancia suavizada.

```cpp
// g = ganancia requerida de la muestra n; N = lookahead en muestras
float minWindow(float g, int n) {            // cola monótona, O(1) amortizado
    while (!q.empty() && q.back().v >= g) q.pop_back();
    q.push_back({g, n});
    while (q.front().i <= n - N) q.pop_front();
    return q.front().v;
}

float s = boxB.process(boxA.process(minWindow(g, n)));   // dos medias móviles
out = delay.process(x) * s;                              // retraso de N - 1 muestras
```

Con `lookahead` por debajo de 0,1 ms, `N` es 1 o menos y la etapa se comporta como un recorte duro; los modos que lo usan (Agresivo) se apoyan en el recorte suave previo y el sobremuestreo para limitar el aliasing.

### Etapa B: cuerpo

Va antes que la A. Trabaja a la frecuencia base: se toma el pico de cada grupo de muestras sobremuestreadas y la ganancia resultante se interpola de vuelta, lo que ahorra CPU.

```cpp
// x = nivel del pico enlazado menos el umbral (techo - margen del modo), en dB
float softKnee(float x, float knee) {
    if (2 * x < -knee)        return 0;
    if (2 * std::abs(x) <= knee) return (x + knee / 2) * (x + knee / 2) / (2 * knee);
    return x;                  // reducción de ganancia en dB, relación infinita
}

// seguidor de la reducción: ataque fijo, release entre rápido y lento
float tau = releaseAuto ? lerp(relFast, relSlow, clamp(sostenido / 3.0f, 0.f, 1.f)) : relSlow;
grEnv = target > grEnv ? coefAtk * grEnv + (1 - coefAtk) * target
                       : coefRel(tau) * grEnv + (1 - coefRel(tau)) * target;
sostenido = media lenta de grEnv, con constante igual a relSlow;
```

El release automático usa el release rápido para picos aislados y se acerca al lento cuando la reducción se mantiene. El umbral de 3 dB y la constante de la media son valores iniciales.

### Recorte suave previo y Carácter

- **Recorte previo por modo:** `x' = lerp(x, techo · softclip(x / techo), cantidad)`, con `softclip` una función tipo `tanh` normalizada a pendiente 1 en cero. La cantidad sale de la tabla de modos.
- **Carácter:** saturación suave sobre la señal con ganancia, `y = x + c · (tanh(k·x)/k − x)` con `c` entre 0 y 1 y `k` entre 1 y 3. Con `c = 0` el bloque no hace nada y se salta.

### Seguro de pico

- Un recorte duro a `techoInterno`, a `fsOS`. En marcha normal no actúa porque la etapa A ya garantiza el techo; es una red de seguridad.
- `techoInterno = techo − margen`, con un margen inicial de 0,1 dB.
- Un estimador de true peak a la salida (sobremuestreo de 4×) cuenta los «overs» por encima del techo. Si aparece alguno, el margen sube 0,05 dB hasta un máximo de 0,5 dB y después baja despacio, 0,01 dB por segundo. Es una heurística para verificar con material real.

### Casos límite

- Con `gain = 0`, entrada por debajo del techo y `character = 0`, la ganancia de ambas etapas es 1 y el audio solo se retrasa.
- En mono, el enlace no hace nada y los controles de enlace se desactivan.
- Con `truePeak` apagado y sobremuestreo 1×, se limita el pico de muestra y se puede reducir la latencia.

## Sobremuestreo

El sobremuestreo evita el aliasing que genera cualquier limitador cuando recorta y permite ver y controlar los picos entre muestras. Nel parte de **4× por defecto** y deja elegir de 1× a 32×, el mismo abanico de 2× a 32× que ofrece [Pro-L 2](https://www.musicradar.com/reviews/fabfilter-pro-l-2).

| Factor | Etapas de 2× | Uso previsto | Meta de CPU (estéreo, 48 kHz, bloque de 256) |
| --- | --- | --- | --- |
| 1× | 0 | Mezcla con latencia mínima; el true peak no queda garantizado | 1 % o menos de un núcleo |
| 2× | 1 | Mezcla y buses | 2 % o menos |
| 4× | 2 | Defecto; masterización y uso general | 4 % o menos |
| 8× | 3 | Master con material muy comprimido | 8 % o menos |
| 16× | 4 | Revisión final | 16 % o menos |
| 32× | 5 | Render final | 32 % o menos |

Las metas de CPU son objetivos por medir en el hito de optimización, no resultados.

**Implementación**

- Se usa `juce::dsp::Oversampling<float>` con filtros de medio banda: FIR equiripple para fase lineal e IIR polifásico para fase mínima. Cada factor es una instancia preparada de antemano; cambiar de factor ocurre en el hilo de mensajes y hace un fundido de 50 ms en la salida para no producir clics.
- La latencia que se informa al anfitrión es la del sobremuestreo más el lookahead. Al cambiar de factor o de fase se avisa al anfitrión con `updateHostDisplay` (cambio de latencia). La latencia real se mide en el hito 5 y se documenta en este archivo.
- Con `osOffline` encendido y el anfitrión en modo no tiempo real (`isNonRealtime()`), se sube un factor: 4× pasa a 8×.
- A 96 kHz o más el máximo efectivo baja a 8×, y a 176,4 kHz o más baja a 4×, para acotar la CPU. La interfaz muestra el factor efectivo.
- Con `truePeak` encendido y un factor de 1× o 2×, la interfaz avisa de que el true peak es aproximado.
- El true peak que muestran los medidores se calcula con su propio sobremuestreo de 4×, independiente del selector. Se valida contra las señales de prueba 15 a 23 de la [EBU Tech 3341](https://tech.ebu.ch/docs/tech/tech3341.pdf).

## Medidores

Cada medidor tiene una definición concreta y una prueba que lo valida. Se miden por defecto la salida del limitador; un selector permite medir la entrada.

| Medidor | Qué muestra | Definición | Actualización |
| --- | --- | --- | --- |
| Pico de muestra | Pico por canal, en dBFS, de entrada y de salida | Sube al instante, cae 15 dB por segundo y retiene el máximo 2 s | 30 Hz |
| True peak | dBTP por canal y máximo de la sesión | Sobremuestreo propio de 4×; botón de reinicio y contador de «overs» sobre el techo | 30 Hz |
| RMS | Nivel RMS por canal, en dBFS | Ventana de 300 ms. Referencia plana: una senoidal a escala completa marca −3 dB. Opción AES17: suma 3 dB para que marque 0 dB | 30 Hz |
| Escalas | Estándar de −16, −32 o −48 dB, y K-12, K-14 o K-20 | Las escalas K usan RMS AES17 y colocan el 0 en −12, −14 o −20 dBFS |  |
| Reducción de ganancia (GR) | dB que quitan juntas las etapas A y B | Valor instantáneo, máximo retenido y etiquetas de pico en la pantalla | 30 Hz |
| Sonoridad momentánea (M) | LUFS | Ventana deslizante de 400 ms, sin puerta | 10 Hz o más |
| Sonoridad a corto plazo (S) | LUFS | Ventana deslizante de 3 s, sin puerta | 10 Hz o más |
| Sonoridad integrada (I) | LUFS | Todo el programa, con puerta absoluta y relativa | 1 Hz o más |
| Rango de sonoridad (LRA) | LU | Según EBU Tech 3342 | 1 Hz |
| PLR | dB | True peak máximo menos sonoridad integrada | 1 Hz |
| Historial | M, S y true peak en el tiempo | Hasta 10 h, con resolución decreciente para lo antiguo | 1 Hz |

### Cálculo de la sonoridad

Se sigue ITU-R BS.1770, tal como lo resumen [EBU R 128](https://en.wikipedia.org/wiki/EBU_R_128) y los requisitos de medidor de la [EBU Tech 3341](https://tech.ebu.ch/docs/tech/tech3341.pdf).

1. **Filtro K a la frecuencia base** (no a la sobremuestreada): un shelving de agudos de unos +4 dB y un paso alto de unos 38 Hz. Los coeficientes se derivan para cualquier frecuencia de muestreo:

```cpp
// Etapa 1: shelving de agudos
const double G = 3.999843853973347, Q = 0.7071752369554196, f0 = 1681.974450955533;
double K = std::tan(M_PI * f0 / fs), Vh = std::pow(10.0, G / 20.0);
double Vb = std::pow(Vh, 0.4996667741545416), a0 = 1.0 + K / Q + K * K;
b = { (Vh + Vb * K / Q + K * K) / a0, 2.0 * (K * K - Vh) / a0, (Vh - Vb * K / Q + K * K) / a0 };
a = { 1.0, 2.0 * (K * K - 1.0) / a0, (1.0 - K / Q + K * K) / a0 };

// Etapa 2: paso alto RLB
const double Q2 = 0.5003270373238773, f2 = 38.13547087602444;
K = std::tan(M_PI * f2 / fs);  a0 = 1.0 + K / Q2 + K * K;
b = { 1.0, -2.0, 1.0 };
a = { 1.0, 2.0 * (K * K - 1.0) / a0, (1.0 - K / Q2 + K * K) / a0 };
```

A 48 kHz deben salir los coeficientes de la tabla de la recomendación; el primer filtro, `b = {1,53512485958697; −2,69169618940638; 1,19839281085285}` y `a = {1; −1,69065929318241; 0,73248077421585}`. Esos valores, y las constantes de arriba, se verifican contra la recomendación antes de darlos por buenos.

2. **Energía por bloque.** Bloques de 400 ms con 75 % de solape, es decir, un paso de 100 ms. Con `z` la media cuadrática de cada canal filtrado, la sonoridad del bloque es `−0,691 + 10·log10(Σ z)` (peso 1 para L y R).
3. **M y S.** M es el bloque más reciente; S es la media de energía de los últimos 30 pasos de 100 ms (3 s). Ninguna lleva puerta ni suavizado extra.
4. **Integrada.** Puerta absoluta en −70 LUFS y puerta relativa 10 LU por debajo del nivel sin puerta. Los bloques se guardan en un histograma de 0,1 LU de resolución, así que la memoria es constante aunque la sesión dure horas.
5. **LRA.** Se calcula con la distribución de los valores de corto plazo, con puerta absoluta de −70 LUFS y puerta relativa de −20 LU, y la diferencia entre los percentiles 95 y 10 (EBU Tech 3342). Esos tres números se verifican contra el documento al implementarlo.
6. **Modo EBU.** Una lámpara indica que el medidor cumple los requisitos: ventanas de 400 ms y 3 s, sin canal LFE y con un decimal. Las escalas del modo son EBU +9 (de −18 a +9 LU, es decir de −41 a −14 LUFS) y EBU +18 (de −36 a +18 LU, de −59 a −5 LUFS); el cero relativo es el objetivo de −23 LUFS.
7. **Reinicio.** Un botón reinicia la integrada y el LRA a la vez, y una opción los reinicia al arrancar el transporte. Durante los primeros 60 s la interfaz marca el LRA como no estable.
8. **Calibración.** Una senoidal estéreo de 1 kHz a −18 dBFS debe marcar −18,0 LUFS.

El hilo de audio solo filtra y acumula la energía de cada paso de 100 ms; envía esos registros a la interfaz por una cola sin bloqueos, y allí se llenan los histogramas y se calculan los percentiles.

### Objetivo y diferencia

El objetivo se elige entre −9, −14, −16, −23 y −24 LUFS, o se escribe a mano. El panel muestra la sonoridad integrada, la diferencia con el objetivo en LU y, con la escala relativa, el cero en el objetivo. Los números se muestran con un decimal.

## Presets

**Formato y ubicación**

- Archivos `.nelpreset` en JSON: versión del formato, nombre, categoría, autor, valores de los 21 parámetros por ID y las claves de interfaz que se guardan (`lufsTarget`, `lufsScale`).
- Presets de usuario en `~/Library/Application Support/FERB/Nel/Presets/User`. Los de fábrica van incrustados en el binario con `juce_add_binary_data` y no se pueden modificar.
- Las categorías son carpetas. Al cargar un archivo con una versión más nueva que la conocida, se avisa y se cargan solo los parámetros reconocidos.

```json
{
  "format": 1,
  "name": "Streaming -14",
  "category": "Streaming",
  "author": "FERB",
  "params": { "mode": 0, "gain": 4.0, "ceiling": -1.0, "oversample": 2 },
  "ui": { "lufsTarget": -14 }
}
```

**Comportamiento**

- Barra de presets: anterior y siguiente, menú por categorías, guardar, guardar como, renombrar y borrar (solo usuario).
- Un asterisco marca un preset modificado. Cargar un preset es un solo paso de deshacer.
- A/B: dos ranuras de estado completo, con copia de A a B; deshacer y rehacer con `juce::UndoManager` enlazado a `AudioProcessorValueTreeState`.
- Integración con el anfitrión: los presets de fábrica se exponen con `getNumPrograms`, `getProgramName` y `setCurrentProgram`, para que aparezcan en el menú de presets del DAW.
- Cargar un preset aplica los cambios con un fundido de 50 ms, igual que cambiar de modo.

**Presets de fábrica** (16 en la versión 1). La ganancia depende del material, así que sus valores son un punto de partida.

| Nombre | Categoría | Modo | Ajustes clave |
| --- | --- | --- | --- |
| Init | Utilidad | Moderno | Ganancia 0, techo −1,0 dBTP, 4× |
| Streaming −14 | Streaming | Moderno | Ganancia +4, techo −1,0, objetivo −14 LUFS |
| Streaming prudente | Streaming | Seguro | Ganancia +2, techo −1,5, objetivo −16 LUFS |
| Master pop brillante | Master | Dinámico | Ganancia +5, techo −1,0, Carácter 8 % |
| Master rock | Master | Versátil | Ganancia +6, techo −1,0 |
| Master electrónica | Master | Agresivo | Ganancia +8, techo −0,3, Carácter 12 % |
| Master acústico | Master | Seguro | Ganancia +2, techo −1,0, release automático |
| Pegamento de bus | Bus | Bus | Ganancia +3, techo −0,5, Carácter 20 % |
| Batería con pegada | Bus | Punchy | Ganancia +4, techo −0,5, enlace de transitorios 50 % |
| Voz clara | Mezcla | Transparente | Ganancia +3, techo −1,0 |
| Cinta cálida | Vintage | Versátil | Ganancia +4, Carácter 45 % |
| Válvula años 60 | Vintage | Punchy | Ganancia +5, Carácter 60 % |
| Radio saturada | Vintage | Agresivo | Ganancia +9, Carácter 80 % |
| Broadcast R 128 | Broadcast | Transparente | Techo −1,0, objetivo −23 LUFS |
| Podcast −16 | Voz | Transparente | Ganancia +6, techo −1,0, objetivo −16 LUFS |
| Render para 16 bits | Utilidad | Moderno | 32×, dither de 16 bits con perfil Medio, techo −1,0 |

## 11. Interfaz

La interfaz es un panel de aparato analógico negro con acentos morados, con perillas crema y medidores de fósforo. Hay una [maqueta interactiva](https://claude.ai/artifact/RoNXBFMKA6NGjJQWxqWBWU) que simula la señal y no procesa audio; la implementación debe replicarla.

**Ventana y redimensión**

- Tamaño base 1040 × 640 px lógicos. Escala libre de 75 % a 200 % desde la esquina inferior derecha, con proporción fija. Hay atajos de 75, 100, 125, 150 y 200 %. Se guarda como `uiScale`.
- Todo se dibuja en coordenadas base y se escala, con soporte Retina.
- En JUCE: `AudioProcessorEditor` con `setResizable(true, false)` y un `ComponentBoundsConstrainer` con relación de aspecto fija. Medidores y pantalla se repintan a 30–60 fps con `VBlankAttachment`; el hilo de audio no calcula nada de dibujo.

**Disposición**

- Cabecera: logotipo «Nel» con placa «BY FERB», preset (anterior, siguiente, menú por categorías, guardar), A/B, deshacer, rehacer, tamaño y bypass.
- Centro: medidor de entrada, pantalla con entrada, salida y reducción de ganancia en ámbar sobre 6 s de historia, y medidor de salida. Bajo la pantalla van GR actual, GR máx, overs y escala (estándar, K-12, K-14, K-20).
- Derecha: VU de reducción de ganancia con aguja y panel de sonoridad estilo equipo: columnas de LED verticales M y S con escala en LU respecto del objetivo (−18 a +9), ventanas empotradas con LUFS integrado, Δ al objetivo, LRA, PLR y TP máx, teclas de objetivo (−9, −14, −16, −23, −24) y reinicio.
- Placa inferior: Ganancia y Techo como perillas grandes con bisel de latón; ocho botones de modo; lookahead, ataque, release, knee y enlaces; Carácter; interruptores (true peak, ganancia unitaria, audición Δ, filtro DC, release auto); y una fila con sobremuestreo 1×–32× (**4× por defecto**), fase lineal o mínima, +1 escalón en render, dither y perfil de noise shaping (activo solo con dither).

**Perillas**

- Cuerpo crema con anillo estriado, tapa central, puntero oscuro con punto ámbar y marcas grabadas. Recorrido de 270°.
- Arrastre vertical (190 px = recorrido completo, Mayús para afinar), rueda, doble clic para el valor por defecto, flechas, Re Pág/Av Pág e Inicio/Fin.
- Lookahead, ataque, release y knee siguen al modo y muestran `*` en la etiqueta del modo si se modifican.
- Accesibilidad: rol `slider` con valor y texto de valor, y foco visible.

**Estilo**

- Paleta: panel c`asi negro #1c1822 (degradado #2a2433 a #111014) con grano fino, acento morado #8a5cf0 en el filete de cabecera, los LED y las teclas activas`, vidrio `#0e0717`, crema `#efe5cf`, latón `#c9a24d`, lila fósforo `#c7a3ff`, ámbar `#ffb347` y alerta `#ff5a7a`.
- Tipografías: Limelight (art déco) para el logotipo, Josefin Sans en mayúsculas con espaciado para rótulos y Oxanium para cifras; son fuentes de Google Fonts, y en el plugin se incrustan como recursos binarios. Idioma español, coma decimal y signo menos tipográfico.
- Medidores: escalera de 48 segmentos (−48 a 0 dBTP), RMS más brillante dentro del pico, retención de pico de 2 s y marca de techo en latón.

**Por verificar en la implementación:** lectura real de los medidores con las señales EBU Tech 3341, contraste y tamaño de los objetivos táctiles, y ajuste de los valores iniciales de cada modo al oído.

## Rendimiento y latencia

Las cifras de CPU son metas para medir en el hito de optimización, no resultados.

| Escenario | Meta |
| --- | --- |
| Hilo de audio, estéreo, 48 kHz, bloque de 256, 4×, interfaz cerrada | 4 % o menos de un núcleo |
| Mismo escenario con todos los medidores activos | 5 % o menos |
| Interfaz abierta, tamaño 100 % | 3 % o menos de un núcleo en el hilo de mensajes |
| Interfaz al 200 % | 6 % o menos |

**Decisiones de rendimiento**

- La etapa B corre a la frecuencia base; solo la etapa A y el seguro corren sobremuestreados.
- El mínimo móvil usa una cola monótona con memoria reservada de antemano para el lookahead máximo (5 ms) y el factor máximo; las medias móviles son sumas recursivas con acumulador en `double`, que se recalcula cada pocos segundos para evitar la deriva.
- Sin asignaciones de memoria, bloqueos ni E/S en `processBlock`; `juce::ScopedNoDenormals` al inicio.
- Si en una ventana de lookahead no hay reducción, la etapa A solo retrasa el audio.
- Los parámetros se suavizan por bloque con `juce::SmoothedValue`; los cambios de estructura (modo, sobremuestreo, preset) usan un fundido de 50 ms.
- Los medidores de sonoridad cuestan dos biquads y una acumulación por canal en el hilo de audio; los histogramas y percentiles se calculan en la interfaz.
- Interfaz: un único `Timer` a 30 Hz para medidores y pantalla; las perillas y los fondos se dibujan una vez en imágenes cacheadas que se regeneran al cambiar el tamaño (con espera de 150 ms al arrastrar); solo se repintan las zonas que cambian. No se usa OpenGL.

**Latencia**

- Latencia total = lookahead (0 a 5 ms) + latencia del sobremuestreo. Se informa con `setLatencySamples` y se avisa al anfitrión cuando cambia, solo al soltar el control.
- Una combinación de latencia mínima (sobremuestreo 1×, lookahead 0, True Peak apagado) queda disponible para monitorizar mientras se graba.
- La latencia medida con un impulso debe coincidir con la informada.

## Proyecto y build

```
nel/
├─ CMakeLists.txt
├─ CMakePresets.json
├─ CLAUDE.md
├─ docs/FICHA.md
├─ assets/ (fonts/, presets/factory/*.nelpreset, images/)
├─ source/
│  ├─ PluginProcessor.h / .cpp
│  ├─ PluginEditor.h / .cpp
│  ├─ Parameters.h / .cpp
│  ├─ dsp/     (Limiter, StageA, StageB, Character, TruePeak, Dither, DcBlocker, OversamplingManager)
│  ├─ meters/  (KWeighting, LoudnessMeter, Histogram, LevelMeters)
│  ├─ presets/ (PresetManager)
│  └─ ui/      (NelLookAndFeel, VintageKnob, ModeSelector, VuNeedle, ScopeDisplay, LoudnessPanel, PresetBar)
├─ tests/      (Catch2: limitador, sonoridad, true peak)
└─ tools/      (scripts de análisis en Python)
```

La lógica de `dsp/` y `meters/` no incluye nada de JUCE que dependa del formato, para poder probarla sin cargar un plugin y para añadir AAX al final.

Configuración en `CMakeLists.txt`, con JUCE 8 por `FetchContent` (fijar la etiqueta exacta al crear el proyecto). AAX solo se añade si se define la ruta del SDK:

```cmake
set(NEL_FORMATS VST3 AU)
if(DEFINED AAX_SDK_PATH)
    juce_set_aax_sdk_path(${AAX_SDK_PATH})
    list(APPEND NEL_FORMATS AAX)
endif()

juce_add_plugin(Nel
    COMPANY_NAME "FERB"
    BUNDLE_ID com.ferb.nel
    PLUGIN_MANUFACTURER_CODE Ferb
    PLUGIN_CODE Nel1
    FORMATS ${NEL_FORMATS}
    PRODUCT_NAME "Nel"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Dynamics
    COPY_PLUGIN_AFTER_BUILD TRUE)
```

Compilar y validar desde la terminal de VS Code:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
ctest --test-dir build --output-on-failure
auval -v aufx Nel1 Ferb
```

- Los plugins se copian a `~/Library/Audio/Plug-Ins/VST3` y `~/Library/Audio/Plug-Ins/Components`. Firma ad-hoc para uso personal: `codesign --force --deep -s - <paquete>`.
- Con AAX: `-DAAX_SDK_PATH=<ruta del SDK>` y, para las versiones comerciales de Pro Tools, firmar con wraptool de PACE según las condiciones que Avid dé al aprobar la cuenta de desarrollador (sección de alcance).
- Extensiones de VS Code: CMake Tools y clangd.
- Dependencias: JUCE 8 y Catch2 para las pruebas. Ninguna otra librería.

## Pruebas y validación

Las metas son objetivos, no resultados. Las pruebas de audio viven en `tests/` y corren con `ctest`; las de comparación se hacen con los scripts de `tools/`.

| Prueba | Cómo | Meta |
| --- | --- | --- |
| Techo | Material variado (pop, rock, electrónica, clásica) y senoidales con ganancia +24 dB; medir el true peak de salida con el medidor propio y con una herramienta externa | Ningún over sobre el techo en los ocho modos con 4×; si aparece alguno, se documenta |
| Transparencia | Ganancia 0, señal bajo el techo, Carácter 0 | Salida igual a la entrada retrasada, con diferencia por debajo de −120 dBFS |
| Distorsión por modo | Senoidal de 1 kHz con 6 dB de reducción; medir THD | Orden esperado: Seguro menor o igual que Transparente, menor o igual que Moderno, menor o igual que Agresivo |
| Aliasing | Senoidales de 10 a 20 kHz empujadas 6 dB sobre el techo; comparar 1× y 4× | Con 4×, energía inarmónica por debajo de −80 dB respecto de la fundamental |
| Latencia | Impulso en cada combinación de sobremuestreo, fase y lookahead | Medida igual a la informada |
| Sonoridad | Señales de prueba de EBU Tech 3341 y 3342; 1 kHz estéreo a −18 dBFS | Dentro de las tolerancias de la norma; la senoidal marca −18,0 LUFS |
| True peak del medidor | Señales 15 a 23 de EBU Tech 3341 | Dentro de las tolerancias |
| Automatización | Mover todos los parámetros con audio sonando | Sin clics ni escalón audible |
| Cambios de estructura | Cambiar modo, sobremuestreo y preset con audio | Sin clics por encima de −60 dBFS |
| Estabilidad numérica | Silencio, DC, valores NaN e infinitos en la entrada, 24 h de audio simulado | Sin NaN a la salida y sin crecimiento de CPU ni de memoria |
| Bloques | Tamaños 1, 32, 256, 4096 y variable | Mismo resultado salvo redondeo |
| Frecuencias de muestreo | 44,1; 48; 88,2; 96; 176,4 y 192 kHz | Funciona y respeta la regla del factor efectivo |
| CPU | Los escenarios de la sección de rendimiento | Dentro de las metas |
| Interfaz | Redimensionar entre 75 % y 200 % con audio durante 10 minutos | Sin cortes de audio ni crecimiento de memoria |
| Compatibilidad | `auval -v aufx Nel1 Ferb`, `pluginval` en nivel 5 o más y carga en tu DAW | Sin errores |
| Comparación con Pro-L 2 | Si dispones de él: mismo material, mismo objetivo de sonoridad, a igual volumen con la ganancia unitaria; comparar PLR, distorsión y aliasing, y escuchar a ciegas | Informe de diferencias; el objetivo es no quedar por debajo |

## Hitos y guía para Claude Code

**Hitos.** Cada uno termina con una prueba que debe pasar antes de seguir.

1. Esqueleto: CMake con JUCE 8, VST3 y AU, editor vacío redimensionable de 75 % a 200 %. Carga en el DAW y pasa `auval`.
2. Parámetros y estado: los 21 parámetros y las claves de interfaz. Prueba de guardar y cargar.
3. Medidores de nivel: pico, true peak con sobremuestreo de 4×, RMS y GR. Pruebas con senoidales.
4. Medidor de sonoridad: filtro K, M, S, I y LRA, con la lectura de calibración y las señales de la EBU.
5. Núcleo del limitador: etapa A con lookahead y sin sobremuestreo. Prueba del techo.
6. Etapa B y los ocho modos con la tabla de la ficha. Prueba de distorsión por modo.
7. Sobremuestreo de 1× a 32×, fase lineal o mínima y latencia informada. Latencia medida igual a la informada.
8. Seguro de pico, True Peak y margen adaptativo. Cero overs en las pruebas del techo.
9. Carácter, recorte previo, dither, filtro de DC, Ganancia unitaria, Audición y Bypass.
10. Presets: formato, gestor, 16 de fábrica, A/B y deshacer.
11. Interfaz, primera parte: `LookAndFeel` vintage, perillas, selector de modo y paneles.
12. Interfaz, segunda parte: pantalla de forma de onda y GR, VU de aguja, panel de sonoridad e historial.
13. Redimensionado y rendimiento de la interfaz.
14. Optimización de CPU y batería completa de validación, con `pluginval` y `auval`.
15. AAX, solo si Avid aprueba la cuenta: SDK, compilación y firma.
16. Pulido: etiqueta By FERB, iconos e instalación en las carpetas de plugins.

**Archivo `CLAUDE.md`** para la raíz del repositorio:

```markdown
# Nel (By FERB)
Limitador de audio VST3, AU y AAX para macOS, con JUCE 8 y CMake.
La ficha técnica docs/FICHA.md es la fuente de verdad.

## Comandos
- Configurar: cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
- Compilar: cmake --build build
- Pruebas: ctest --test-dir build --output-on-failure
- Validar AU: auval -v aufx Nel1 Ferb

## Reglas de audio
- Sin asignar memoria, bloquear, escribir logs ni hacer E/S en processBlock.
- juce::ScopedNoDenormals al inicio de processBlock.
- Parámetros con SmoothedValue; cambios de estructura (modo, sobremuestreo, preset) con fundido de 50 ms.
- dsp/ y meters/ no dependen del formato del plugin.
- La interfaz lee medidores solo por atomics o colas sin bloqueos.

## Flujo
- Un hito por vez, con su prueba. Un commit por hito. No avanzar si la prueba falla.
- Los valores marcados como iniciales en la ficha se pueden afinar: proponer el cambio y actualizar la ficha.
```

**Prompts.** Plantilla para cada hito:

> Lee `docs/FICHA.md` y `CLAUDE.md`. Implementa el hito N de la ficha. Escribe primero la prueba que lo cierra, luego el código, y ejecuta la prueba. Si algo de la ficha es ambiguo o contradictorio, dime cuál es antes de decidir. Al terminar, resume qué cambió y haz un solo commit.

Ejemplos:

> Lee `docs/FICHA.md`, secciones «Algoritmo del limitador» y «Pruebas y validación». Implementa el hito 5: la etapa A con lookahead sin sobremuestreo. La prueba debe comprobar que ninguna muestra de salida supera el techo con senoidales y ruido a +24 dB de ganancia.

> Implementa el hito 4, el medidor de sonoridad. Deriva los coeficientes del filtro K como indica la ficha y comprueba que a 48 kHz coinciden con la tabla de la recomendación. Una senoidal estéreo de 1 kHz a −18 dBFS debe marcar −18,0 LUFS.

> Haz el hito 7. Antes de escribir código, dime cómo piensas reservar las instancias de `juce::dsp::Oversampling` para los seis factores y cómo harás el fundido al cambiar.

Para la interfaz, guarda en `docs/ui/` capturas del diseño interactivo de Nel y pide los hitos 11 y 12 con ellas como referencia.

## Estado y cambios sobre la ficha

Fecha: 31 oct 2026. Implementados los hitos 1 a 14 salvo la lectura contra las señales oficiales de la EBU (se usan señales sintéticas) y el AAX (hito 15, sin SDK). Pasan `CoreTests`, `NelTests`, `NelProcessorTests`, `auval -v aufx Nel1 Ferb` y `pluginval` nivel 10 (VST3 y AU, con pruebas de interfaz). Pendiente: prueba manual en Ableton, Logic y Pro Tools, y afinar de oído los valores iniciales de los modos. Donde el texto de arriba choca con esta sección, manda esta sección.

**Decisiones que cambiaron**

| Tema | Ficha original | Ahora | Por qué |
| --- | --- | --- | --- |
| Recorte previo | `techo · tanh(x/techo)` mezclado según el modo | Recorte con rodilla: idéntico a la entrada hasta el 60 % del techo y, por encima, una curva tipo tanh que se acerca al techo | El `tanh` deformaba también señales bajo el techo (−46 dB de error) y la prueba de transparencia («salida igual a la entrada retrasada») no podía cumplirse |
| Orden de Carácter y recorte previo | Antes del sobremuestreo | Después de subir, a la frecuencia alta | Saturar a la frecuencia base genera aliasing |
| Lookahead | Milisegundos continuos | Se redondea a muestras base enteras; la ventana es N+1 y el retardo del audio N | Latencia total entera; la ganancia suavizada nunca supera la requerida (probado contra una referencia por fuerza bruta) |
| Fase mínima | IIR polifásico | Los mismos FIR de media banda pasados a fase mínima con el cepstro | Un diseño y un camino de código para las dos fases. Su latencia varía unas décimas de muestra con la frecuencia (se declara el redondeo) |
| Latencia | Informada al soltar el control | El procesador la declara al cambiar sobremuestreo, fase, lookahead o render; el motor adopta la estructura cuando el valor lleva 150 ms estable, con fundido de 50 ms | Evita encadenar fundidos al arrastrar el lookahead |
| Bypass | Parámetro propio | Bypass del anfitrión | Regla del repositorio |
| Elegir un modo | «Se cargan los valores del modo en los parámetros avanzados» | Lo hace el clic en la interfaz (`selectMode`); la automatización del parámetro `mode` solo cambia los valores internos | Un preset o una automatización no pueden pisar los valores avanzados guardados |
| Presets | `.nelpreset` en JSON, con categorías | `.ferbpreset` en XML del `PresetManager` común, sin categorías; cada preset escribe los avanzados de su modo | Una sola ruta para los cuatro plugins |
| Ganancia unitaria y Audición | Conmutan | Con suavizado (20 ms y 10 ms) | Conmutar de golpe da un clic |
| Escala de medidores | Estándar de −16, −32 o −48 dB, K-12, K-14, K-20 | ESTÁNDAR (−48 a 0), K-12, K-14 y K-20; la escalera siempre abarca −48 a 0 dB | Es lo que muestra la maqueta |
| Interfaz | — | Botón GUARDAR añadido a la cabecera; sin botón de tamaño (se usa la esquina de la ventana) | La maqueta lleva GUARDAR |

**No implementado en esta versión**
- Historial de sonoridad de hasta 10 h y relación de la fuente (entrada o salida) en la interfaz: el motor mide la fuente que se le pide (`setLoudnessFromInput`) pero la maqueta no tiene control.
- RMS con referencia AES17 (el RMS que se muestra es el plano).
- AAX.
- Lectura contra las señales oficiales de la EBU Tech 3341 y 3342: se verifican con senoidales y niveles sintéticos (1 kHz a −18 dBFS marca −17,99 LUFS a 44,1 y 48 kHz; dos niveles separados 10 LU dan un LRA de 10,00 LU).
- El A/B no se guarda con la sesión.

**Resultados medidos** (48 kHz, estéreo, bloque de 256)

| Prueba | Meta | Resultado |
| --- | --- | --- |
| Techo −1 dBTP con +24 dB, ocho modos y cuatro señales (ruido de programa, senoidales de 1 y 15 kHz, impulsos), 4× | Ningún over | Peor true peak −0,998 dBTP medido a 4× (el estándar); −0,92 dBTP con un medidor de 8×, más estricto |
| Techo con 1× (pico de muestra) y 32× | Ningún over | Cumple |
| Transparencia, ganancia 0, señal bajo el techo, Carácter 0 | Menos de −120 dBFS | 1×: menos de −130 dBFS. Con sobremuestreo: menos de −90 dBFS (el rizado de los filtros) |
| Latencia, impulso | Medida igual a la informada | Exacta en fase lineal; ±1 muestra en fase mínima. Con sobremuestreo 1×, 2×, 4×, 8×, 16×, 32×: lineal 0, 39, 45, 47, 47, 48 muestras y mínima 0, 4, 5, 5, 6, 6, más el lookahead |
| Sobremuestreador, banda de paso | Plana | ±0,05 dB (lineal) y ±0,15 dB (mínima) hasta 17 kHz |
| Sobremuestreador, imágenes | Menos de −80 dB | Cumple en todos los factores y las dos fases |
| Aliasing, senoidal empujada 6 dB sobre el techo, Moderno | −80 dB con 4× | 10 kHz: −31 dBc (1×), −56 (2×), **−77 (4×)**, −96 (8×); 14 kHz: −31, −65, **−73**, −89. **No llega a −80 dB con 4×**; sí con 8× |
| Aliasing, Agresivo | — | 4×: −68 dBc a 10 kHz y −61 dBc a 14 kHz; 8×: −86 y −81 |
| Sin clics (14 controles y cambios de estructura) | ≤ 1,3 veces la señal estable | Cumple |
| Robustez | Sin NaN ni picos | Silencio, continua, NaN, infinitos y subnormales; 44,1 a 192 kHz; bloques de 1 a 4096 |
| `processBlock` | Sin reservas de memoria | 0 reservas (mono y estéreo, parámetros aleatorios) |
| CPU, 4× lineal | 4 % o menos | 1,1 % (1×: 0,48 %; 2×: 0,83 %; 8×: 1,6 %; 16×: 2,4 %; 32×: 4,1 %; la fase mínima cuesta lo mismo) |
| Procesador completo | Sin fallos | 18 millones de comprobaciones: tasas, bloques, canales, estado, editor, latencia declarada, presets, A/B y deshacer |

**Cosas que aprendí por el camino**
- El ruido blanco hasta Nyquist cae en la banda de transición del sobremuestreo (20 a 28 kHz con 48 kHz de base) y los filtros lo alteran: el limitador no puede garantizar el techo sobre lo que el submuestreo cambia después. Las pruebas de techo usan ruido con ancho de banda de programa (paso bajo a 16 kHz).
- `replaceState` del APVTS no restaura un parámetro Bool o Choice que alguien dejó en un valor normalizado fraccionario, y `pluginval` lo prueba así. Se corrigió en los cinco procesadores con `ferb::syncParametersFromState`.
