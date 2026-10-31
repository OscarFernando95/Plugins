# Cremita — ficha técnica del inflator

Oct 31, 2026 · @Oscar Fernando

Cremita sube la sonoridad percibida y aporta calidez sin comprimir, a la manera del Oxford Inflator, con una curva propia, antialiasing moderno y vúmetros analógicos. Formatos: VST3, AU y AAX, para macOS.

## 1. Resumen

Cremita hace que una mezcla o una pista suene más fuerte, gruesa y cálida **sin comprimir**: reforma la curva de transferencia para que las muestras de nivel medio y bajo ganen presencia, y añade armónicos suaves de válvula. Es el compañero natural de Nel: Cremita antes, Nel al final.

- **Sin detector ni envolvente.** Es un procesador estático, así que no bombea ni cambia el ataque de la batería.
- **Curva propia.** El control *Curva* (−50 a +50) recorre desde un efecto sutil con algo de expansión en lo fuerte hasta el más grueso y sonoro.
- **Juicio justo.** Un botón de *igualar sonoridad* compensa la diferencia de volumen entre entrada y salida, para que el oído no se engañe con «más fuerte suena mejor».
- **Limpio a alta frecuencia.** Antialiasing ADAA de primer orden más sobremuestreo ajustable (2× por defecto).
- **Vúmetros analógicos** de entrada y salida con aguja, y un testigo de *Efecto* que muestra cuánto se está modificando la señal.
- Mono y estéreo, VST3, AU y AAX, macOS universal.

**Fuera de alcance:** no es limitador ni compresor, y no protege de picos; para eso está Nel.

## 2. Mercado: qué hace el Oxford Inflator y qué tomamos

Según el [manual del Oxford Inflator](https://media.uaudio.com/support/manuals/dd/Oxford%20Inflator%20Manual.pdf) y su [tutorial](https://sonnox.com/articles/oxford-inflator-the-complete-tutorial/):

| Control del Inflator | Qué hace | Qué hace Cremita |
| --- | --- | --- |
| Input | Nivel de entrada; por encima de 0 dB hay sobrecarga deliberada, hasta unos +6 dBr si el clip está apagado | Igual, de −12 a +12 dB |
| Effect | 0 a 100 % del efecto; a 100 % evita sobrecargas de salida | Igual, con compensación de ganancia opcional |
| Curve | −50 a +50, 0 por defecto: de sutil y con algo de expansión a más fuerte y grueso | Misma escala, con curva propia y gráfico de transferencia visible |
| Clip 0 dB | Limita al máximo digital; activo por defecto | Igual |
| Band Split | Tres bandas (LF, MF, HF); apagado mantiene los picos de salida ≤ entrada | Tres bandas LR4 con cruces ajustables |
| Output y meters | Nivel de salida, medidor de efecto, luz de sobrecarga | Igual, con vúmetros de aguja y retención de la luz de sobrecarga |

El fabricante describe el proceso como un cambio en la probabilidad de los valores de muestra (los grandes se vuelven más probables), sin compresión, con armónicos de carácter válvula. **La fórmula exacta no es pública**, así que Cremita no la copia: implementa su propia curva con el mismo objetivo.

No revisé a fondo otros inflators; la comparación se apoya solo en el Oxford Inflator.

**Lo que mejoramos:**

- **Igualar sonoridad** por medición K-weighted, para comparar con justicia.
- **Antialiasing ADAA** y sobremuestreo ajustable, para que la saturación en agudos no ensucie.
- **Sesgo** (calidez de armónicos pares) y modo **Mid/Side**.
- **Vúmetros de aguja** más un gráfico de la curva, que el original no muestra.

## 3. Algoritmo y DSP

**Flujo de señal**

1. Ganancia de entrada, y conversión a mid/side si está activo.
2. Opcional: separación en 3 bandas (Linkwitz-Riley de 4.º orden, suma compensada en fase), cruces iniciales de 200 Hz y 3 kHz, ajustables de 80 a 500 Hz y de 1,5 a 8 kHz.
3. Por cada banda (o en banda completa): sobremuestreo, curva *f* con antialiasing ADAA, y vuelta a la frecuencia base.
4. Mezcla *Efecto*: y = (1 − E)·x + E·f(x), con x retrasado lo mismo que la ruta procesada.
5. Filtro pasa altos de 5 Hz (quita el DC que crea el término par) y *Clip 0 dB* opcional.
6. Igualar sonoridad, ganancia de salida y vúmetros.

**Curva de transferencia propia**

Con x normalizado a ±1 después de *Input*:

```latex
f_c(x) = x + k\,x\,(1 - x^2) + b\,x^2\,(1 - |x|)
```

- *k* sigue al control *Curva* (c, de −50 a +50): k = 0,125 + 0,0075·c. La pendiente en 0 es 1 + k, así que c = 0 da +1,0 dB en señales bajas, c = +50 da +3,5 dB y c = −50 da −2,5 dB, que se comporta como una expansión sobre lo suave.
- *b* sigue al control *Sesgo* y añade armónicos pares (calidez): b = (Sesgo / 100)·min(0,3; 1 − 2k). El límite asegura que la curva siga siendo monótona.
- f(0) = 0 y f(±1) = ±1, así que, con *Input* ≤ 0 dB, la salida no supera 0 dBFS. El término cúbico crea tercer armónico, el sesgo crea segundo y cuarto.
- Con *Clip* apagado la curva se prolonga más allá de ±1 hasta ±2 (hasta +6 dB de sobrecarga) con una pendiente suave que termina en tanh.
- Todos los coeficientes son **valores iniciales**: se ajustan al oído contra un Inflator de referencia en pruebas ciegas.

**Antialiasing ADAA de primer orden**

```latex
y[n] = \frac{F_1(x[n]) - F_1(x[n-1])}{x[n] - x[n-1]}, \quad F_1(x) = \frac{x^2}{2} + k\left(\frac{x^2}{2} - \frac{x^4}{4}\right) + b\left(\frac{x^3}{3} - \frac{x^3|x|}{4}\right)
```

Si |x\[n\] − x\[n−1\]| < 10⁻⁵ se usa f del punto medio. La antiderivada es un polinomio, así que cuesta pocas operaciones. Se combina con sobremuestreo de 2× por defecto (1× a 8×, FIR lineal o IIR de fase mínima).

**Igualar sonoridad**

Se mide la sonoridad de entrada y de salida con filtro K (BS.1770) y un integrador de 3 s; la diferencia, limitada a ±12 dB y suavizada 1 s, se resta de la salida. Se congela en silencio.

**Vúmetros**

Balística de VU (300 ms a 99 %), 0 VU configurable a −18, −20 o −14 dBFS. El testigo de *Efecto* muestra la media de |y − x| en dB.

## 4. Parámetros

Todos van en `AudioProcessorValueTreeState`, son automatables y se guardan en el estado del plugin.

| ID | Nombre | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- |
| `input` | Input | −12 a +12 dB | 0 | Sobre 0 dB es sobrecarga deliberada |
| `effect` | Efecto | 0 a 100 % | 100 | Mezcla seca y procesada |
| `curve` | Curva | −50 a +50 | 0 | Define k de la curva |
| `bias` | Sesgo | 0 a 100 % | 0 | Armónicos pares (calidez) |
| `split` | Band Split | on/off | off | 3 bandas LR4 |
| `xo1` | Cruce 1 | 80 a 500 Hz | 200 | Solo con Band Split |
| `xo2` | Cruce 2 | 1,5 a 8 kHz | 3000 | Solo con Band Split |
| `trimLF`, `trimMF`, `trimHF` | Ajuste de banda | −6 a +6 dB | 0 | Peso del efecto por banda |
| `clip` | Clip 0 dB | on/off | on | Apagado permite sobrecarga hasta +6 dB |
| `ms` | Mid/Side | on/off | off |  |
| `os` | Sobremuestreo | 1×, 2×, 4×, 8× | 2× |  |
| `osPhase` | Fase del sobremuestreo | Lineal / Mínima | Mínima |  |
| `match` | Igualar sonoridad | on/off | off |  |
| `output` | Output | −12 a +12 dB | 0 |  |
| `vuRef` | Referencia VU | −18 / −20 / −14 dBFS | −18 |  |
| `inOn` | In | on/off | on | Bypass con compensación de nivel |

**Estado de interfaz** (no automatable): `uiScale`, `abSlot`, `presetName`, `showCurve`.

**Presets de fábrica (10):** Init, Mezcla suave, Presencia vocal, Bus de batería, Pegamento de master, Bajo grueso, Guitarra viva, Clásico, Brillo de aire, Calidez de cinta.

## 5. Interfaz y estilo

Cremita se ve como una **unidad de rack analógica** de estilo clásico, como los plugins de UAD y Waves: frente marfil, orejas de nogal con tornillos, dos vúmetros grandes con bisel de latón, palancas cromadas y serigrafía grabada. Hay una [maqueta interactiva](https://claude.ai/artifact/S1tGKoDvgBoJWvH3nkRoji#cremita) que simula la señal y no procesa audio; la implementación debe replicarla.

**Ventana y redimensión**

- Tamaño base 1260 × 412 px lógicos. Escala libre de 75 % a 200 % con la esquina inferior derecha, proporción fija, y atajos de 75, 100, 125, 150 y 200 %. Se guarda como `uiScale`.
- En JUCE: `AudioProcessorEditor` con `setResizable(true, false)` y `ComponentBoundsConstrainer` con relación de aspecto fija; todo se dibuja en coordenadas base y se escala.

**Disposición**

- Cabecera: logotipo «Cremita» con placa «BY FERB», presets (anterior, nombre con menú, siguiente), A/B y bypass.
- Fila superior: vúmero de **entrada**, ventana de cristal con la **curva de transferencia** (la curva real con k y b, la diagonal de referencia y un punto blanco que muestra el nivel de trabajo) y vúmero de **salida**. Escala de VU con 0 VU en −18 dBFS (configurable).
- Fila inferior: perillas Input, Efecto, **Curva** (grande, con bisel de latón), Sesgo y Output; cinco **palancas cromadas** (Bandas, Clip 0, Mid/Side, Igualar sonoridad, IN/OUT); ventanas de **Efecto** (modificación media en dB) y **Sobrecarga** (luz que se retiene 2 s); y selector de sobremuestreo 1X a 8X.

**Presets**

- Menú con los 10 presets de fábrica (la maqueta muestra 5), flechas anterior y siguiente, asterisco si se modifica, A/B con dos ranuras y deshacer.
- Fábrica incrustada con `juce_add_binary_data`; los del usuario en `~/Library/Application Support/FERB/Cremita/Presets/User` como JSON.

**Estilo**

- Paleta: frente marfil `#e6dabd` (degradado `#f1e8cf` a `#cbbd9a`) con grano fino, tinta marrón `#2a1d14`, orejas de nogal `#4a2d17`, latón `#d6b36a`, ámbar `#e08a2e` para cifras y lámparas, y vidrio `#150e09`.
- Tipografías: Limelight para el logotipo, Josefin Sans para rótulos y Oxanium para cifras (Google Fonts, incrustadas). Idioma español y coma decimal.
- Perillas de baquelita oscura estriada con puntero crema y marcas grabadas; recorrido de 270°. Arrastre vertical (190 px = recorrido), Mayús para afinar, rueda, doble clic para el valor por defecto, flechas, Re Pág/Av Pág e Inicio/Fin. Rol `slider` con valor y texto, y foco visible.

**Por verificar en la implementación:** contraste de la tinta marrón sobre el marfil, tamaño de los objetivos táctiles de las palancas y la balística real de las agujas.

## 6. Rendimiento, latencia y calidad

Las cifras son **objetivos a medir**, no resultados.

| Medida | Objetivo |
| --- | --- |
| CPU, estéreo, banda completa, 2× con ADAA | menos de 1,5 % de un núcleo |
| CPU, 3 bandas a 4× | menos de 4 % |
| Latencia con sobremuestreo 1× o IIR de fase mínima | 0 o muy baja; informada al host |
| Latencia con FIR lineal | informada al host; medir con un impulso |
| Aliasing: seno de 8 kHz a −6 dBFS con Curva +50, 2× y ADAA | componentes alias por debajo de −70 dBc |
| Efecto = 0 % | salida idéntica a la entrada (prueba de nulidad) |

- El procesamiento estático no tiene estado, salvo el filtro de DC, los cruces y los filtros del sobremuestreo.
- Sin asignaciones ni bloqueos en el hilo de audio; `juce::ScopedNoDenormals`.
- Cambios de sobremuestreo o de Band Split con *crossfade* de 50 ms.
- El botón *In* compensa la latencia, para que el A/B no tenga saltos.
- Frecuencias de 44,1 a 192 kHz; por encima de 96 kHz el sobremuestreo efectivo baja un escalón.

## 7. Formatos y proyecto

Proyecto JUCE 8 con CMake y `FetchContent`, igual que en [la ficha de Nel](https://claude.ai/artifact/Ja4PfyDVnH1KTXUUTtTSaB) (ahí está el detalle de `juce_add_plugin`, `auval` y firmas). Aquí solo lo propio de Cremita.

| Dato | Valor |
| --- | --- |
| Formatos | VST3, AU, AAX (y Standalone solo para pruebas) |
| Código de plugin / fabricante | `Crem` / `Ferb` |
| AU | `aufx Crem Ferb`; validar con `auval -v aufx Crem Ferb` |
| Categoría VST3 | `Fx Distortion` |
| Categoría AAX | `Harmonic` |
| Canales | mono y estéreo |
| macOS | binario universal (arm64 y x86\_64), mínimo 11 |

- **VST3 y AU** para uso personal: firma ad hoc (`codesign --force --sign -`), sin notarización.
- **AAX** necesita una cuenta de desarrollador de Avid, el SDK de AAX y firma con PACE/iLok (`wraptool`). Sin firmar solo carga en las versiones de desarrollo de Pro Tools. Parte de ese material está bajo acuerdo de confidencialidad, así que no se puede copiar al repositorio.
- Estructura recomendada: un repositorio con `common/` (look and feel By Ferb, perillas, vúmetros, gestor de presets) y una carpeta por plugin.

## 8. Pruebas y validación

Pruebas unitarias con Catch2 sobre el DSP, sin interfaz, y validadores de formato.

| Prueba | Criterio de éxito |
| --- | --- |
| Curva | monótona en \[−1, 1\] para todo k y b permitidos; f(0) = 0 y f(±1) = ±1 |
| Armónicos | con un seno de 1 kHz, el tercer armónico coincide con la teoría dentro de 0,5 dB; el segundo aparece solo con Sesgo > 0 |
| ADAA | la derivada numérica de F₁ coincide con f; el caso de x casi igual no da división por cero ni NaN |
| Aliasing | seno de 8 kHz a −6 dBFS, Curva +50, 2× y ADAA: alias por debajo de −70 dBc |
| Efecto 0 % | nulidad respecto de la entrada retrasada |
| Clip 0 dB | con Input ≤ 0 dB, ninguna muestra supera 0 dBFS |
| Band Split | la suma de las tres bandas sin proceso es plana (±0,1 dB) |
| Igualar sonoridad | la diferencia de sonoridad entre entrada y salida queda en ±0,2 LU |
| Vúmetros | balística de VU: 300 ms a 99 % con un tono de 1 kHz |
| Robustez | silencio, DC, NaN, subnormales, bloques de 1 a 4096, cambios de frecuencia de muestreo |

**Validadores:** `pluginval` con rigor 10 sobre VST3 y AU, `auval -v aufx Crem Ferb`, y el AAX Validator de Avid más una prueba manual en Pro Tools.

**Escucha:** pruebas ciegas contra un Oxford Inflator, si se tiene acceso, con sonoridad igualada, sobre mezcla, batería, voz y bajo. Allí se afinan *k*, *b* y las ganancias por banda.

## 9. Hitos y guía para Claude Code

| Hito | Entregable | Listo cuando |
| --- | --- | --- |
| M0 | Esqueleto del repositorio, `common/` y Cremita compilando en VST3, AU y AAX | carga vacío en un host y pasa `pluginval` nivel 5 |
| M1 | Curva, ADAA y sobremuestreo con pruebas | pasan las pruebas de curva, ADAA y aliasing |
| M2 | Band Split LR4, Mid/Side y Clip | pasan las pruebas de bandas y clip |
| M3 | Igualar sonoridad y vúmetros | pasan las pruebas de sonoridad y balística |
| M4 | Parámetros, presets, A/B y deshacer | el estado se guarda y se restaura |
| M5 | Interfaz con vúmetros de aguja y gráfico de curva | coincide con [la maqueta](https://claude.ai/artifact/S1tGKoDvgBoJWvH3nkRoji#cremita) enlazada en la sección 5 |
| M6 | Validación, afinado al oído y CPU | `pluginval` 10, `auval`, AAX Validator y objetivos de la sección 6 |

**Bloque para `CLAUDE.md`**

```markdown
# Cremita (By Ferb)
- Inflator: curva de transferencia estática, sin compresión. VST3, AU y AAX, macOS universal.
- JUCE 8 + CMake (FetchContent). Sin dependencias externas salvo JUCE y Catch2.
- f(x) = x + k x (1 - x^2) + b x^2 (1 - |x|); k = 0,125 + 0,0075 c; b limitado para mantener monotonicidad.
- Antialiasing ADAA de primer orden + sobremuestreo 2x por defecto.
- Hilo de audio: sin asignaciones, sin bloqueos, ScopedNoDenormals.
- Los coeficientes son valores iniciales a afinar al oído: no los presentes como resultados.
- Pruebas: Catch2 en tests/. Validadores: pluginval 10, auval, AAX Validator.
- AAX: requiere AAX SDK y firma PACE; no incluir el SDK en el repositorio.
- Interfaz: dos vúmetros de aguja, perillas analógicas, escala 75 a 200 %.
```

**Primer prompt sugerido**

```markdown
Lee la ficha técnica de Cremita e implementa el hito M1: la curva f_c(x), su antiderivada para ADAA y el sobremuestreo, con pruebas Catch2 de monotonicidad, armónicos y aliasing. Aún sin interfaz. Muéstrame los resultados medidos de las pruebas.
```
