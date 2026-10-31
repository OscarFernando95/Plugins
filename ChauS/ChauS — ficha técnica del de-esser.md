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

Fecha: 31 oct 2026. Implementado M0 (esqueleto). Donde el texto de arriba choca con esta sección, manda esta sección.

**Decisiones que cambiaron**

| Tema | Ficha original | Ahora | Por qué |
| --- | --- | --- | --- |
| Bypass | Parámetro propio con crossfade de 20 ms | Bypass del anfitrión (`getBypassParameter`), fuera de los presets | Regla del repositorio |
| Presets | JSON en `~/Library/Application Support/FERB/ChauS/Presets/User` y `juce_add_binary_data` | `.ferbpreset` en XML del `PresetManager` común; fábrica en código | Una sola ruta para los plugins |
| Refresco de la interfaz | `VBlankAttachment` | `Timer` | Funciona también en la captura sin pantalla |
| Hilo del proyecto | Carpeta `common/` | `core/`, `ui/` y `ui/vintage/` ya existentes | Se reutilizan |

**Resultados medidos** (48 kHz, estéreo, bloque de 256)

| Prueba | Meta | Resultado |
| --- | --- | --- |
| M0: `ChauSProcessorTests` (tasas 44,1 a 192 kHz, bloques 1 a 4096, mono y estéreo, estado, editor, canales y sidechain) | Sin fallos | 18,15 millones de comprobaciones, 8 casos, todas pasan |
| `processBlock` sin reservas (esqueleto en paso directo) | 0 reservas | 0 |
| `auval -v aufx ChaS Ferb` | Pasa | Pasa |
| `pluginval` nivel 5, VST3 y AU | Pasa | Pasa |
