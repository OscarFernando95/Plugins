# Visión general {.pb}

**Poty** es un enhancer multibanda moderno: un ecualizador de realce de cinco bandas cuya parte añadida pasa por un saturador. Así, **solo se generan armónicos donde se realza**: lo que no se toca queda intacto y lo que se realza gana carácter, en vez de saturar todo el espectro.

Cada banda elige su propio tipo de saturación (Tubo, Cinta, Sólido, Diodo Ge o Digital). Hay modos Estéreo, Mid y Side, y una interfaz con analizador de espectro y nodos arrastrables. Su referencia funcional es el concepto del enhancer de realce paralelo (el Spectre de Wavesfactory); el motor de Poty es propio.

<div class="kpi">
<div><b>1,7 %</b><span>CPU de un núcleo, 5 bandas, calidad Mejor</span></div>
<div><b>−96 dB</b><span>Aliasing en Máxima (ajustes de fábrica)</span></div>
<div><b>47 muestras</b><span>Latencia constante, 0,98 ms a 48 kHz</span></div>
<div><b>5 × 5</b><span>Cinco bandas, cinco tipos de saturación</span></div>
</div>

<figure class="shot"><img src="{{img}}/poty.png" style="width:100%"><figcaption>Interfaz de Poty con el preset «Brillo vocal» (1100 × 710 px base; la ventana se redimensiona del 60 % al 200 %).</figcaption></figure>

## Características

- **Cinco bandas de realce** (frecuencia 20 Hz a 20 kHz, ganancia 0 a +12 dB, Q 0,3 a 4), cada una con su tipo de saturación y su drive.
- **Saturación solo del realce.** La salida es la señal original más la suma de lo que añade cada banda, escalada por la Mezcla.
- **Modos Estéreo, Mid y Side:** en Mid o Side solo ese canal pasa por las bandas y el otro se deja intacto.
- **Tres calidades** (Buena 2×, Mejor 4×, Máxima 8×), todas con ADAA de primer orden.
- **Analizador de espectro** de entrada y salida y curva del ecualizador, con nodos que se arrastran.
- **Sin clics** al mover cualquier control; **latencia constante** de 47 muestras en todas las calidades.
- **Presets:** 5 de fábrica, guardar, guardar como, eliminar y anterior o siguiente.

## Cuándo usarlo y cuándo no

Poty sirve para dar brillo, presencia, cuerpo o aire a una pista o a un bus sin que el espectro entero se sature: la banda 1 puede calentar los graves con un tubo mientras la 5 añade aire con cinta. No comprime ni limita, no tiene sidechain ni dinámica por banda, y **solo realza** (la ganancia va de 0 a +12 dB; no recorta bandas).

# Instalación y compatibilidad

| Tema | Detalle |
| --- | --- |
| Sistema | macOS 11 o superior, binario universal (Apple Silicon e Intel) |
| Formatos | VST3 y AU (AUv2, tipo efecto) |
| Canales | Estéreo a estéreo y mono a mono, con modos Estéreo, Mid y Side |
| Frecuencias de muestreo | 44,1 a 192 kHz, tamaño de bloque variable |
| Latencia | 47 muestras, constante en todas las calidades |
| Fuera de alcance | Sexta banda, dinámica, sidechain, MIDI, bit-crusher, Windows, Linux |

## Dónde se instala

| Formato | Carpeta |
| --- | --- |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/Poty.vst3` |
| Audio Unit | `~/Library/Audio/Plug-Ins/Components/Poty.component` |
| Presets del usuario | `~/Library/Application Support/FERB/Poty/Presets` |

Cada compilación copia el plugin a esas carpetas y lo firma «ad hoc». Si el paquete se lleva a otro equipo, macOS puede bloquearlo por no estar notarizado; se desbloquea con `xattr -dr com.apple.quarantine <paquete>`. Tras actualizar la versión de un AU, el anfitrión puede necesitar volver a escanear los plugins.

> **Aviso.** Poty no está notarizado ni firmado con Developer ID. Está pensado para uso personal en el equipo donde se compila.

# Guía de uso {.pb}

## Primeros pasos

1. Inserta Poty en una pista o bus. El preset de fábrica «Brillo vocal» ya realza cinco zonas.
2. Elige una banda (clic en su nodo del analizador o en su tarjeta) y **arrastra el nodo**: en horizontal cambia la frecuencia y en vertical la ganancia.
3. Elige el **tipo de saturación** de la banda en su tarjeta y ajusta su **Drive**.
4. Controla el conjunto con **Mezcla** (cuánto del realce saturado se suma) y **Salida**.

## Mapa de la interfaz

| Zona | Contenido |
| --- | --- |
| Cabecera | Marca de cinco barras de colores, nombre con la píldora **By FERB**, selector de preset, segmento Modo (Estéreo, Mid, Side) y Bypass |
| Analizador | Espectro de entrada (relleno) y de salida (línea), curva del ecualizador, cinco nodos numerados y etiqueta de la banda seleccionada. De 20 Hz a 20 kHz y de 0 a −80 dB |
| Tarjetas de banda (5) | Interruptor de banda (punto de color), Frecuencia, Ganancia, Q, barra de Drive y selector de tipo. La seleccionada lleva borde de su color |
| Panel global | Perillas Entrada, Salida y Mezcla; segmentos Calidad e Intensidad |

## Controles globales

| Control | Qué hace | Rango (defecto) |
| --- | --- | --- |
| **Entrada** | Ganancia antes de las bandas | −12 a +12 dB (0) |
| **Salida** | Ganancia final | −12 a +12 dB (−0,5) |
| **Mezcla** | Cuánto del camino saturado se suma al original | 0 a 100 % (70) |
| **Calidad** | Sobremuestreo del saturador: Buena (2×), Mejor (4×), Máxima (8×). Más calidad, menos aliasing y más CPU | Mejor |
| **Intensidad** | Escala el drive de todas las bandas: Sutil (×0,6), Media (×1), Agresiva (×1,8) | Media |
| **Modo** | Qué canal pasa por las bandas: Estéreo, Mid o Side | Estéreo |
| **Bypass** | Deja pasar la señal original con un fundido de 10 ms | Apagado |

## Controles por banda

| Control | Qué hace | Rango |
| --- | --- | --- |
| **Interruptor** (punto de color de la tarjeta) | Activa o apaga la banda; una banda apagada no se procesa | on / off |
| **Frecuencia** | Centro del realce (escala logarítmica) | 20 Hz a 20 kHz |
| **Ganancia** | Realce de la banda. Con 0 dB la banda no hace nada | 0 a +12 dB |
| **Q** | Anchura del realce (escala logarítmica) | 0,3 a 4 |
| **Drive** | Cantidad de saturación del realce | 0 a 100 % |
| **Tipo** | Carácter de la saturación | Tubo, Cinta, Sólido, Diodo Ge, Digital |

### Tipos de saturación

| Tipo | Carácter |
| --- | --- |
| **Tubo** | Asimétrica: armónicos pares e impares, cálida |
| **Cinta** | Simétrica y suave, con compresión de agudos (lleva un realce y su inverso de +3 dB a 2 kHz alrededor de la curva) |
| **Sólido** | Simétrica, de rodilla más marcada |
| **Diodo Ge** | Recorte asimétrico de diodo de germanio |
| **Digital** | Recorte duro simétrico |

## Gestos del ratón

| Dónde | Gesto | Efecto |
| --- | --- | --- |
| Nodo del analizador | Clic | Selecciona la banda y muestra su etiqueta |
| Nodo del analizador | Arrastrar | Frecuencia (horizontal) y ganancia (vertical) |
| Nodo del analizador | Rueda | Cambia la Q de la banda (bajo el cursor o la seleccionada) |
| Nodo del analizador | Doble clic | Restablece frecuencia, ganancia y Q de la banda a su valor de fábrica |
| Perillas | Arrastrar · rueda · doble clic | Cambia el valor · cambia el valor · valor por defecto |
| Campos de valor | Arrastre vertical (**Mayús** afina) · doble clic · **Alt** + clic | Cambia el valor · escribe un valor a mano · valor por defecto |

La ventana se redimensiona arrastrando la esquina inferior derecha, del 60 % al 200 % con proporción fija. El tamaño se guarda con la sesión.

## Presets

El selector de la cabecera tiene flechas para pasar al preset anterior o siguiente y un menú con los de fábrica, los del usuario y las órdenes **Guardar**, **Guardar como…**, **Eliminar** y **Mostrar carpeta de presets**. Un asterisco indica que se modificó el preset cargado. Un preset guarda todos los controles salvo el Bypass; la banda seleccionada no es un parámetro.

| Preset | Modo | Mezcla | Calidad · Intensidad | Idea |
| --- | --- | --- | --- | --- |
| **Brillo vocal** (defecto) | Estéreo | 70 % | Mejor · Media | Realce en cinco zonas, con diodo de germanio en 5 kHz |
| **Calidez de bus** | Estéreo | 55 % | Mejor · Sutil | Realces suaves con tubo y cinta |
| **Aire y brillo** | Estéreo | 50 % | Máxima · Sutil | Aire en agudos (+6 dB a 8 kHz, +9 dB a 16 kHz) |
| **Graves con cuerpo** | Mid | 75 % | Mejor · Media | Tubo en 55 y 120 Hz, solo sobre el centro de la mezcla |
| **Presencia agresiva** | Estéreo | 80 % | Mejor · Agresiva | Germanio y digital en medios y agudos |

## Consejos de uso

- **Más realce no es mejor.** La saturación aparece cuando el realce empuja niveles altos; sube la ganancia de la banda o su drive, no ambos a la vez.
- **Compara a igual volumen:** ajusta Salida para igualar el nivel con el Bypass.
- **Graves en un bus estéreo:** usa el modo Mid para que el realce no toque los lados.
- **Aliasing:** para materiales con mucho agudo y drive alto, sube Calidad a Máxima (cuesta 3,3 % de un núcleo).
- **Una banda con ganancia 0 no consume CPU:** se salta por completo.

> **Nota.** Los valores de drive efectivo, de la intensidad y de los tipos de saturación son iniciales y pueden afinarse de oído.

# Referencia de parámetros

Son **37 parámetros**: 7 globales y 6 por banda. Todos pasan por `AudioProcessorValueTreeState` con identificador de versión 1, son automatizables y se guardan en el estado. **No se renombran:** hacerlo rompe presets y sesiones guardadas.

## Globales

| ID | Nombre en la interfaz | Tipo | Rango | Defecto | Notas |
| --- | --- | --- | --- | --- | --- |
| `inGain` | Entrada | Decimal | −12 a +12 dB | 0 | Suavizado de 20 ms |
| `outGain` | Salida | Decimal | −12 a +12 dB | −0,5 | Suavizado de 20 ms |
| `mix` | Mezcla | Decimal | 0 a 100 % | 70 | Escala el camino saturado |
| `quality` | Calidad | Opciones | Buena (2×), Mejor (4×), Máxima (8×) | Mejor | Fundido de 5 ms al cambiarla |
| `intensity` | Intensidad | Opciones | Sutil, Media, Agresiva | Media | Escala el drive: ×0,6, ×1, ×1,8 |
| `mode` | Modo | Opciones | Estéreo, Mid, Side | Estéreo | Fundido de 5 ms al cambiarlo |
| `bypass` | Bypass | Sí/No | apagado o encendido | Apagado | Es el bypass del anfitrión; no forma parte de los presets |

## Por banda

El identificador es el prefijo `b1_` a `b5_` más el sufijo.

| Sufijo | Tipo | Rango | Notas |
| --- | --- | --- | --- |
| `on` | Sí/No | apagado o encendido | Una banda apagada no se procesa |
| `freq` | Decimal | 20 a 20 000 Hz | Escala logarítmica |
| `gain` | Decimal | 0 a +12 dB | Solo realce |
| `q` | Decimal | 0,3 a 4 | Escala logarítmica |
| `type` | Opciones | Tubo, Cinta, Sólido, Diodo Ge, Digital | Mezcla de 10 ms al cambiarlo |
| `drive` | Decimal | 0 a 100 % | Suavizado de 20 ms |

| Banda | Frecuencia | Ganancia | Q | Tipo | Drive |
| --- | --- | --- | --- | --- | --- |
| 1 | 90 Hz | +5,0 dB | 0,8 | Tubo | 45 % |
| 2 | 350 Hz | +3,0 dB | 1,0 | Cinta | 30 % |
| 3 | 1,8 kHz | +6,0 dB | 1,1 | Sólido | 55 % |
| 4 | 5,0 kHz | +8,0 dB | 1,2 | Diodo Ge | 70 % |
| 5 | 12 kHz | +7,0 dB | 0,7 | Digital | 40 % |

**Estado de la interfaz** (no automatizable): `uiScale` (tamaño de la ventana) y `presetName`. La banda seleccionada es estado de la ventana, no un parámetro.

# Documentación técnica

## Arquitectura de señal

{{fig:flujo}}

La salida es la señal original más la suma de lo que añade cada banda, escalada por la mezcla:

<div class="formula">y = x + Mezcla · Σ<sub>i=1…5</sub> g<sub>i</sub>(Δ<sub>i</sub>) &nbsp;&nbsp; con &nbsp;&nbsp; Δ<sub>i</sub> = realce<sub>i</sub>(x) − x</div>

- **Realce por banda.** Un filtro de campana (biquad) con la frecuencia, ganancia y Q de la banda. Δᵢ es solo lo que el realce añade a la señal.
- **Saturador normalizado.** gᵢ(u) = f(d·u)/d, con f′(0) = 1: con señal pequeña la banda se comporta como un realce exacto, y la saturación aparece cuando el realce empuja niveles altos. Con los saturadores en modo lineal el resultado es un ecualizador de realce en paralelo (la prueba de referencia).
- **Drive efectivo:** d = 1 + 19 · Drive/100 · escala de Intensidad (×0,6, ×1 o ×1,8).
- **Modo:** en Mid o Side solo ese canal pasa por las bandas y el otro se deja intacto. La conversión a mid/side solo se hace cuando el modo no es Estéreo.
- Las bandas apagadas o con ganancia 0 se saltan por completo.

## Tipos de saturación

Cada tipo es una función f(v) con f′(0) = 1, aplicada a v = d·Δ y dividida entre d. Todas tienen antiderivada cerrada, así que admiten ADAA de primer orden.

| Tipo | f(v) | Antiderivada F(v) |
| --- | --- | --- |
| Tubo | `(tanh(v + b) − tanh(b)) / sech²(b)`, con `b = 0,25` | `(log cosh(v + b) − tanh(b)·v) / sech²(b)` |
| Cinta | `tanh(v)`, entre un realce de agudos de +3 dB a 2 kHz y su inverso | `log cosh(v)` |
| Sólido | `v / √(1 + v²)` | `√(1 + v²)` |
| Diodo Ge | `asinh(k·v) / k`, con `k = 3` si `v ≥ 0` y `k = 2` si `v < 0` | `(v·asinh(k·v) − √(1 + k²v²)/k) / k`, por mitades, continua en 0 |
| Digital | `clamp(v, −1, 1)` | `v²/2` dentro de ±1 y `\|v\| − 1/2` fuera |

{{fig:satur}}

Los valores `b`, `k` y el realce de la cinta son iniciales y se afinan de oído. `log cosh` usa la forma estable `|v| + log1p(exp(−2|v|)) − log(2)`.

## ADAA de primer orden

El antialiasing evalúa la antiderivada en lugar de la función, y cae al punto medio cuando dos muestras consecutivas casi coinciden:

```cpp
// f es la función, F su antiderivada (en doble precisión)
double adaa(double v, double& vPrev, double& FPrev) {
    double Fv = F(v);
    double dv = v - vPrev;
    double y  = std::abs(dv) > 1e-4 ? (Fv - FPrev) / dv
                                    : f(0.5 * (v + vPrev));
    vPrev = v;  FPrev = Fv;
    return y;
}
```

Se calcula en `double` porque restar `F(v) − F(v₋₁)` en `float` mete ruido; con `double` el punto medio es exacto a ese umbral (1e-4).

## Sobremuestreo y latencia

El sobremuestreo se aplica solo al camino saturado, con etapas de media banda FIR propias (Kaiser, unos 100 dB de rechazo, fase lineal): Buena es 2×, Mejor 4× y Máxima 8×. La señal original se retrasa lo mismo que el camino saturado y esa latencia es **constante, 47 muestras** (la del 8×), en todas las calidades. Cambiar de calidad nunca cambia la latencia ni obliga al anfitrión a recalcular la compensación.

| Frecuencia de muestreo | 44,1 kHz | 48 kHz | 96 kHz | 192 kHz |
| --- | --- | --- | --- | --- |
| Latencia (47 muestras) | 1,07 ms | 0,98 ms | 0,49 ms | 0,24 ms |

## Analizador de espectro y curva del ecualizador

- **Captura:** el hilo de audio copia la mezcla mono de entrada y de salida a colas sin bloqueos. Si el editor está cerrado, no copia nada.
- **FFT de 8192 puntos** con ventana Hann, a 30 Hz, tomada de la frecuencia base (no la sobremuestreada). A 48 kHz da unos 6 Hz por intervalo, suficiente para las bandas desde 90 Hz.
- **Balística:** sube de inmediato y baja 15 dB por segundo. Se dibuja de 20 Hz a 20 kHz en escala logarítmica y de 0 a −80 dB.
- **Curva del EQ:** suma en dB de la respuesta de los cinco filtros sobre 400 puntos logarítmicos, recalculada solo cuando cambia un parámetro de banda.

## Cambios sin clics

| Cambio | Mecanismo |
| --- | --- |
| Entrada, Salida, Mezcla, Drive, Intensidad | Suavizado de 20 ms |
| Frecuencia, Ganancia, Q | Coeficientes del biquad interpolados cada 32 muestras |
| Tipo de saturación | Mezcla de 10 ms entre el tipo anterior y el nuevo |
| Calidad, Modo | Fundido de 5 ms a silencio, cambio, calentamiento y vuelta |
| Bypass | Fundido de 10 ms |

## Reglas de tiempo real

- Sin asignaciones de memoria ni bloqueos en `processBlock`: todo se reserva en `prepare` para el máximo (8×), incluso al cambiar de modo en caliente. Lo vigila una prueba con un asignador instrumentado.
- `juce::ScopedNoDenormals` al inicio del bloque y parámetros suavizados con `juce::SmoothedValue`.
- Interfaz: todo el dibujo corre a 30 Hz y el analizador deja de leer las colas con el editor cerrado.

## Estado y presets

El estado es el árbol de `AudioProcessorValueTreeState` más las propiedades de interfaz `uiScale` y `presetName`. Los presets guardan el valor real de cada parámetro en unidades de la interfaz: los de fábrica van en el código y los del usuario en archivos `.ferbpreset` (XML). Un parámetro que el preset no menciona vuelve a su valor por defecto.

## Estructura del código

| Carpeta | Contenido |
| --- | --- |
| `Poty/source/dsp/` | `BandFilter` (realce), `BandSaturator` (saturador con ADAA), `PotyEngine` (motor independiente del anfitrión) |
| `Poty/source/ui/` | Analizador, tarjetas de banda, panel global, estilo |
| `core/` | DSP común sin JUCE: saturadores, ADAA, sobremuestreador, biquad |
| `ui/` | Presets, escalado de ventana, segmentos y campos de valor comunes |

# Calidad y verificación

Medidas a 48 kHz, estéreo, bloque de 256 y ajustes de fábrica salvo que se indique (28 de octubre de 2026).

| Prueba | Meta | Resultado |
| --- | --- | --- |
| Respuesta lineal frente a la teórica | < 0,1 dB | Peor error 0,015 dB, en 44,1, 48 y 96 kHz, las cuatro calidades internas y mezcla al 70 y 100 % |
| Transparencia, ganancias en 0 | < −90 dBFS | Por debajo de −90 dBFS en todas las calidades |
| Armónicos, senoidal a 1 y 5 kHz | Tubo y Ge con pares; Cinta, Sólido y Digital solo impares | Se cumple: pares por debajo de −80 dB en los simétricos |
| Aliasing, peor caso (banda en la fundamental, +8 dB, casi recorte) | −70 dB (Buena), −80 dB (Mejor y Máxima) | Buena −54 dB · Mejor −68 dB · **Máxima −90 dB** |
| Aliasing con ajustes de fábrica, −6 dBFS, de 1 a 12 kHz | idem | Buena −52 dB · Mejor −73 dB · **Máxima −96 dB** |
| CPU, 5 bandas (% de un núcleo) | ≤ 3 % con calidad Mejor | Buena 1,0 % · **Mejor 1,7 %** · Máxima 3,3 % (Mid o Side, la mitad) |
| Mid y Side | El otro canal queda igual | Residuo por debajo de −90 dB |
| Cambios de calidad, modo, tipo, drive, intensidad, nodos y bypass con audio sonando | Sin clics | Salto máximo entre muestras ≤ 1,3 veces el de la señal estable |
| `processBlock` y memoria | Sin asignaciones | Cero reservas, con cambios de modo en caliente |
| Procesador completo, 18 millones de comprobaciones | Sin NaN ni picos | 44,1 a 192 kHz, bloques de 1 a 4096, mono y estéreo, parámetros aleatorios con audio sonando |

> **Nota.** La meta de aliasing de −70 y −80 dB solo se cumple en el peor caso con **Máxima**. Con los ajustes de fábrica (−6 dBFS) Mejor alcanza −73 dB. Si se quiere cumplir −80 dB incluso en el peor caso, hay que usar Máxima (3,3 % de un núcleo).

## Cómo repetir las medidas

```bash
ctest --test-dir build                         # todas las pruebas
./build/Poty/PotyTests "[.report]"             # informe de aliasing, CPU y respuesta lineal
./build/Poty/PotyProcessorTests                # procesador completo con editor
```

# Compatibilidad y validación

| Validador | Resultado |
| --- | --- |
| `auval -v aufx Poty Ferb` | Pasa. Un aviso conocido: la Q no «retiene» exactamente su valor por defecto por el redondeo de la escala logarítmica |
| `pluginval` nivel de rigor 10 (VST3 y AU) | Pasa |
| `PotyProcessorTests` | Pasa (tasas y bloques extremos, estado, vida del editor, latencia, cero reservas en `processBlock`) |

Pendiente: comprobación manual en Logic Pro y Ableton Live.

# Limitaciones y pendientes

- **Sin prueba manual en anfitriones.** Los validadores y las pruebas automáticas pasan, pero falta comprobarlo a mano en Logic Pro y Ableton Live.
- **Aliasing de Buena y Mejor** por debajo de la meta del peor caso (ver la nota de calidad).
- **Valores iniciales por afinar de oído:** el realce de la cinta, `b` del tubo, `k` del germanio, las escalas de intensidad y la fórmula del drive efectivo.
- **Solo realza:** la ganancia por banda va de 0 a +12 dB.
- **Solo macOS.** Sin Windows, Linux, AAX, CLAP, sidechain ni MIDI. El AAX compila con el SDK de Avid (2.8.0, arm64) pero va sin firmar (PACE), sin validar y sin probar en Pro Tools.
- **Distribución:** firma ad hoc, sin notarización. Antes de distribuirlo hay que revisar las condiciones de licencia de JUCE.

# Historial de versiones

| Versión | Fecha | Cambios |
| --- | --- | --- |
| **1.0.0** | {{fecha}} | Primera edición numerada. Reúne los hitos 1 a 11: 37 parámetros, cinco bandas con cinco tipos de saturación y ADAA, calidades 2×, 4× y 8× con latencia constante, modos Estéreo, Mid y Side, analizador con nodos, presets; sin clics y sin reservas en `processBlock`. |

El número de versión sigue SemVer y está en `cmake/Versions.cmake`: **mayor** si algo rompe presets o sesiones (IDs de parámetros, formato de estado), **menor** para funciones nuevas compatibles y **parche** para correcciones.

# Avisos {.ap}

- **Licencias de software.** Poty está construido con JUCE 8.0.15 y las pruebas con Catch2. Las condiciones de licencia de JUCE deben revisarse antes de cualquier distribución.
- **Tipografías.** Manrope y JetBrains Mono, incrustadas en el plugin bajo sus licencias abiertas (ver `Poty/assets/fonts/LICENSE.md`).
- **Marcas de terceros.** Los nombres de productos de otras empresas se citan solo como referencia funcional y pertenecen a sus propietarios. FERB no está afiliado a ellas.
- **Uso.** Software de uso personal, entregado tal cual, sin garantía.
