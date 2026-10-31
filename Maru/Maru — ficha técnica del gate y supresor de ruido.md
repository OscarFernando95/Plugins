# Maru — ficha técnica del gate y supresor de ruido

Oct 31, 2026 · @Oscar Fernando

Maru limpia el ruido de fondo de voces e instrumentos con un solo control grande, a la manera del Waves NS1, y suma un motor espectral moderno y un gate clásico opcional. Formatos: VST3, AU y AAX, para macOS.

## 1. Resumen

Maru quita el ruido de fondo constante (ventilador, aire acondicionado, siseo, zumbido de sala) de voz, locución, guitarra o bajo con **una sola perilla grande**. Todo lo demás es opcional y vive en una fila secundaria.

- **Dos motores.** *Supresión* (espectral, el motor por defecto) baja el ruido sin cortar la señal; *Gate* es una puerta clásica con histéresis y lookahead para amplificadores, batería y pausas limpias.
- **Un control manda.** La perilla *Supresión* (0–100) mueve a la vez el umbral, la reducción máxima y la agresividad. El usuario no necesita ajustar nada más para un buen resultado.
- **Transparencia antes que fuerza.** Es preferible dejar algo de ruido que producir «ruido musical» (burbujeo metálico) o cortar el final de las palabras.
- **Barato en CPU.** La versión 1 no usa redes neuronales. Un motor neuronal opcional queda como fase 2.
- **Mono y estéreo**, VST3, AU y AAX, macOS universal (Apple silicon e Intel).

**Fuera de alcance en la versión 1:** dereverberación, quitar clics o ruidos impulsivos, y ruido muy cambiante (tráfico, público). Para eso hacen falta otras herramientas.

## 2. Mercado: qué hacen los demás y qué tomamos

| Producto | Enfoque | Control y límites | Lección para Maru |
| --- | --- | --- | --- |
| [Waves NS1](https://josephnilo.com/blog/waves-ns1-explained/) | Supresión adaptativa, analiza el audio sin captura de perfil | Un solo fader 0–100; medidor de atenuación 0 a −48; versiones mono y estéreo; no expone ataque, ratio, bandas ni perfil | La referencia de sencillez. No separa ruido de voz, así que una reducción grande puede comerse sonido útil |
| [Waves Clarity Vx](https://www.provideocoalition.com/review-waves-clarity-vx/) | Redes neuronales entrenadas para separar voz y fondo | Un dial 0–100 %; tres redes (Broad 1, Broad 2, Broad ECO); versión estéreo con extras | Muy potente, pero con artefactos al quitar mucho (la reseña sugiere 50–75 %) y CPU alta en Broad 1 y 2 |
| Gate clásico (referencia general) | Puerta por umbral con ataque, hold y release | Umbral, rango, ataque, hold, release, filtro de sidechain | Exacto y predecible, pero obliga a ajustar y corta colas y respiraciones |

Solo analicé a fondo NS1 y Clarity Vx; otros supresores del mercado quedan sin revisar.

**Lo que tomamos:** la perilla única, el medidor de atenuación y las versiones mono y estéreo.

**Lo que mejoramos:**

- Un límite de reducción (*Rango*) y suavizado en tiempo y frecuencia, para quedarse lejos del rango donde aparecen artefactos.
- Un botón *Escuchar lo quitado* (la diferencia), que NS1 no tiene y que permite comprobar que no se pierde voz.
- Un testigo con el **piso de ruido estimado**, para ver qué está aprendiendo el algoritmo.
- Un segundo motor (Gate) que comparte el mismo estimador, así el gate también se ajusta solo con una perilla.
- Menos CPU que una red neuronal, con la red como módulo opcional futuro.

## 3. Algoritmo y DSP

**Flujo de señal**

1. Entrada mono o estéreo. El análisis usa la suma mid (enlace estéreo del 100 % por defecto); la misma ganancia se aplica a L y R para no mover la imagen.
2. **Estimador de piso de ruido**, compartido por los dos motores.
3. Motor activo: *Supresión* (espectral) o *Gate* (temporal).
4. Retardo de compensación, mezcla de salida, medidor de atenuación y testigo del piso de ruido.

**Estimador de piso de ruido (común)**

- STFT con ventana raíz de Hann; N = 1024 muestras a 48 kHz (escala con la frecuencia de muestreo, tope 4096) y salto N/4. Modo *Baja latencia*: N = 512.
- Potencia suavizada por bin y **estadística de mínimos** (Martin, 2001) en una ventana de unos 1,5 s, ajustada con una probabilidad de presencia de voz al estilo MCRA/IMCRA (Cohen y Berdugo, 2002), de modo que el piso no suba mientras alguien habla.
- Resultado: un espectro de ruido λ(k) por bin y un valor global en dBFS que se muestra en pantalla.

**Motor Supresión (por defecto)**

- SNR a priori con *decision-directed* (α = 0,98) y ganancia de amplitud espectral logarítmica (Ephraim y Malah), con reducción máxima fijada por *Rango*.
- La perilla *Supresión* a (0–1) controla a la vez la sobreestimación del ruido β = 1 + 2a y el piso de ganancia: a = 0,5 da β ≈ 2 y unos −12 dB; a = 1 da β ≈ 3 y −Rango (−30 dB por defecto). Son valores iniciales a afinar al oído.
- **Contra el ruido musical:** suavizado de la ganancia en frecuencia (3 bins) y en tiempo (ataque 2 ms, recuperación ajustable), y límite a la velocidad de cambio de la ganancia por bin.
- **Preservación de transitorios:** un detector de flujo espectral sube el piso 6 dB durante unos 30 ms tras un ataque, para no ablandar consonantes ni pulsaciones.
- Reconstrucción por solapamiento y suma (WOLA). Latencia = N muestras, reportada al host (unos 21 ms con N = 1024 a 48 kHz; a medir).

**Motor Gate**

- Detector de pico o RMS sobre el sidechain con filtros HP/LP (o sidechain externo).
- **Umbral automático:** piso de ruido + margen de 6 + 18a dB. Con *Auto* apagado se usa el umbral manual.
- **Histéresis** (cierra 6 dB por debajo de donde abre), *hold*, ataque, release, *rango* y **lookahead** de 0 a 10 ms (retraso del audio, sumado a la latencia reportada).
- Ganancia suavizada con rampa de coseno elevado para evitar clics.

**Fase 2 (opcional, no entra en la v1):** un motor neuronal tipo RNNoise o DeepFilterNet como tercer modo. Hay que comprobar la licencia vigente de cada uno antes de incluirlos, y medir su CPU frente a Clarity Vx.

## 4. Parámetros

Todos van en `AudioProcessorValueTreeState`, son automatables y se guardan en el estado del plugin. Los de la fila secundaria pueden ocultarse en el panel, pero siguen existiendo.

| ID | Nombre | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- |
| `amount` | Supresión | 0 a 100 % | 50 | Control principal; maneja umbral, piso y agresividad |
| `engine` | Motor | Supresión / Gate | Supresión |  |
| `range` | Rango | −60 a −6 dB | −30 | Reducción máxima |
| `sens` | Sensibilidad | −10 a +10 | 0 | Desplaza el estimador (Supresión) o el margen (Gate) |
| `recovery` | Recuperación | 10 a 1000 ms | 120 | Subida de ganancia (Supresión) o release (Gate) |
| `gateAuto` | Auto | on/off | on | Umbral = piso de ruido + margen |
| `gateThresh` | Umbral | −80 a 0 dB | −50 | Solo Gate con Auto apagado |
| `attack` | Ataque | 0,1 a 50 ms | 2 | Solo Gate |
| `hold` | Hold | 0 a 500 ms | 40 | Solo Gate |
| `hyst` | Histéresis | 0 a 12 dB | 6 | Solo Gate |
| `lookahead` | Lookahead | 0 a 10 ms | 3 | Solo Gate; suma latencia |
| `scHP` | Sidechain HP | 20 a 2000 Hz | 80 | Solo Gate |
| `scLP` | Sidechain LP | 1 a 20 kHz | 12 000 | Solo Gate |
| `scExt` | Sidechain externo | on/off | off | Solo Gate |
| `link` | Enlace estéreo | 0 a 100 % | 100 |  |
| `latency` | Latencia | Baja / Alta | Alta | Cambia N de 512 a 1024 |
| `listen` | Escuchar lo quitado | on/off | off | Diferencia entre entrada y salida |
| `output` | Salida | −12 a +12 dB | 0 |  |
| `bypass` | Bypass | on/off | off | Con crossfade de 20 ms |

**Mapeo del control principal** (valores iniciales, a afinar al oído)

| Supresión | Sobreestimación β | Reducción máxima | Margen del Gate |
| --- | --- | --- | --- |
| 0 % | 1 | 0 dB (sin efecto) | — |
| 25 % | 1,5 | −6 dB | 10,5 dB |
| 50 % | 2 | −12 dB | 15 dB |
| 75 % | 2,5 | −21 dB | 19,5 dB |
| 100 % | 3 | −Rango (−30 dB) | 24 dB |

**Estado de interfaz** (no automatable): `uiScale`, `abSlot`, `presetName`.

**Presets de fábrica (10):** Init, Voz de podcast, Locución, Voz con ventilador, Zumbido de sala, Guitarra acústica, Amplificador (gate), Batería (gate), Instrumento suave, Rescate fuerte.

## 5. Interfaz y estilo

Maru se ve como una **unidad de rack analógica** de las que imitan los plugins clásicos de UAD y Waves: frente metálico, orejas con tornillos, serigrafía grabada, vúmetro con bisel cromado, palancas y lámpara de joya. Hay una [maqueta interactiva](https://claude.ai/artifact/S1tGKoDvgBoJWvH3nkRoji#maru) que simula la señal y no procesa audio; la implementación debe replicarla.

**Ventana y redimensión**

- Tamaño base 1260 × 300 px lógicos (formato apaisado de rack). Escala libre de 75 % a 200 % con la esquina inferior derecha, con proporción fija, y atajos de 75, 100, 125, 150 y 200 %. Se guarda como `uiScale`.
- En JUCE: `AudioProcessorEditor` con `setResizable(true, false)` y `ComponentBoundsConstrainer` con relación de aspecto fija; todo se dibuja en coordenadas base y se escala. Los medidores se repintan con `VBlankAttachment`.

**Disposición (de izquierda a derecha)**

- Cabecera: logotipo «Maru» con placa «BY FERB», presets (anterior, nombre con menú, siguiente), A/B y bypass.
- Ventana de cristal con el **piso de ruido** que aprende el estimador: barras de entrada y salida y una línea ámbar con el piso. Con *Escuchar lo quitado* las barras muestran la diferencia.
- Tres **palancas cromadas**: motor (Supresión o Gate), escuchar lo quitado y enlace estéreo.
- **Perilla grande Supresión** (178 px, bisel de latón), el control principal.
- Fila de perillas pequeñas según el motor: en Supresión, Rango, Sensibilidad y Recuperación; en Gate, Rango, Ataque, Hold, Histéresis y Release.
- Derecha: **vúmero de atenuación** con aguja, lámpara de joya (*Trabajando* o *En reposo*; en Gate, *Puerta abierta* o *cerrada*), ventanas de piso estimado y reducción, y selector de latencia Baja o Alta.

**Presets**

- Menú con los 10 presets de fábrica (la maqueta muestra 5), flechas anterior y siguiente, y un asterisco cuando se modifica uno. A/B con dos ranuras y deshacer.
- Fábrica incrustada en el binario con `juce_add_binary_data`; los del usuario en `~/Library/Application Support/FERB/Maru/Presets/User` como JSON.

**Estilo**

- Paleta: frente verde salvia `#4e6155` (degradado `#62786b` a `#37463d`) con grano metálico fino, serigrafía crema `#f3ecd9`, vidrio `#060a07`, acento verde pálido `#d7efc9`, latón `#c9a24d` y ámbar `#ffb347` para lámparas.
- Tipografías: Limelight para el logotipo, Josefin Sans para rótulos y Oxanium para cifras (Google Fonts, incrustadas como recursos binarios). Idioma español y coma decimal.
- Perillas de baquelita oscura estriada con puntero crema y marcas grabadas; la grande lleva bisel de latón. Recorrido de 270°.
- Interacción: arrastre vertical (190 px = recorrido completo, Mayús para afinar), rueda, doble clic para el valor por defecto, flechas, Re Pág/Av Pág e Inicio/Fin. Rol `slider` con valor y texto, y foco visible.

**Por verificar en la implementación:** contraste de la serigrafía sobre el verde, tamaño de los objetivos táctiles de las palancas y lectura real del medidor frente a la atenuación medida.

## 6. Rendimiento, latencia y calidad

Las cifras son **objetivos a medir**, no resultados.

| Medida | Objetivo |
| --- | --- |
| CPU, Supresión estéreo a 48 kHz (Apple silicon, un núcleo) | menos de 3 % |
| CPU, Gate estéreo | menos de 0,5 % |
| Latencia, calidad Alta (N = 1024 a 48 kHz) | unos 21 ms, informada al host |
| Latencia, calidad Baja (N = 512) | unos 11 ms |
| Latencia del Gate | su lookahead (0 a 10 ms) |
| Reducción de ruido blanco estacionario a −40 dBFS con Supresión al 100 % | 20 dB o más, sin ruido musical audible |
| Pérdida de inteligibilidad al 50 % (medida STOI sobre voz sintética y real) | menos de 0,03 |

- Sin asignaciones de memoria ni bloqueos en el hilo de audio; todo se reserva en `prepareToPlay`.
- FFT con `juce::dsp::FFT` (usa Accelerate en macOS cuando está disponible).
- `juce::ScopedNoDenormals` en `processBlock`.
- Un cambio de latencia o de lookahead llama a `setLatencySamples` y `updateHostDisplay`, con *crossfade* de 50 ms para que no haya un salto sonoro.
- Tamaños de bloque de 1 a 4096 muestras y frecuencias de 44,1 a 192 kHz.

## 7. Formatos y proyecto

Proyecto JUCE 8 con CMake y `FetchContent`, igual que en [la ficha de Nel](https://claude.ai/artifact/Ja4PfyDVnH1KTXUUTtTSaB) (ahí está el detalle de `juce_add_plugin`, `auval` y firmas). Aquí solo lo propio de Maru.

| Dato | Valor |
| --- | --- |
| Formatos | VST3, AU, AAX (y Standalone solo para pruebas) |
| Código de plugin / fabricante | `Maru` / `Ferb` |
| AU | `aufx Maru Ferb`; validar con `auval -v aufx Maru Ferb` |
| Categoría VST3 | `Fx Restoration` |
| Categoría AAX | `NoiseReduction` |
| Canales | mono y estéreo; entrada de sidechain opcional |
| macOS | binario universal (arm64 y x86\_64), mínimo 11 |

- **VST3 y AU** para uso personal: firma ad hoc (`codesign --force --sign -`), sin notarización.
- **AAX** necesita una cuenta de desarrollador de Avid, el SDK de AAX y firma con PACE/iLok (`wraptool`). Sin firmar solo carga en las versiones de desarrollo de Pro Tools. Parte de ese material está bajo acuerdo de confidencialidad, así que no se puede copiar al repositorio.
- Estructura recomendada: un repositorio con `common/` (look and feel By Ferb, perillas, gestor de presets, medidores) y una carpeta por plugin, para reutilizar lo hecho en Nel.

## 8. Pruebas y validación

Pruebas unitarias con Catch2 sobre el DSP, sin interfaz, y validadores de formato.

| Prueba | Criterio de éxito |
| --- | --- |
| Bypass y Supresión 0 % | salida idéntica a la entrada, con la latencia informada |
| Latencia | un impulso aparece exactamente en la muestra informada |
| Estimador de ruido | converge a ±1,5 dB del nivel real de ruido blanco en 3 s |
| Reducción estacionaria | 20 dB o más al 100 %; sin crecer al subir la perilla |
| Voz sintética (armónicos modulados) con ruido | la voz no baja más de 1 dB al 50 % |
| Ruido musical | sin picos aislados en el espectrograma del residuo (revisión visual y estadística) |
| Transitorios | un impulso de pulsación mantiene su pico dentro de 1,5 dB |
| Gate | abre y cierra con la histéresis y el hold indicados; sin clics (derivada limitada) |
| Robustez | silencio, DC, NaN, valores subnormales, bloques de 1 a 4096, cambios de frecuencia de muestreo |
| Mono y estéreo | enlace 100 % mantiene la imagen; enlace 0 % es independiente |

**Validadores:** `pluginval` con rigor 10 sobre VST3 y AU, `auval -v aufx Maru Ferb` para AU, y el AAX Validator de Avid (viene con el SDK) y una prueba manual en Pro Tools para AAX.

**Escucha:** una lista de 8 grabaciones (locución, voz cantada, guitarra acústica, batería) con ruido real de ventilador y sala, comparando Maru contra el NS1 si se tiene acceso.

## 9. Hitos y guía para Claude Code

| Hito | Entregable | Listo cuando |
| --- | --- | --- |
| M0 | Esqueleto del repositorio, `common/` y Maru compilando en VST3, AU y AAX | carga vacío en un host y pasa `pluginval` nivel 5 |
| M1 | Estimador de ruido y motor Supresión con pruebas | pasan las pruebas de estimador y reducción |
| M2 | Motor Gate con umbral automático | pasan las pruebas del gate |
| M3 | Parámetros, presets, A/B y deshacer | el estado se guarda y se restaura |
| M4 | Interfaz completa: perilla grande, medidor, testigos, escala | coincide con [la maqueta](https://claude.ai/artifact/S1tGKoDvgBoJWvH3nkRoji#maru) enlazada en la sección 5 |
| M5 | Validación: `pluginval` 10, `auval`, AAX Validator | sin fallos |
| M6 | Afinado de valores al oído y medición de CPU | se cumplen los objetivos de la sección 6 |

**Bloque para `CLAUDE.md`**

```markdown
# Maru (By Ferb)
- Gate y supresor de ruido de un solo control. VST3, AU y AAX, macOS universal.
- JUCE 8 + CMake (FetchContent). Sin dependencias externas salvo JUCE y Catch2.
- Hilo de audio: sin asignaciones, sin bloqueos, ScopedNoDenormals.
- Latencia informada con setLatencySamples + updateHostDisplay.
- Valores iniciales de la ficha técnica son orientativos: no los presentes como resultados.
- Pruebas: Catch2 en tests/. Validadores: pluginval 10, auval, AAX Validator.
- AAX: requiere AAX SDK y firma PACE; no incluir el SDK en el repositorio.
- Interfaz: 1 perilla grande + fila secundaria; escala 75 a 200 %; fuentes Limelight, Josefin Sans, Oxanium.
```

**Primer prompt sugerido**

```markdown
Lee la ficha técnica de Maru y crea el esqueleto del proyecto (M0): CMake con JUCE 8, targets VST3, AU y AAX (AAX opcional por la variable AAX_SDK_PATH), carpeta common/ y tests/ con Catch2. Compila VST3 y AU, firma ad hoc y díme cómo cargarlos en mi DAW. No empieces el DSP todavía.
```

## Estado y cambios sobre la ficha

Fecha: 31 oct 2026. Implementado M0 (esqueleto). Donde el texto de arriba choca con esta sección, manda esta sección.

**Decisiones**

| Tema | Ficha original | Ahora | Por qué |
| --- | --- | --- | --- |
| Bypass | Parámetro propio con crossfade de 20 ms | Bypass del anfitrión (`getBypassParameter`), fuera de los presets y del A/B | Regla del repositorio |
| Presets | JSON en `~/Library/Application Support/FERB/Maru/Presets/User` y `juce_add_binary_data` | `.ferbpreset` (XML) del `PresetManager` común; fábrica en código | Una sola ruta para todos los plugins |
| Refresco de la interfaz | `VBlankAttachment` | `Timer` | Funciona también en la captura sin pantalla |
| `common/` | Carpeta nueva | `core/`, `ui/` y `ui/vintage/` existentes | Se reutilizan |

**Resultados medidos**

| Prueba | Meta | Resultado |
| --- | --- | --- |
| M0: `MaruProcessorTests` (tasas 44,1 a 192 kHz, bloques 1 a 4096, mono y estéreo, estado, editor, canales y sidechain) | Sin fallos | 18,15 millones de comprobaciones, 10 casos |
| `auval -v aufx Maru Ferb` | Pasa | Pasa |
| `pluginval` nivel 5, VST3 y AU | Pasa | Pasa |
