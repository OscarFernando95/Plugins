# Visión general {.pb}

**Cremita** sube la sonoridad percibida y aporta calidez **sin comprimir**. Reforma la curva de transferencia para que las muestras de nivel medio y bajo ganen presencia y añade armónicos suaves de válvula, de modo que una mezcla o una pista suena más fuerte, gruesa y cálida sin perder el ataque de la batería.

Es un procesador **estático**: no tiene detector ni envolvente, así que no bombea. Su idea de partida es la del inflator (el Oxford Inflator de Sonnox); la curva, el antialiasing y los medidores de Cremita son propios. Es el compañero natural de Nel: Cremita antes, Nel al final.

<div class="kpi">
<div><b>0,32 %</b><span>CPU de un núcleo, banda completa, 2×</span></div>
<div><b>−96 dBc</b><span>Aliasing a 8 kHz, Curva +50, 2× (límite de medida)</span></div>
<div><b>47 muestras</b><span>Latencia constante, 0,98 ms a 48 kHz</span></div>
<div><b>10</b><span>Presets de fábrica</span></div>
</div>

<figure class="shot"><img src="{{img}}/cremita.png" style="width:100%"><figcaption>Interfaz de Cremita (1260 × 412 px base; la ventana se redimensiona del 75 % al 200 %).</figcaption></figure>

## Características

- **Curva propia** con el control *Curva* (−50 a +50): de un efecto sutil con algo de expansión en lo fuerte, al más grueso y sonoro. El control *Sesgo* añade armónicos pares (calidez).
- **Sin detector ni envolvente.** La misma entrada produce siempre la misma salida.
- **Igualar sonoridad:** compensa la diferencia de volumen entre entrada y salida (medida K-weighted, BS.1770) para que el oído no se engañe con «más fuerte suena mejor».
- **Bandas opcionales:** tres bandas Linkwitz-Riley de 4.º orden con cruces ajustables y un peso del efecto por banda.
- **Mid/Side** opcional y **Clip 0 dB** al final de la cadena.
- **Limpio a alta frecuencia:** antialiasing ADAA de primer orden y sobremuestreo ajustable (1×, 2×, 4× y 8×; 2× por defecto).
- **Vúmetros de aguja** de entrada y salida, un testigo de *Efecto* que muestra cuánto se modifica la señal y una luz de *Sobrecarga*.
- **Presets** con A/B, deshacer y rehacer.

## Cuándo usarlo y cuándo no

Cremita sirve para dar volumen percibido y cuerpo a una pista o a la mezcla final, sin compresión. **No es un limitador ni un compresor y no protege de picos**: con *Clip 0 dB* encendido nada supera 0 dBFS (si la entrada no pasa de 0 dB), pero eso es un recorte, no una limitación suave. Para proteger la salida está Nel.

# Instalación y compatibilidad

| Tema | Detalle |
| --- | --- |
| Sistema | macOS 11 o superior, binario universal (Apple Silicon e Intel) |
| Formatos | VST3 y AU (AUv2, tipo efecto) |
| Canales | Mono y estéreo; Mid/Side opcional |
| Frecuencias de muestreo | 44,1 a 192 kHz, tamaño de bloque variable |
| Latencia | 47 muestras, constante en todos los modos |
| Fuera de alcance | Compresión, limitación, sidechain, MIDI, Windows, Linux |

## Dónde se instala

| Formato | Carpeta |
| --- | --- |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/Cremita.vst3` |
| Audio Unit | `~/Library/Audio/Plug-Ins/Components/Cremita.component` |
| Presets del usuario | `~/Library/Application Support/FERB/Cremita/Presets` |

Cada compilación copia el plugin a esas carpetas y lo firma «ad hoc». Si el paquete se lleva a otro equipo, macOS puede bloquearlo por no estar notarizado; se desbloquea con `xattr -dr com.apple.quarantine <paquete>`. Tras actualizar la versión de un AU, el anfitrión puede necesitar volver a escanear los plugins.

> **Aviso.** Cremita no está notarizado ni firmado con Developer ID. Está pensado para uso personal en el equipo donde se compila.

# Guía de uso {.pb}

## Primeros pasos

1. Inserta Cremita en una pista o en el bus de mezcla. El preset «Init» deja todo en valores neutros: Curva 0, Efecto 100 %.
2. Sube **Input** o la **Curva** hasta que el testigo de *Efecto* marque el cambio que buscas.
3. Enciende **Igualar** (palanca) para comparar con la señal original a igual sonoridad, y alterna **IN/OUT** para oír antes y después.
4. Añade **Sesgo** para más calidez (armónicos pares).

## Mapa de la interfaz

La interfaz imita una unidad de rack analógica: frente marfil, orejas de nogal, vúmetros con bisel de latón, palancas cromadas y serigrafía grabada.

| Zona | Contenido |
| --- | --- |
| Cabecera | Nombre con la placa **BY FERB**; presets (anterior, nombre, siguiente), A/B, deshacer y rehacer; Bypass |
| Fila superior | Vúmetro de **entrada**, ventana de cristal con la **curva de transferencia** y vúmetro de **salida** |
| Fila inferior | Perillas Input, Efecto, **Curva** (grande), Sesgo y Output; cinco palancas; ventanas de Efecto y Sobrecarga; selector de sobremuestreo 1X a 8X |

## Controles

| Control | Qué hace | Rango (defecto) |
| --- | --- | --- |
| **Input** | Ganancia antes de la curva. Por encima de 0 dB se sobrecarga a propósito | −12 a +12 dB (0) |
| **Efecto** | Mezcla entre la señal seca y la procesada. 0 % deja la entrada intacta | 0 a 100 % (100) |
| **Curva** | Forma de la curva. Positiva: más grueso y sonoro; negativa: más sutil, con expansión sobre lo suave | −50 a +50 (0) |
| **Sesgo** | Asimetría de la curva: armónicos pares (calidez) | 0 a 100 % (0) |
| **Output** | Ganancia final | −12 a +12 dB (0) |
| **Sobremuestreo** | 1X, 2X, 4X u 8X. Más sobremuestreo, menos aliasing y más CPU | 2X |

### Palancas

| Palanca | Arriba · abajo | Qué hace |
| --- | --- | --- |
| **Bandas** | 3 · 1 | Divide en tres bandas (LR4) y procesa cada una; al activarla aparece el editor de bandas |
| **Clip 0** | ON · OFF | Recorta a 0 dBFS al final de la cadena. Apagado permite sobrecarga hasta +6 dB |
| **M/S** | M/S · L/R | Procesa mid y side en lugar de izquierda y derecha |
| **Igualar** | ON · OFF | Compensa la diferencia de sonoridad entre entrada y salida |
| **IN** | IN · OUT | OUT deja pasar la entrada retrasada (sin Input ni Output), con un fundido de 10 ms, para comparar |

### Medidores

- **Vúmetros de entrada y salida:** balística de VU (99 % en 300 ms). El 0 VU está en −18 dBFS por defecto (configurable a −20 o −14 dBFS desde el parámetro *Referencia VU*; la interfaz no tiene control para eso, se automatiza desde el anfitrión).
- **Curva de transferencia:** la curva real con los valores `k` y `b`, la diagonal de referencia y un punto blanco con el nivel de trabajo.
- **Efecto:** cuánto cambia el nivel el procesado frente al seco, en dB.
- **Sobrecarga:** luz que se retiene unos 2 segundos.

### Editor de bandas

Con **Bandas** encendido, bajo la ventana de cristal aparecen las pestañas **CURVA** y **BANDAS**. En BANDAS la ventana pasa a ser un editor:

- Los **cruces** se arrastran en horizontal (Cruce 1: 80 a 500 Hz · Cruce 2: 1,5 a 8 kHz, escala logarítmica).
- El **peso de cada banda** se arrastra en vertical (±6 dB): multiplica el aporte de esa banda al efecto.
- **Doble clic** restablece, la **rueda** ajusta y **Mayús** afina.

> **Nota.** El editor de bandas se maneja solo con ratón; el teclado no llega a esos tiradores.

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
| **Init** | Valores neutros (los de fábrica) |
| **Mezcla suave** | Efecto 70 %, Curva −10, Sesgo 15 %, Igualar |
| **Presencia vocal** | Input +2 dB, Efecto 80 %, Curva +10, Sesgo 30 %, Igualar, Output −1 dB |
| **Bus de batería** | Input +3 dB, Curva +20, Sesgo 10 %, Bandas, Igualar, Output −2 dB |
| **Pegamento de master** | Input +4 dB, Bandas, Clip apagado, M/S, Output −3 dB |
| **Bajo grueso** | Curva +35, Sesgo 40 %, Bandas con Cruce 1 en 120 Hz, graves +3 dB y agudos −4 dB |
| **Guitarra viva** | Efecto 80 %, Curva +25, Sesgo 30 %, Bandas (cruces 250 Hz y 4 kHz), graves −2 dB, medios +2 dB, agudos +2 dB |
| **Clásico** | Valores neutros (Curva 0) |
| **Brillo de aire** | Efecto 70 %, Curva +15, Bandas con Cruce 2 en 5 kHz, graves −6 dB, medios −3 dB, agudos +3 dB, 4× |
| **Calidez de cinta** | Efecto 80 %, Curva −20, Sesgo 70 % |

Los presets del usuario son archivos `.ferbpreset` (XML) en la carpeta indicada arriba. Los cinco primeros de fábrica usan los valores de la maqueta de diseño; los otros cinco son valores iniciales.

## Consejos de uso

- **Compara a igual sonoridad.** Activa Igualar antes de juzgar el efecto: «más fuerte» no es «mejor».
- **Sin sobrecargar la salida:** con Clip encendido y Input ≤ 0 dB nada supera 0 dBFS, incluso con Output alto, porque el recorte va al final.
- **Más calidez:** sube Sesgo con Curva moderada. A Curva +50 el Sesgo deja de actuar (ver las limitaciones).
- **Bajos y agudos por separado:** con Bandas, baja el peso de los graves y sube el de los agudos para abrir el brillo sin engordar.
- **Sobremuestreo:** 2× basta para casi todo; con 1× el ADAA no alcanza en agudos intensos.

> **Nota.** Los coeficientes de la curva y las ganancias por banda de los presets son valores iniciales para afinar de oído contra una referencia, en prueba ciega y con la sonoridad igualada.

# Referencia de parámetros

Todos los parámetros pasan por `AudioProcessorValueTreeState` con identificador de versión 1, son automatizables y se guardan en el estado. **No se renombran:** hacerlo rompe presets y sesiones guardadas.

| ID | Nombre | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `input` | Input | Decimal | −12 a +12 dB | 0 | Sobre 0 dB es sobrecarga deliberada |
| `effect` | Efecto | Decimal | 0 a 100 % | 100 | Mezcla de seco y procesado |
| `curve` | Curva | Decimal | −50 a +50 | 0 | Define `k` de la curva |
| `bias` | Sesgo | Decimal | 0 a 100 % | 0 | Armónicos pares (calidez) |
| `split` | Band Split | Sí/No | apagado o encendido | Apagado | Tres bandas LR4 |
| `xo1` | Cruce 1 | Decimal | 80 a 500 Hz | 200 | Escala logarítmica; solo con Band Split |
| `xo2` | Cruce 2 | Decimal | 1,5 a 8 kHz | 3000 | Escala logarítmica; solo con Band Split |
| `trimLF` | Ajuste graves | Decimal | −6 a +6 dB | 0 | Peso del efecto en la banda |
| `trimMF` | Ajuste medios | Decimal | −6 a +6 dB | 0 | Peso del efecto en la banda |
| `trimHF` | Ajuste agudos | Decimal | −6 a +6 dB | 0 | Peso del efecto en la banda |
| `clip` | Clip 0 dB | Sí/No | apagado o encendido | Encendido | Apagado permite sobrecarga hasta +6 dB |
| `ms` | Mid/Side | Sí/No | apagado o encendido | Apagado | |
| `os` | Sobremuestreo | Opciones | 1×, 2×, 4×, 8× | 2× | Cambiarlo hace un fundido de 5 ms |
| `match` | Igualar sonoridad | Sí/No | apagado o encendido | Apagado | |
| `output` | Output | Decimal | −12 a +12 dB | 0 | |
| `vuRef` | Referencia VU | Opciones | −18, −20, −14 dBFS | −18 | Sin control en la interfaz |
| `inOn` | In | Sí/No | apagado o encendido | Encendido | OUT = entrada retrasada |
| `bypass` | Bypass | Sí/No | apagado o encendido | Apagado | Es el bypass del anfitrión; no forma parte de los presets |

**Estado de la interfaz** (no automatizable): `uiScale` (tamaño de la ventana), `presetName` y `view` (pestaña CURVA o BANDAS).

# Documentación técnica

## Arquitectura de señal

{{fig:flujo}}

1. **Input** y, si está activo, conversión a mid/side.
2. Opcional: separación en 3 bandas Linkwitz-Riley de 4.º orden (suma compensada en fase), cruces iniciales de 200 Hz y 3 kHz.
3. Por cada banda (o en banda completa): sobremuestreo, curva *f* con ADAA y vuelta a la frecuencia base.
4. **Efecto**, con el seco retrasado lo mismo que la ruta procesada: <br>`y = seco + Efecto · Σ peso_b · (procesado_b − seco_b)`. Con Band Split apagado el peso vale 1; con Band Split el seco pasa por los mismos cruces, de modo que con Efecto 0 la magnitud es plana.
5. **Filtro de continua** (paso alto de 5 Hz) aplicado solo al aporte, para quitar el DC que crea el término par. Con Efecto 0 la salida es la entrada retrasada, bit a bit.
6. **Igualar sonoridad**, **Output** y vúmetros.
7. **Clip 0 dB** al final, para que «con Input ≤ 0 dB nada supere 0 dBFS» valga también con Output alto.

La entrada con NaN o infinito se sustituye por 0 antes de entrar a los filtros, para que un NaN no se quede en los estados.

## Curva de transferencia

Con x normalizado a ±1 después de Input:

<div class="formula">f(x) = x + k·x·(1 − x²) + b·x²·(1 − |x|) &nbsp;&nbsp; para |x| ≤ 1</div>

- **k** sigue al control Curva (c, de −50 a +50): `k = 0,125 + 0,0075·c`. La pendiente en 0 es 1 + k: con c = 0 da +1,0 dB en señales bajas, con c = +50 da +3,5 dB y con c = −50 da −2,5 dB (una expansión sobre lo suave).
- **b** sigue al control Sesgo y añade armónicos pares: `b = Sesgo/100 · min(0,3; 1 − 2k)`. El límite mantiene la curva monótona.
- f(0) = 0 y f(±1) = ±1, así que con Input ≤ 0 dB la salida no supera 0 dBFS. El término cúbico crea tercer armónico; el sesgo crea segundo y cuarto.
- Con **Clip apagado** la curva se prolonga más allá de ±1 (hasta +6 dB de sobrecarga): `f(x) = ±(1 + tanh(m·(|x| − 1)))`, con `m` la pendiente de f en ±1 (mínimo 0,25). Es continua y monótona y tiene antiderivada cerrada.

{{fig:curvas}}

{{fig:sesgo}}

## Antialiasing ADAA de primer orden

<div class="formula">y[n] = ( F(x[n]) − F(x[n−1]) ) / ( x[n] − x[n−1] ) + ½·( f(x[n]) − f(x[n−1]) )</div>

F es la antiderivada de f, un polinomio (cuesta pocas operaciones). El primer término es el ADAA clásico; **el segundo es una reposición propia**: el promedio del ADAA es un pasa bajos que restaba 0,5 dB a 10 kHz con 2× incluso con señales débiles. Con la reposición la ganancia es plana (+1,02 dB ±0,1 de 100 Hz a 10 kHz) y a cambio se pierden 3 a 9 dB de supresión de aliasing solo con saturación extrema. Si |x[n] − x[n−1]| < 10⁻⁵ se usa f en el punto medio. Se combina con sobremuestreo (1× a 8×, FIR de media banda).

## Igualar sonoridad

Se mide la sonoridad de la **entrada original** (antes de Input) y de la salida antes de Output, con filtro K (BS.1770) y un integrador de 3 s. La diferencia, limitada a ±12 dB y suavizada 1 s, se resta de la salida. Se congela por debajo de −70 LUFS (silencio).

## Vúmetros y testigo de Efecto

Balística de VU (99 % en 300 ms), 0 VU a −18 dBFS por defecto. El testigo de Efecto muestra el cambio de nivel del procesado frente al seco, en dB (con Curva 0, aproximadamente +1,0 dB).

## Latencia

La latencia declarada al anfitrión es **constante, 47 muestras** (la del 8×) en todos los modos. Cambiar el sobremuestreo no cambia la latencia. Medida con un impulso: el pico cae en la muestra informada, exacta sin bandas; con bandas, ±3 muestras por el retardo de grupo de los cruces.

| Frecuencia de muestreo | 44,1 kHz | 48 kHz | 96 kHz | 192 kHz |
| --- | --- | --- | --- | --- |
| Latencia (47 muestras) | 1,07 ms | 0,98 ms | 0,49 ms | 0,24 ms |

## Cambios sin clics

| Cambio | Mecanismo |
| --- | --- |
| Input, Output, Efecto, Curva, Sesgo y ajustes de banda | Parámetros suavizados |
| Sobremuestreo, Bandas, M/S y Clip | Fundido a silencio de 5 ms con `ModeSwitch` |
| IN / OUT | Fundido de 10 ms |

## Reglas de tiempo real

- Sin asignaciones de memoria ni bloqueos en `processBlock`: todo se reserva en `prepare` para el máximo (8×). Lo vigila una prueba con un asignador instrumentado.
- `juce::ScopedNoDenormals` al inicio y parámetros suavizados con `juce::SmoothedValue`.
- Interfaz: temporizador a 30 Hz para vúmetros y curva.

## Estado y presets

El estado es el árbol de `AudioProcessorValueTreeState` más las propiedades de interfaz `uiScale`, `presetName` y `view`. Los presets guardan el valor real de cada parámetro; los de fábrica van en el código y los del usuario en archivos `.ferbpreset` (XML). El A/B no se guarda con la sesión.

## Estructura del código

| Carpeta | Contenido |
| --- | --- |
| `Cremita/source/dsp/` | `CremitaCurve` (curva, antiderivada y ADAA), `CremitaEngine` (motor independiente del anfitrión) |
| `Cremita/source/ui/` | Ventana de curva y editor de bandas |
| `core/` | DSP común sin JUCE: sobremuestreador, bandas, medidor de sonoridad BS.1770, medidor de nivel |
| `ui/` y `ui/vintage/` | Presets, A/B, escalado y el kit de interfaz «aparato analógico» compartido |

# Calidad y verificación

Medidas a 48 kHz, estéreo y bloque de 256 (31 de octubre de 2026).

| Prueba | Meta | Resultado |
| --- | --- | --- |
| Efecto 0 %, nulidad frente a la entrada retrasada | Idéntica | Por debajo de −130 dBFS en 1× a 8× y con M/S |
| Curva monótona, f(0) = 0, f(±1) = ±1 | Todo k y b | Cumple en Curva −50 a +50, Sesgo 0 a 100 y Clip encendido o apagado |
| Pendiente en cero | +1,0 / +3,5 / −2,5 dB | +1,02 / +3,52 / −2,50 dB |
| 3.er armónico (1 kHz, A = 0,5, Curva +50) | ±0,5 dB de la teoría | Cumple; el 2.º queda por debajo de −110 dB sin Sesgo |
| ADAA | Derivada de F igual a f; sin NaN | Cumple en todas las regiones, también con x casi igual a x₋₁ |
| Aliasing, 8 kHz a −6 dBFS, Curva +50, 2× | −70 dBc | **−96 dBc** (límite de medida) a 44,1, 48 y 96 kHz, también con 5 y 9,5 kHz |
| Aliasing de 9,5 kHz con 1× | — | −33 dBc: con 1× el ADAA no basta, por eso el defecto es 2× |
| Aliasing con Input +12 dB, 3,1 kHz, 2× | — | −52 dBc con Clip encendido (lo fija el recorte final) y −66 dBc apagado |
| Band Split, suma sin proceso | ±0,1 dB | Dentro de ±0,1 dB de 40 Hz a 12 kHz a 44,1, 48 y 96 kHz |
| Ganancia con señal baja y Curva 0 | +1,0 dB | +1,02 dB ±0,1 de 100 Hz a 10 kHz, con y sin bandas |
| Clip 0 dB, Input ≤ 0 dB | Nada sobre 0 dBFS | Máximo 1,0 con todos los sobremuestreos; apagado con Input +12 dB llega a 2,73 |
| Igualar sonoridad | ±0,2 LU | Cumple con Curva +50 y −50 (ruido con envolvente lenta, 30 s) |
| VU | 99 % en 300 ms; una senoidal marca su valor eficaz | 0,99 ±0,006; valor eficaz dentro de 0,5 % |
| Sin clics (15 controles y cambios de estructura) | ≤ 1,3 veces la señal estable | Cumple |
| Robustez | Sin NaN ni picos | Silencio, continua, NaN, infinito y subnormales; 44,1 a 192 kHz; bloques de 1 a 4096 |
| `processBlock` | Sin reservas de memoria | 0 reservas (mono y estéreo, con parámetros aleatorios) |
| CPU, banda completa 2× (% de un núcleo) | < 1,5 % | **0,32 %** |
| CPU, 3 bandas a 4× | < 4 % | **1,24 %** (8×: 2,0 %) |

## Cómo repetir las medidas

```bash
ctest --test-dir build                          # todas las pruebas
./build/Cremita/CremitaTests                    # DSP y medidas
./build/Cremita/CremitaProcessorTests           # procesador completo con editor
```

# Compatibilidad y validación

| Validador | Resultado |
| --- | --- |
| `auval -v aufx Crem Ferb` | Pasa |
| `pluginval` nivel de rigor 10 (VST3 y AU, con pruebas de interfaz) | Pasa |
| `CremitaTests` y `CremitaProcessorTests` | Pasan |

Pendiente: comprobación manual en Ableton Live, Logic Pro y Pro Tools.

# Limitaciones y pendientes

- **Sin prueba manual en anfitriones.** Falta probarlo a mano en Ableton, Logic y Pro Tools.
- **Sesgo y Curva:** con `b = Sesgo · min(0,3; 1 − 2k)`, en Curva +50 se tiene k = 0,5 y b = 0: el Sesgo deja de actuar. Es consecuencia de exigir que la curva sea monótona.
- **Valores iniciales por afinar de oído:** los coeficientes de la curva, las ganancias por banda y la mitad de los presets de fábrica.
- **Interfaz:** la *Referencia VU* no tiene control (es un parámetro automatizable) y el editor de bandas solo se maneja con el ratón.
- **A/B** no se guarda con la sesión.
- **AAX (Pro Tools):** el proyecto lo soporta con el SDK de Avid y firma PACE/iLok, pero no está compilado ni probado; sin firmar solo carga en Pro Tools Developer.
- **Distribución:** firma ad hoc, sin notarización. Antes de distribuirlo hay que revisar las condiciones de licencia de JUCE.

# Historial de versiones

| Versión | Fecha | Cambios |
| --- | --- | --- |
| **1.0.0** | {{fecha}} | Primera edición numerada. Reúne los hitos M0 a M5: curva propia con ADAA y sobremuestreo, bandas LR4, Mid/Side, Clip, igualar sonoridad, vúmetros, parámetros, 10 presets, A/B, deshacer e interfaz con editor de bandas; sin clics y sin reservas en `processBlock`. |

El número de versión sigue SemVer y está en `cmake/Versions.cmake`: **mayor** si algo rompe presets o sesiones (IDs de parámetros, formato de estado), **menor** para funciones nuevas compatibles y **parche** para correcciones.

# Avisos {.ap}

- **Licencias de software.** Cremita está construido con JUCE 8.0.15 y las pruebas con Catch2. Las condiciones de licencia de JUCE deben revisarse antes de cualquier distribución.
- **Tipografías.** Limelight, Josefin Sans y Oxanium, incrustadas en el plugin bajo sus licencias abiertas (ver `ui/vintage/fonts/LICENSE.md`).
- **Marcas de terceros.** Los nombres de productos de otras empresas se citan solo como referencia funcional y pertenecen a sus propietarios. FERB no está afiliado a ellas.
- **Uso.** Software de uso personal, entregado tal cual, sin garantía.
