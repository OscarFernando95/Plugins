# ChauS — ficha técnica del de-esser

Oct 31, 2026 · @Oscar Fernando

ChauS domestica las eses de una voz con un EQ dinámico que sigue la sibilante y muestra en pantalla dónde y cuánto actúa, a la manera del FabFilter Pro-DS. Formatos: VST3, AU y AAX, para macOS.

## 1. Resumen

ChauS controla las eses duras de una voz bajando **solo la banda de la sibilante**, con un EQ dinámico que sigue la frecuencia de cada «s» y lo muestra en pantalla: se ve el espectro, la campana que se mueve y cuánto baja.

- **Detecta sibilantes, no solo agudos.** Un clasificador por rasgos espectrales distingue una «s» de un platillo o de un brillo natural de la voz.
- **Sigue la s.** La campana dinámica rastrea el pico de la sibilante (cada cantante la tiene en un sitio distinto, normalmente entre 5 y 10 kHz).
- **Umbral adaptativo.** Aprende el nivel típico de las eses de esa voz, así el mismo ajuste sirve en frases suaves y fuertes.
- **Lookahead** de hasta 15 ms para que la reducción llegue antes que la s, y **enlace estéreo** con modo mid/side.
- **Pantalla que enseña el énfasis de las S:** espectro en vivo arriba y línea de tiempo abajo donde cada sibilante queda marcada.
- Perillas vintage, estilo By Ferb, y VST3, AU y AAX en macOS universal.

**Fuera de alcance:** plosivas, ruido de respiración y resonancias de sala; no sustituye la edición manual de eses muy marcadas.

## 2. Mercado: qué hacen los demás y qué tomamos

| Producto | Enfoque | Rasgos relevantes |
| --- | --- | --- |
| [FabFilter Pro-DS](https://prod.fabfilter.com/help/pro-ds/using/basiccontrols) | Modo *Single Vocal* (algoritmo que separa sibilante de no sibilante) y modo *Allround* (dispara por rango HP/LP y umbral, para mezclas) | Umbral, Range, filtros HP/LP de 2 a 20 kHz, medidor circular y analizador; [controles avanzados](https://prod.fabfilter.com/help/pro-ds/using/advancedcontrols): Full Band o Split Band, enlace estéreo 0–100 % y mid/side, lookahead hasta 15 ms (unos 10 ms suelen ir bien en voz), escucha del disparo y del sidechain |
| [Oeksound Soothe2](https://www.production-expert.com/production-expert-1/the-best-de-esser-plugins-for-vocals-and-dialogue) | EQ dinámico multibanda que solo corta | Bandas automáticas que responden a acumulaciones de resonancia, no solo a la s |
| Waves Sibilance | Resíntesis orgánica que aísla la energía sibilante | De banda ancha a banda dividida, pantalla gráfica, versión de latencia cero |
| Wavesfactory Re-Esser | Separa componentes tonales y fricativos | Sin umbral; nivel independiente para cada componente |
| Sonnox Oxford SuprEsser | EQ dinámico en banda dividida o completa | Tres modos de calidad, lookahead y **umbral flotante** que se adapta a la interpretación |
| Antares Vocal De-Esser | IA entrenada con voz humana | Forma de onda con color para sibilantes suaves y duras, controles separados |

Las descripciones de los cinco últimos vienen de [este comparativo](https://www.production-expert.com/production-expert-1/the-best-de-esser-plugins-for-vocals-and-dialogue); no revisé sus manuales.

**Lo que tomamos:**

- De Pro-DS: los dos modos, el lookahead de 15 ms, el enlace estéreo con mid/side y las dos funciones de escucha.
- De Sonnox: el umbral adaptativo.
- De Antares: distinguir sibilantes suaves y duras.
- De Soothe2: que la acción sea un corte dinámico de banda, no una bajada de volumen.

**Lo que mejoramos:**

- **Pantalla que enseña el énfasis de la S:** espectro en vivo, la campana que sigue a la sibilante y una línea de tiempo con cada s marcada.
- **Seguimiento de frecuencia y Q** del pico de la sibilante, con ancho automático.
- **Dos perillas de sensibilidad** (s suaves y s duras) en lugar de una sola.

## 3. Algoritmo y DSP

**Flujo de señal**

1. Entrada, y sidechain interno (o externo) filtrado entre HP y LP.
2. **Detector de sibilantes** por rasgos espectrales.
3. **Seguimiento** de la frecuencia y el ancho de la s.
4. **Cálculo de reducción** con umbral adaptativo.
5. **EQ dinámico**: campana (o estante) que baja esa banda; en modo Banda ancha baja todo el audio.
6. Retardo de lookahead sobre el audio, y datos hacia la pantalla.

**Detección (modo *Voz sola*)**

Análisis de 512 muestras con salto de 128 (unos 10,7 ms a 48 kHz). Por cuadro se calculan cinco rasgos:

- energía de la banda de sibilante (entre HP y LP),
- razón entre esa banda y la zona de 300 Hz a 3 kHz,
- planitud espectral en la banda (una s se parece a ruido, un tono no),
- centroide espectral de la banda,
- pendiente de 2 a 4 kHz hacia arriba.

Una **regresión logística** combina los rasgos en una puntuación de 0 a 1, con histéresis (enciende en 0,6, apaga en 0,4) y suavizado de 5 ms. Los pesos iniciales se ajustan con grabaciones de voces; una pequeña red neuronal tipo MLP queda como mejora futura. En modo *General* no hay clasificador: solo se compara la energía de la banda con el umbral, como en una mezcla completa.

**Seguimiento de la s**

La frecuencia central es el centroide, suavizado 20 ms mientras la puntuación es alta; sin sibilante vuelve a la mediana de los últimos 2 s. El ancho *Q* se calcula con la dispersión del espectro (de 2 a 6) y puede fijarse a mano.

**Umbral adaptativo**

Se guarda el percentil 90 de los picos de sibilante de los últimos 5 s. El umbral efectivo es *Umbral* más ese nivel de referencia, de modo que el ajuste sigue valiendo si el cantante se acerca o se aleja. Se puede apagar para usar un umbral absoluto.

**Reducción**

exceso = nivel de la banda − umbral efectivo; reducción = el exceso con rodilla suave de 3 dB, limitado por *Rango* (hasta 24 dB). Ataque de 0,5 ms apoyado en el lookahead; liberación de 10 a 60 ms que depende de la duración de la s. Los dos controles de sensibilidad (*suaves* y *duras*) desplazan el umbral hasta ±6 dB según la clase de la sibilante, comparando su nivel con la referencia adaptativa.

**Aplicación**

- *Banda dividida:* filtro peaking en biquad con coeficientes recalculados cada 16 muestras e interpolados. No añade latencia propia, porque es IIR.
- *Banda ancha:* ganancia sobre todo el audio con la misma curva.
- *Estante* en lugar de campana si la s cae por encima de 9 kHz y el ancho es grande.
- **Lookahead** de 0 a 15 ms: retrasa el audio, no el detector; esa latencia se informa al host.
- **Estéreo:** enlace de 0 a 100 %; reducción por canal = mezcla entre la propia y la mayor de los dos. Con M/S, mid y side se procesan por separado.

**Datos para la pantalla**

Una cola sin bloqueos pasa al hilo de la interfaz el espectro (FFT de 2048, 75 % de solape, suavizado de 1/6 de octava), la frecuencia y el ancho de la campana, la reducción y la puntuación de cada cuadro. El hilo de audio no dibuja nada.

## 4. Parámetros

Todos van en `AudioProcessorValueTreeState`, son automatables y se guardan en el estado del plugin.

| ID | Nombre | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- |
| `mode` | Modo | Voz sola / General | Voz sola | General solo compara energía con el umbral |
| `thr` | Umbral | −60 a 0 dB | −30 | Con *Adaptativo*, −30 significa «justo en la referencia»; cada dB menos baja el umbral un dB |
| `adaptive` | Adaptativo | on/off | on | Referencia: percentil 90 de las s de los últimos 5 s |
| `range` | Rango | 0 a 24 dB | 8 | Reducción máxima |
| `hp` | Filtro HP | 2 a 12 kHz | 4 | Límite inferior de la banda de s |
| `lp` | Filtro LP | 5 a 20 kHz | 12 | Límite superior |
| `track` | Seguir la s | on/off | on | La campana sigue la frecuencia detectada |
| `q` | Ancho (Q) | Auto, 1 a 8 | Auto |  |
| `band` | Banda | Dividida / Ancha | Dividida | Dividida baja solo la banda; Ancha baja todo |
| `shape` | Forma | Campana / Estante / Auto | Auto |  |
| `sensSoft` | S suaves | −6 a +6 dB | 0 | Desplaza el umbral para s suaves |
| `sensHard` | S duras | −6 a +6 dB | 0 | Desplaza el umbral para s fuertes |
| `lookOn` | Lookahead activo | on/off | on |  |
| `look` | Lookahead | 0 a 15 ms | 10 | Suma latencia |
| `release` | Liberación | Auto, 10 a 100 ms | Auto |  |
| `link` | Enlace estéreo | 0 a 100 % | 100 |  |
| `ms` | Mid/Side | Apagado / Mid / Side | Apagado |  |
| `scExt` | Sidechain externo | on/off | off |  |
| `listenTrig` | Escuchar disparo | on/off | off | Oye lo detectado |
| `listenSC` | Escuchar sidechain | on/off | off | Oye el sidechain filtrado |
| `output` | Salida | −12 a +12 dB | 0 |  |
| `bypass` | Bypass | on/off | off | Crossfade de 20 ms |

**Estado de interfaz** (no automatable): `uiScale`, `abSlot`, `presetName`, `displayZoom`.

**Presets de fábrica (10):** Init, Voz masculina, Voz femenina, Locución, Rap, Coros, Voz agresiva, Podcast, Batería (platillos), Mezcla completa.

## 5. Interfaz y estilo

ChauS es la pieza **moderna** de la familia: panel negro con filete coral, una pantalla grande que enseña dónde y cuánto actúa el EQ dinámico, y perillas vintage de baquelita. Hay una [maqueta interactiva](https://claude.ai/artifact/S1tGKoDvgBoJWvH3nkRoji#chaus) que simula una voz con eses y no procesa audio; la implementación debe replicarla.

**Ventana y redimensión**

- Tamaño base 960 × 580 px lógicos. Escala libre de 75 % a 200 % con la esquina inferior derecha, proporción fija, y atajos de 75, 100, 125, 150 y 200 %. Se guarda como `uiScale`.
- En JUCE: `AudioProcessorEditor` con `setResizable(true, false)` y `ComponentBoundsConstrainer` con relación de aspecto fija. La pantalla se repinta a 30–60 fps con `VBlankAttachment`.

**La pantalla que muestra el énfasis de las S**

- **Espectro en vivo** (arriba, 660 × 290): eje de 200 Hz a 20 kHz en escala logarítmica y de −85 a −20 dB. Relleno violeta para la entrada y línea crema para la salida.
- **Zona de sibilante**: franja sombreada en coral entre los filtros HP y LP.
- **Campana dinámica**: curva coral con un manejador que sigue la frecuencia de cada s y baja según la reducción, con su lectura en kHz y dB.
- **Lo que se quita**: el área entre la entrada y la salida se rellena en coral, así se ve la s que se está domando.
- **Línea de tiempo** (abajo, 660 × 84, unos 3 s): nivel de la banda de sibilante, umbral efectivo en coral, reducción en ámbar desde arriba y una marca sobre cada evento detectado.
- En la maqueta HP y LP se ajustan con perillas; arrastrarlos sobre la pantalla queda como mejora.

**Controles**

- Columna derecha: perilla grande **Umbral** (con bisel de latón), perilla **Rango**, selectores Modo (Voz sola o General), Banda (Dividida o Ancha) y Estéreo (L/R, Mid o Side), y tres interruptores: *Escuchar disparo*, *Umbral adaptativo* y *Seguir la S*.
- Fila inferior de siete perillas: Filtro HP, Filtro LP, Lookahead, S suaves, S duras, Enlace y Salida.
- Cabecera: logotipo, placa «BY FERB», presets (anterior, nombre con menú, siguiente), A/B y bypass.

**Presets**

- Menú con los 10 presets de fábrica (la maqueta muestra 5), flechas, asterisco si se modifica, A/B con dos ranuras y deshacer.
- Fábrica incrustada con `juce_add_binary_data`; los del usuario en `~/Library/Application Support/FERB/ChauS/Presets/User` como JSON.

**Estilo**

- Paleta: panel negro `#17141a` (degradado `#272229` a `#0d0b0f`), filete y acentos coral `#ff6b5e`, vidrio `#09070b`, violeta apagado para el espectro, latón `#c9a24d` y ámbar `#ffb347` para la reducción.
- Tipografías: Limelight, Josefin Sans y Oxanium (Google Fonts, incrustadas). Idioma español y coma decimal.
- Perillas de baquelita crema con marcas grabadas; recorrido de 270°. Mismos gestos que en Maru: arrastre vertical con Mayús para afinar, rueda, doble clic, flechas, Re Pág/Av Pág e Inicio/Fin. Rol `slider` con valor y texto, y foco visible.

**Por verificar en la implementación:** legibilidad del coral sobre negro para personas con daltonismo (la lectura numérica de la campana lo respalda), y el costo de la pantalla a 200 %.

## 6. Rendimiento, latencia y calidad

Las cifras son **objetivos a medir**, no resultados.

| Medida | Objetivo |
| --- | --- |
| CPU, estéreo a 48 kHz (un núcleo de Apple silicon) | menos de 2 % |
| Latencia | la del lookahead activo (0 a 15 ms); no hay latencia propia del EQ dinámico, porque es IIR |
| Aciertos del detector en voz con eses reales | 95 % o más |
| Falsos positivos en platillos, hi-hat y consonantes «t» | menos de 5 % |
| Error de la frecuencia seguida frente al pico medido | menos de 300 Hz |
| Retraso entre el inicio de la s y el 90 % de la reducción, con lookahead de 10 ms | 0 ms (la reducción llega a tiempo) |

- La FFT de la pantalla (2048) corre en el hilo de interfaz, nunca en el de audio; los datos pasan por una cola sin bloqueos.
- Sin asignaciones ni bloqueos en el hilo de audio; `juce::ScopedNoDenormals`.
- La ganancia nunca cambia más rápido que 0,5 ms para evitar clics y bandas laterales.
- Cambiar el lookahead actualiza la latencia con `setLatencySamples` y `updateHostDisplay`, con *crossfade* de 50 ms.
- Frecuencias de 44,1 a 192 kHz.

## 7. Formatos y proyecto

Proyecto JUCE 8 con CMake y `FetchContent`, igual que en [la ficha de Nel](https://claude.ai/artifact/Ja4PfyDVnH1KTXUUTtTSaB) (ahí está el detalle de `juce_add_plugin`, `auval` y firmas). Aquí solo lo propio de ChauS.

| Dato | Valor |
| --- | --- |
| Formatos | VST3, AU, AAX (y Standalone solo para pruebas) |
| Código de plugin / fabricante | `ChaS` / `Ferb` |
| AU | `aufx ChaS Ferb`; validar con `auval -v aufx ChaS Ferb` |
| Categoría VST3 | `Fx Dynamics` |
| Categoría AAX | `Dynamics` |
| Canales | mono y estéreo; entrada de sidechain opcional |
| macOS | binario universal (arm64 y x86\_64), mínimo 11 |

- **VST3 y AU** para uso personal: firma ad hoc (`codesign --force --sign -`), sin notarización.
- **AAX** necesita una cuenta de desarrollador de Avid, el SDK de AAX y firma con PACE/iLok (`wraptool`). Sin firmar solo carga en las versiones de desarrollo de Pro Tools. Parte de ese material está bajo acuerdo de confidencialidad, así que no se puede copiar al repositorio.
- Estructura recomendada: un repositorio con `common/` (look and feel By Ferb, perillas, gestor de presets) y una carpeta por plugin. La pantalla del espectro puede reutilizarse en Poty.

## 8. Pruebas y validación

Pruebas unitarias con Catch2 sobre el DSP, sin interfaz, y validadores de formato.

| Prueba | Criterio de éxito |
| --- | --- |
| Sibilante sintética (ruido de banda de 6 a 9 kHz modulado en amplitud) | detectada en 95 % o más de los eventos |
| Falsos positivos | un platillo, un hi-hat y una «t» sintética activan el detector en menos de 5 % |
| Seguimiento de frecuencia | la campana sigue una s que sube de 6 a 9 kHz con error medio menor de 300 Hz |
| Lookahead | con 10 ms, la reducción llega al 90 % antes que el pico de la s |
| Rango | la reducción nunca supera *Rango*; con *Rango* 0 la salida es idéntica a la entrada retrasada |
| Umbral adaptativo | la misma frase a −6 dB y a −12 dB produce reducciones relativas parecidas (diferencia menor de 1,5 dB) |
| Banda dividida | fuera de la banda de la s, el cambio de nivel es menor de 0,5 dB |
| Estéreo y M/S | enlace 100 % da la misma reducción en L y R; en modo Side solo cambia el canal side |
| Clics | la derivada de la ganancia está limitada; sin saltos audibles al encender o apagar |
| Robustez | silencio, DC, NaN, subnormales, bloques de 1 a 4096, cambios de frecuencia de muestreo |

**Validadores:** `pluginval` con rigor 10 sobre VST3 y AU, `auval -v aufx ChaS Ferb`, y el AAX Validator de Avid más una prueba manual en Pro Tools.

**Escucha:** diez frases con eses duras de voces masculinas y femeninas, comparando contra un de-esser de referencia a la misma reducción media.

## 9. Hitos y guía para Claude Code

| Hito | Entregable | Listo cuando |
| --- | --- | --- |
| M0 | Esqueleto del repositorio, `common/` y ChauS compilando en VST3, AU y AAX | carga vacío en un host y pasa `pluginval` nivel 5 |
| M1 | Detector de sibilantes y umbral adaptativo con pruebas | pasan las pruebas de detección y adaptación |
| M2 | EQ dinámico con seguimiento, lookahead y estéreo/M/S | pasan las pruebas de rango, banda y lookahead |
| M3 | Parámetros, presets, A/B y deshacer | el estado se guarda y se restaura |
| M4 | Pantalla de espectro y línea de tiempo con la s marcada | coincide con [la maqueta](https://claude.ai/artifact/S1tGKoDvgBoJWvH3nkRoji#chaus) enlazada en la sección 5 |
| M5 | Validación con `pluginval` 10, `auval` y AAX Validator | sin fallos |
| M6 | Afinado de pesos del detector con grabaciones reales y CPU | se cumplen los objetivos de la sección 6 |

**Bloque para `CLAUDE.md`**

```markdown
# ChauS (By Ferb)
- De-esser con EQ dinámico que sigue la sibilante. VST3, AU y AAX, macOS universal.
- JUCE 8 + CMake (FetchContent). Sin dependencias externas salvo JUCE y Catch2.
- Detector: 5 rasgos espectrales + regresión logística con histéresis. Modo General: solo energía.
- Umbral adaptativo: percentil 90 de las s de los últimos 5 s.
- Lookahead 0 a 15 ms; latencia informada con setLatencySamples + updateHostDisplay.
- La FFT de la pantalla corre en el hilo de interfaz; los datos pasan por una cola sin bloqueos.
- Hilo de audio: sin asignaciones, sin bloqueos, ScopedNoDenormals.
- Los pesos y umbrales son valores iniciales: no los presentes como resultados.
- Pruebas: Catch2 en tests/. Validadores: pluginval 10, auval, AAX Validator.
- AAX: requiere AAX SDK y firma PACE; no incluir el SDK en el repositorio.
```

**Primer prompt sugerido**

```markdown
Lee la ficha técnica de ChauS e implementa el hito M1: extractor de los cinco rasgos, regresión logística con histéresis y umbral adaptativo, con pruebas Catch2 sobre sibilantes sintéticas. Aún sin interfaz. Muéstrame las tasas de acierto medidas.
```

## Estado y cambios sobre la ficha

Fecha: 31 oct 2026. Implementados M0 a M5. Quedan el AAX (M5, sin SDK), M6 (ajuste del detector con grabaciones de voz) y la prueba manual en Ableton, Logic y Pro Tools. Detalle: M0 (esqueleto), M1 (detector de sibilantes y umbral adaptativo) M2 (EQ dinámico con seguimiento, lookahead y estéreo/M-S) M3 (parámetros, presets, A/B y deshacer) y M4 (pantalla de espectro y línea de tiempo, interfaz completa). Donde el texto de arriba choca con esta sección, manda esta sección.

**Decisiones que cambiaron**

| Tema | Ficha original | Ahora | Por qué |
| --- | --- | --- | --- |
| Bypass | Parámetro propio con crossfade de 20 ms | Bypass del anfitrión (`getBypassParameter`), fuera de los presets | Regla del repositorio |
| Presets | JSON en `~/Library/Application Support/FERB/ChauS/Presets/User` y `juce_add_binary_data` | `.ferbpreset` en XML del `PresetManager` común; fábrica en código | Una sola ruta para los plugins |
| Refresco de la interfaz | `VBlankAttachment` | `Timer` | Funciona también en la captura sin pantalla |
| Hilo del proyecto | Carpeta `common/` | `core/`, `ui/` y `ui/vintage/` ya existentes | Se reutilizan |
| Rasgos del detector | Cinco | **Seis**: se añade el «aire» (densidad espectral de 12 a 18 kHz frente a 5 a 9 kHz, independiente de HP y LP) | Con solo los cinco, platillos y hi-hats son casi idénticos a una «s» aguda: el centroide y la planitud se miden *dentro* de HP–LP y se pierde lo que queda por encima. Con el aire, los platillos bajan de 23 % a 3 % de golpes que activan |
| Rasgo «pendiente 2 a 4 kHz» | Pendiente | Regresión de la densidad espectral (potencia media por bin) en cuatro subbandas de igual ancho en octavas | Sumar la energía por subbanda daba una pendiente +3 dB/oct mayor que la real (el ancho crece con la frecuencia) |
| Centroide | Rasgo lineal | Entra como «cercanía» al centro geométrico de HP–LP: −(log₂(centroide/centro))² | Un clasificador lineal no puede rechazar a la vez lo más agudo (hi-hat) y lo más grave («t») |
| Nivel de banda para el umbral | Energía de la banda | Dominio del tiempo (HP y LP de 2.º orden, envolvente RMS de 1 ms), muestra a muestra; la FFT solo clasifica | El calculador de reducción necesita nivel rápido; los cuadros de 10,7 ms no bastan |
| Tamaño del cuadro | 512 con salto 128 | 512/128 a 44,1 y 48 kHz; ×2 a 96 kHz y ×4 a 192 kHz | Mismo ancho de bin (≈ 94 Hz) y misma duración en todas las tasas |
| Referencia adaptativa | Percentil 90 de los picos de los últimos 5 s | Igual, con picos por evento (mínimo 10 ms), referencia inicial −30 dBFS y suavizado de 250 ms; al expirar los eventos de uno en uno el valor sigue la ventana, y sin ninguno conserva el último | Evita saltos del umbral efectivo al terminar cada s |
| Rango y liberación «Auto» | `range` y `release` con valor «Auto» | `release` (10 a 100 ms) más `releaseAuto` (Bool, defecto encendido); `q` (1 a 8) más `qAuto` (Bool, defecto encendido) | `AudioProcessorValueTreeState` no tiene un Float con «Auto»; son los IDs que se fijan en M3 |
| Reducción | Exceso con rodilla de 3 dB, limitado por Rango | Igual; el umbral efectivo es `referencia + (Umbral + 30)` con adaptativo (o el Umbral absoluto) menos la sensibilidad de la clase. Clase de la s: «dura» si su nivel pasa de la referencia (mezcla lineal de −6 a +6 dB sobre la referencia) | La ficha no daba la fórmula de la clase |
| Balística | Ataque 0,5 ms, liberación 10 a 60 ms | Ataque 0,5 ms (constante de tiempo); liberación automática = ¼ de la duración de la s, entre 10 y 60 ms | Interpretación de «depende de la duración» |
| Seguimiento | Centroide suavizado 20 ms; mediana de 2 s | Igual; con la s activa 20 ms, sin ella vuelve a la mediana (de los últimos 750 cuadros con sibilante) con constante de 200 ms; con «Seguir» apagado, el centro geométrico de HP y LP | |
| Ancho Q automático | Dispersión del espectro, de 2 a 6 | `Q = 0,5 / σ` (σ = dispersión en octavas), limitado a 2–6; se mantiene entre eses | Valor inicial a afinar de oído |
| Forma automática | Estante si la s cae sobre 9 kHz y el ancho es grande | Estante si `fc > 9 kHz` y `Q < 2,5` (con Q automática solo se da con Q = 2); fundido de 30 ms entre campana y estante (se calculan las dos) | |
| Coeficientes | Recalculados cada 16 muestras e interpolados | Igual: interpolación lineal de los cinco coeficientes (campana y estante) | |
| Enlace estéreo | Reducción por canal = mezcla de la propia y la mayor | Igual, sobre la reducción pedida (antes de la balística); cada canal conserva su detector y su frecuencia | Con 100 % las dos reducciones son idénticas muestra a muestra |
| Mid/Side | «Mid y side se procesan por separado» | Una sola vía de detección y reducción sobre el componente elegido (Mid o Side); el otro solo se retrasa. Con L/R, dos vías | «Solo cambia el canal side» en modo Side |
| Banda ancha | «Baja todo el audio con la misma curva» | Ganancia de banda ancha igual a la reducción (sin filtro) | |
| Escuchar disparo / sidechain | Dos funciones de escucha | Sidechain = la banda HP–LP que oye el detector; Disparo = esa banda con la puerta de la sibilante (2 ms). Ambas retrasadas el lookahead y con fundido de 10 ms | |
| Latencia | Informada al soltar el control, fundido de 50 ms | `latencyFor` = lookahead en muestras enteras (0 con lookahead apagado); el procesador la declara en cuanto cambia el parámetro y el motor adopta el valor cuando lleva 150 ms estable, con un fundido de 50 ms entre dos tomas de retardo | Evita encadenar fundidos al arrastrar; igual que Nel |
| Cambios de estructura | — | Mid/Side y banda ancha: fundido a silencio de 5 ms con `ModeSwitch` (igual que Cremita y Poty) | Regla del repositorio |
| Suavizados | — | Salida, bypass y escucha en doble precisión: con `float` el suavizado se estanca a ~1e-4 del objetivo (medí un error de 4·10⁻⁵ sobre una señal de 1,8 con salida +6 dB) | |

**Resultados medidos** (48 kHz, estéreo, bloque de 256)

| Prueba | Meta | Resultado |
| --- | --- | --- |
| M0: `ChauSProcessorTests` (tasas 44,1 a 192 kHz, bloques 1 a 4096, mono y estéreo, estado, editor, canales y sidechain) | Sin fallos | 18,15 millones de comprobaciones, 8 casos, todas pasan |
| `processBlock` sin reservas (esqueleto en paso directo) | 0 reservas | 0 |
| `auval -v aufx ChaS Ferb` | Pasa | Pasa |
| `pluginval` nivel 5, VST3 y AU | Pasa | Pasa |

**Resultados medidos de M1** (señales sintéticas, semillas distintas de las del ajuste; 44,1, 48 y 96 kHz; banda 4–12 kHz)

| Prueba | Meta | Resultado |
| --- | --- | --- |
| Sibilante sintética (ruido de 6 a 9 kHz con envolvente, sobre voz sintética), activo en el pico | 95 % o más | 215 de 216 (99,5 %) |
| Voz sintética sin eses | Sin falsos positivos | 0,0 % del tiempo activo |
| Platillo sintético (1,2 s sobre la voz) | Menos de 5 % | 1,6 % del tiempo activo; 3 % de los platillos activan alguna vez |
| **Hi-hat** sintético | Menos de 5 % | **No se cumple**: 12,5 % del tiempo y 33 % de los golpes (hi-hat de parciales metálicos); 35 % y 77 % si lleva mucho ruido blanco (espectralmente es una «s» aguda) |
| **«t»** sintética | Menos de 5 % | **No se cumple**: 10,9 % del tiempo de la ráfaga y 70 % de las «t» activan alguna vez |
| Histéresis (0,6 / 0,4) y suavizado de 5 ms | Entre 0,4 y 0,6 conserva el estado; 63 % del escalón en ≈ 5 ms | Cumple (3 a 9 ms con el salto del cuadro) |
| Umbral adaptativo | Percentil 90 exacto; la misma frase 6 dB más baja da una referencia 6 ± 1,5 dB más baja | Cumple a 44,1 y 48 kHz |
| Robustez | Silencio, continua, NaN, infinito y subnormales sin NaN ni activaciones | Cumple, de 44,1 a 192 kHz |
| `tick` sin reservas | 0 | 0 |

**Voces del sistema (informe, no prueba).** Con las voces de macOS (`say`: Mónica, Paulina, Eddy y Flo, en español; 188 palabras de una sola emisión) el detector activa en las 72 palabras con «s» (100 %); en las 92 palabras sin «s» ni fricativas activa en 25 (27 %: sobre todo ráfagas de «t», «k», «p» y «d»), con solo 0,7 % del tiempo activo; en las 24 con «f» o «j», en 14 (58 %). Sobre el mismo material, un ajuste menos regularizado daba 26 % de falsas activaciones con el mismo sesgo y fue descartado.

**Límites de M1 (para decidir):**
- El ajuste usa señales sintéticas que escribí yo, así que el 99,5 % de detección es sobre mi modelo de «s», no sobre voz grabada. La meta de 95 % sobre eses reales y los falsos positivos en hi-hat y «t» quedan para M6 con grabaciones (cuando pases voces con eses duras).
- Con un ruido blanco filtrado y un hi-hat, o una «s» muy aguda, los seis rasgos espectrales no separan: es un límite del método, no del ajuste. Una «t» o un hi-hat activarán el detector y recibirán algo de reducción; en la práctica el modo *Voz sola* es para pistas de voz.
- Los pesos son valores iniciales: `Classifier::initial()` (regularización L2 de 0,01; sesgo bajado 0,3 sobre el ajuste). Se reproducen con `./build/ChauS/ChauSTests "[.train]"`; el barrido de sesgo, con `"[.report]"` (y `CHAUS_TTS_DIR` para las voces).

**Resultados medidos de M2** (señales sintéticas, 44,1/48/96 kHz, estéreo salvo que se diga, bloque de 256)

| Prueba | Meta | Resultado |
| --- | --- | --- |
| Rango 0 (banda dividida y ancha, L/R, Mid y Side, tres formas) | Salida idéntica a la entrada retrasada | Por debajo de −110 dBFS en todas las combinaciones |
| Latencia | Pico del impulso en la muestra declarada | Exacta con 0, 3, 10 y 15 ms a 44,1, 48 y 96 kHz |
| Rango | La reducción nunca lo supera | Máximo ≤ Rango + 0,001 dB; llega a Rango − 0,5 dB con eses fuertes (3, 8 y 24 dB; banda dividida y ancha) |
| Banda dividida, Rango 24 | Fuera de la banda de la s, menos de 0,5 dB | Por debajo de 2 kHz: −0,02 dB; la banda de 5,5 a 9,5 kHz baja 13,0 a 13,2 dB |
| Seguimiento, s que sube de 6 a 9 kHz | Error medio menor de 300 Hz | 84 Hz (44,1 kHz), 95 Hz (48 kHz) y 189 Hz (96 kHz) |
| Umbral adaptativo, misma frase a −6 y a −12 dB (Umbral −38) | Diferencia de reducción menor de 1,5 dB | 0,00 dB (44,1 kHz) y 0,04 dB (48 kHz); con umbral absoluto, 1,5 a 2,5 dB (Rango 12) |
| Lookahead de 10 ms, reducción al 90 % de la meseta | Antes del pico de la s | Todas las eses medidas (7 a 9 por frase) a 44,1, 48 y 96 kHz |
| **Retraso entre el inicio de la s y el 90 % de la reducción** | 0 ms | **No es 0**: +28,6 ms sin lookahead, +23,5 / +18,5 / +12,9 ms con 5 / 10 / 15 ms. La reducción es 1:1 sobre el exceso y la s de mi modelo tarda ≈ 30 ms en subir, así que el 90 % de la meseta solo puede llegar cuando el nivel llega a su meseta |
| Lo que falta por quitar en cada instante (reducción pedida por el nivel menos la aplicada), por s | — | Máximo 5,3 dB sin lookahead; 4,0 / 3,3 / 2,6 dB con 5 / 10 / 15 ms. Tiempo con más de 3 dB de falta: 3,8 / 1,5 / 0,5 / 0,1 ms. Es el retraso del cuadro de la FFT (≈ 5 ms) más el ataque de 0,5 ms |
| Enlace 100 % | Misma reducción en L y R | Idéntica muestra a muestra; con 0 %, difieren más de 1 dB |
| Mid/Side | Side: solo cambia side | El componente que no se procesa sale idéntico (< −110 dBFS) en Side y en Mid |
| Modo General, sidechain externo, escuchas, salida y bypass | Comportamiento descrito arriba | Pasan |
| Sin clics (20 controles y cambios de estructura, alternando cada 400 ms) | ≤ 1,3 veces la señal estable | Cumple |
| Robustez | Sin NaN ni picos | Silencio, continua, NaN, infinitos y subnormales; 44,1 a 192 kHz; bloques de 1 a 4096; el resultado no depende del tamaño del bloque (< −100 dBFS) |
| `process` | Sin reservas de memoria | 0 reservas (mono y estéreo, con estructura, lookahead, bandas y escuchas cambiando) |
| CPU, estéreo 48 kHz, lookahead 10 ms | Menos de 2 % | 0,59 % (voz sola, banda dividida); 0,56 % (banda ancha); 0,19 % (General) |

**Límites de M2 (para decidir):**
- La meta «0 ms de retraso del 90 % de la reducción» de la ficha no es alcanzable con una reducción 1:1 sobre el exceso: el 90 % de la meseta llega cuando el nivel de la s llega a su meseta. Lo que sí se mide es que el lookahead recorta el faltante máximo de 5,3 a 3,3 dB y deja 0,5 ms con más de 3 dB de falta. Con lookahead 0 no hay «a tiempo» posible: el clasificador ve la s unos 5 ms después de empezar.
- El nivel de referencia por defecto (Umbral −30) deja la mayor parte de las eses por debajo del umbral efectivo (la referencia es el percentil 90 de los picos): para reducir más hay que bajar el Umbral. Es lo que dice la ficha; se afina de oído.

**Parámetros y presets (M3)**

- **24 parámetros** con `ParameterID{id, 1}`: los 22 de la tabla de la sección 4 (sin `bypass`, que es el del anfitrión y no entra en los presets ni en el A/B) más `qAuto` y `releaseAuto` (Bool, encendidos por defecto), que sustituyen al valor «Auto» de `q` y `release`. Rangos y defectos de la ficha (HP 4 kHz, LP 12 kHz, Umbral −30, Rango 8, Lookahead 10 ms, Enlace 100 %, Forma Auto). `ms` se llama «Apagado / Mid / Side» en el anfitrión; la interfaz muestra L/R, Mid y Side. **No renombrar estos IDs.**
- La maqueta usa HP 3,5 kHz y LP 11 kHz como valor de las perillas; manda la ficha (4 y 12 kHz). El preset *Voz masculina* sí lleva 3,5 y 11 kHz.
- **10 presets de fábrica** (Init, Voz masculina, Voz femenina, Locución, Rap, Coros, Voz agresiva, Podcast, Batería (platillos), Mezcla completa). Voz masculina, Voz femenina, Locución, Voz agresiva y Mezcla completa usan los valores de la maqueta; los otros cinco son valores iniciales. Un preset carga solo los parámetros que menciona; el resto vuelve a su defecto. Aparecen también como programas del anfitrión.
- Estado: `setStateInformation` usa `ferb::syncParametersFromState`. A/B (no se guarda con la sesión) y deshacer con `UndoManager`.
- La latencia la declara el procesador en cuanto cambian `look` o `lookOn`; el motor adopta el valor a los 150 ms.

**Resultados medidos de M3** (48 kHz salvo que se diga)

| Prueba | Meta | Resultado |
| --- | --- | --- |
| `ChauSProcessorTests` (parámetros aleatorios, 44,1 a 192 kHz, bloques 1 a 4096, mono y estéreo, estado, editor, canales y sidechain) | Sin fallos | 18,15 millones de comprobaciones, 16 casos |
| Latencia declarada frente a la medida con un impulso | Igual | Exacta con 0, 4, 10 y 15 ms a 44,1, 48 y 96 kHz |
| Latencia al mover el lookahead o apagarlo | Se actualiza | 480 → 720 → 0 → 720 muestras |
| Estado guardado y restaurado (24 parámetros, valores aleatorios, también Bool fraccionarios) | Igual | Cumple |
| Presets | 10 cargan, son programas del anfitrión y Init es el defecto | Cumple |
| `processBlock` | Sin reservas | 0 reservas |
| `auval -v aufx ChaS Ferb` | Pasa | Pasa |
| `pluginval` nivel 10, VST3 y AU, sin pruebas de interfaz | Pasa | Pasa (queda la corrida completa con interfaz en M5) |

**Interfaz (M4)**

- Replica la maqueta (960 × 580): pantalla de espectro 660 × 290, línea de tiempo 660 × 84, columna derecha (Umbral y Rango, selectores Modo, Banda y Estéreo, tres interruptores) y la fila de siete perillas. Captura: `FERB_SNAPSHOT_BLOCKS=232 ./build/ChauS/ChausSnapshot salida.png 1.0 "Voz masculina"` (la herramienta pasa una voz sintética con eses para ver la pantalla en acción) y comparar con la maqueta renderizada con Chrome (`--headless=new --screenshot … "Maru, Cremita y ChauS.html#chaus"`).
- **Espectro:** bandas de 1/6 de octava (nivel eficaz en dBFS) de 200 Hz a 20 kHz, 160 puntos, FFT de 2048 con solape del 75 % (cuatro ventanas por lectura) en el hilo de la interfaz. La entrada y la salida llegan por dos anillos de muestras (suma a mono) que llena `processBlock`. Ataque rápido y caída lenta. **Lo que se quita** es el área entre entrada y salida, en coral.
- **Campana:** se dibuja la respuesta real del filtro en uso (campana, estante o la mezcla entre ambos), colgando de la línea superior (3,2 px por dB), con el manejador en la frecuencia seguida y su lectura «7,4 kHz −3,2 dB».
- **Línea de tiempo:** 375 puntos (3 s) con un punto cada 8 ms que el motor deja en un anillo sin bloqueos: máximo del nivel de banda, umbral efectivo, reducción, frecuencia y marca de evento.
- Interfaz con `Timer` a 30 Hz.

**Cambios sobre la maqueta y la ficha (interfaz)**

| Tema | Maqueta / ficha | Ahora | Por qué |
| --- | --- | --- | --- |
| Controles sin perilla | La maqueta no tiene Forma, Ancho (Q), Liberación, Sidechain externo ni Escuchar sidechain | Dos interruptores bajo los tres de la maqueta («SC EXT.» y «OÍR SC») y un botón «AJUSTES AVANZADOS» con un menú de Forma (Campana, Estante, Auto), Ancho (Auto o 1 a 8) y Liberación (Auto o 10 a 100 ms, en pasos) | No hay sitio en 960 × 580 sin cambiar la composición; el menú da los valores más útiles y el valor exacto sigue siendo automatizable |
| Lookahead apagado | — | La perilla dice «APAGADO» cuando `lookOn` está apagado | `lookOn` no tiene control en la maqueta |
| Bisel de las ventanas | La maqueta lleva un filo fino (2 px negro y 2 px de latón oscuro) | Variante fina del kit (`GlassView(palette, plainBezel)`), opcional: los demás plugins no cambian | Fidelidad con la maqueta |
| Texto de los botones con lámpara | Crema | Corregido en el kit: `VButton` con lámpara dibujaba el texto con el color del contorno de la lámpara (negro al 50 %), también en «BYPASS» de todas las cabeceras | Fallo del kit compartido |
| HP y LP | Perillas; arrastrarlos sobre la pantalla queda como mejora | Solo perillas | Como la maqueta |
| `displayZoom` (estado de interfaz) | Listado en la ficha | **No implementado** | La ficha no dice qué hace |

**Resultados medidos de M4**

| Prueba | Meta | Resultado |
| --- | --- | --- |
| Espectro de la pantalla: senoidal de 0,1 de amplitud (−23,01 dBFS eficaces), 44,1 / 48 / 96 kHz | Nivel y frecuencia | Máximo en su frecuencia (±7 %) y dentro de ±3,1 dB de −23,01 desde 1 kHz; 3,6 dB de defecto a 300 Hz (la banda de 1/6 de octava es más estrecha que el lóbulo de la ventana); 40 dB o más de caída lejos del pico |
| Telemetría: un punto cada 8 ms, reducción ≤ Rango, eventos de la s | Coherente | Cumple (≥ 10 puntos de evento en 3 s y no todo el tiempo; coinciden los valores del momento con el último punto) |
| Anillos del procesador | Entrada y salida (suma a mono) | Idénticos a la señal (< 10⁻⁶ la entrada) |
| `processBlock` | Sin reservas | 0 reservas (con los anillos y la telemetría) |
| Editor abierto y pintado **antes de `prepareToPlay`** | Sin fallos | Cumple (ver abajo) |
| `pluginval` nivel 10 **con** pruebas de interfaz, VST3 y AU | Pasa | Pasa |
| `auval -v aufx ChaS Ferb` | Pasa | Pasa |
| Coste de pintar la ventana entera (render por software, peor caso), 75 / 100 / 200 % | Menos de 33 ms (30 Hz) | 4,1 / 5,8 / 17,5 ms por cuadro |

**Lección:** el primer intento de `pluginval` con interfaz terminó en un fallo de segmentación: el editor leía los anillos de muestras del espectro antes de que `prepareToPlay` los reservara. Mis pruebas y la captura siempre preparaban el procesador antes. Los anillos se reservan ahora una sola vez, en el constructor; reservarlos en `prepareToPlay` habría dejado además una carrera con el editor. Hay una prueba que abre y pinta el editor sin preparar.

**Por verificar:** la legibilidad del coral sobre negro para daltonismo (la lectura numérica de la campana ayuda) sigue sin comprobarse.

**Validación (M5)** — 31 oct 2026, binarios recién construidos

| Validador | Resultado |
| --- | --- |
| `ctest` (12 ejecutables: Core, Ui, Tufi, Poty, Cremita, Nel y ChauS, con y sin procesador) | 100 % pasan (se reconstruyó todo tras tocar el kit compartido) |
| `auval -v aufx ChaS Ferb` | Pasa |
| `pluginval` nivel 10, VST3 y AU, **con** pruebas de interfaz | Pasa |
| AAX Validator de Avid | **No ejecutado:** no hay SDK de AAX en este equipo |
| CPU, estéreo, lookahead 10 ms (% de un núcleo) | 44,1 kHz 0,57 % · 48 kHz 0,6 % (banda ancha 0,58 %; General 0,2 %) · 96 kHz 1,2 % · **192 kHz 2,6 %** (la meta de < 2 % es a 48 kHz) |

**Resumen frente a los objetivos de la sección 6**

| Objetivo | Estado |
| --- | --- |
| CPU < 2 % (estéreo, 48 kHz) | Cumple: 0,6 % |
| Aciertos del detector en voz con eses reales ≥ 95 % | **Sin medir con voz grabada.** Sintético: 99,5 %; voces del sistema (`say`): 72 de 72 palabras con «s» |
| Falsos positivos en platillo < 5 % | Cumple (1,6 % del tiempo; 3 % de los golpes) |
| Falsos positivos en hi-hat y «t» < 5 % | **No se cumple** (hi-hat metálico 12,5 % del tiempo; «t» 10,9 %; 70 % de las «t» activan alguna vez) |
| Error de la frecuencia seguida < 300 Hz | Cumple: 84 a 189 Hz |
| Retraso del 90 % de la reducción respecto del inicio de la s = 0 ms (lookahead 10 ms) | **No se cumple**: +18,5 ms; el 90 % de la meseta antes del pico de la s sí se cumple |
| Latencia = lookahead | Cumple (exacta) |

**Pendiente (M6 y más allá):** reajustar los pesos del clasificador con grabaciones de voz con eses duras (masculinas y femeninas) y volver a medir las metas de detección; probar a mano en Ableton, Logic y Pro Tools; AAX cuando haya SDK; comparar de oído con un de-esser de referencia a la misma reducción media (la «escucha» de la sección 8, que no se hizo); afinar de oído Q automática, liberación automática, clase de sibilante y presets.

**Audio real (31 oct 2026, segunda pasada)** — 14 pistas de la sesión «Luna - Zoe (By Maru)» (mono, 48 kHz, 24 bits; la de karaoke, estéreo de 16 bits). Herramienta: `CHAUS_REAL_FILE=… CHAUS_REAL_OUT=… ./build/ChauS/ChauSTests "[.real]"` (solo lee el archivo; escribe CSV y el audio procesado). **No hay etiquetas**: lo que sigue combina una regla independiente y mi revisión a ojo de espectrogramas, no una anotación hecha por oído.

| Qué | Resultado |
| --- | --- |
| Instrumentos (9 pistas continuas `Luna - Zoe_NN`, ≈ 15 min) | 0 activaciones salvo 0,13 % en una pista (≈ 1 evento) |
| Voces (LEAD 1 ×2 y REFUERZOS ×3, ≈ 9 min): ritmo de eventos | 13 a 24 por minuto, de 100 a 145 ms de mediana; centroide en 7,2 a 8,7 kHz (dentro de HP–LP por defecto) |
| Candidatos de una regla independiente (ráfagas de 6–16 kHz entre el 15 % más brillante, ≥ 40 ms) | 115 de 121 detectados (95 %; por pista 93 a 100 %). De los 6 no detectados, 4 parecen «s» reales cortas o suaves en el espectrograma |
| Eventos sin candidato (33) | La mayoría son «s» finales de frase, más suaves; unos pocos, muy cortos (< 30 ms: 10 a 20 % de los eventos de cada voz), parecen plosivas o chasquidos |
| **Karaoke con batería real** (269 s) | 12 activaciones (1,2 % del tiempo), en entradas de platillo crash y barridos anchos. **El patrón de hi-hat (varios golpes por segundo) no activó nada**: el falso positivo del hi-hat con audio real es mucho menor que con mi señal sintética |
| Reducción con los ajustes por defecto (Umbral −30, Rango 8) sobre una voz principal | Media de **1,1 dB** dentro de los eventos y > 3 dB solo en el 0,4 % del tiempo: por defecto casi no actúa (el Umbral −30 es «justo en la referencia»; con −40 y Rango 10, 5,3 dB de media y > 3 dB en el 2,1 %) |

**Lectura honesta:** sobre voz real, el detector reconoce las «s» claras y no se dispara con instrumentos ni con un hi-hat real; el «95 % de aciertos» no se puede certificar sin anotar todas las «s» (el 95 % anterior es sobre candidatos fáciles, no sobre todas). Quedan por decidir: (1) una duración mínima del evento o un filtro de transitorios para plosivas y chasquidos (10–20 % de los eventos son < 30 ms); (2) si el Umbral por defecto debe actuar más.

**Cierre con audio real (31 oct 2026, tercera pasada)**

| Cambio | Antes → ahora | Medido con las grabaciones |
| --- | --- | --- |
| Reducción por defecto | Umbral −30 y Rango 8 → **Umbral −36 y Rango 10** (`thr` y `range`) | Reducción media en las «s» de las voces de 1,1 dB a 2,4–3,0 dB (picos de 10 dB); instrumentos, 0 activaciones |
| Presets de voz | Valores de la maqueta → Umbral −6 dB y Rango +2 dB (Voz masculina −36/10, Voz femenina −36/11, Locución −40/8, Rap −38/12, Coros −38/9, Voz agresiva −32/16, Podcast −39/9); Batería y Mezcla completa (modo General) sin cambio | Los presets de la maqueta casi no actuaban sobre una voz real |
| Plosivas y chasquidos | Reducción inmediata → **entrada progresiva** `(edad / 8 ms)²` (`Settings::eventRampMs`, no es un parámetro) | Pico de reducción en eventos < 20 ms: 0,92 → 0,25 dB; en «s» de ≥ 80 ms: 4,21 → 4,17 dB (media 2,62 → 2,54). Costo sintético: faltante máximo con lookahead de 10 ms 3,3 → 3,7 dB. Con 15 ms: 0,07 dB y 4,7 dB de faltante; con 30 ms se pierde demasiado al principio de la «s» |

Estado de las metas tras la prueba con audio real: el hi-hat real no activa el detector (meta cumplida con audio real; con mi hi-hat sintético de ruido blanco, no); los chasquidos y plosivas casi no reciben reducción; el 95 % de aciertos sigue sin certificarse (no hay etiquetas por oído). Queda a tu oído: ajustar HP, LP, Umbral y la sensibilidad con tus grabaciones.
