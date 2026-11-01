# Historial de cambios

Formato basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/); cada plugin sigue [SemVer](https://semver.org/lang/es/) con su propio número (ver `cmake/Versions.cmake`). Las fechas son AAAA-MM-DD.

- **Mayor:** algo rompe presets o sesiones (IDs de parámetros, formato del estado).
- **Menor:** funciones nuevas compatibles.
- **Parche:** correcciones sin cambio de funciones.

## Sin publicar

### Dra. McPoti 0.9.0 (preliminar)
- Interfaz (disposición de la maqueta): palancas de tuerca hexagonal, selector rotativo de Voicing, válvulas de vidrio con placa y rango, cruces entre módulos, vúmetros con marco negro, Guardar y joya de encendido. Corrige dos fallos de la plantilla de modo: pisaba el 8× de un preset (tolerancia de un paso entero) y marcaba el preset como modificado al mover los cruces.
- Interfaz: perillas con escala numérica (`ScaleKnob`) y palancas cromadas (`ChromeToggle`) en `DraMcPoti/source/ui/Widgets.h`; PDF con la captura real junto a la maqueta.
- Interfaz, paso 1 de la fidelidad a `Dra. McPoti — maqueta.html`: `Palette::mcpoti()` en el kit (frente pizarra, orejas y bisel de acero, ámbar). **La interfaz sigue sin parecerse a la maqueta** (faltan palancas cromadas, selector rotativo, válvulas de vidrio, escala 0 a 10, vúmetros de marco negro, pantalla de presets y la disposición): se construyó solo desde la ficha, sin abrir la maqueta.
- Documentación en PDF (`docs/fuentes/DraMcPoti.md`) y registro en `docs/construir.py`. `pluginval` nivel 10 y `auval` pasan (VST3 y AU). Versión 0.9.0: 1.0.0 al cerrar la interfaz y escuchar.
- Fase 2 (parcial): `ferb::FourBandSplit` en `core/Crossover.h` (4 bandas LR4, suma plana ±0,1 dB) y `McPotiEngine` con los cuatro caracteres Vintage, dinámica por banda, Solo, Delta y Mezcla; `DraMcPotiTests`. Sobremuestreo de 1× a 8× con ADAA y latencia constante (aliasing medido: 2× −63,3 dB, no cumple la meta de −70; 4× y 8× −96,5 dB). Fundido sin clics al cambiar de factor o de Voicing. Oversampling por defecto 4× (−96,5 dB, 3,0 % de CPU). Los cuatro caracteres Modernos: Seda (shelf paralelo de +15 dB por banda), Dinámico (compander), Aire (shelf que se aplana al comprimir) y Adaptativo (Válvula guiada por el balance tonal). Color, Fase, Suavizar, Ancho y Mono seguro, guarda de sibilancia e Igualar y fase lineal (FIR de 2049 coeficientes, +1024 muestras de latencia solo con ella, 11,6 % de CPU a 4×). Plantilla por modo. Cinta con pre-énfasis (aliasing a 2×: −61,1 dB). Interfaz: válvulas que brillan, nombre del carácter, atenuación por modo y era, dos vúmetros y ventana de armónicos.
- Fase 1: carpeta `DraMcPoti/`, 44 parámetros, 20 presets de fábrica más Init, A/B, panel provisional con el kit `ui/vintage/`, `DraMcPotiProcessorTests` y `DraMcPotiSnapshot`. El audio pasa sin procesar; el DSP llega en las fases 2 a 6. Ficha con su sección «Estado y cambios sobre la ficha».

### Repositorio
- AAX: los seis plugins compilan con el SDK de AAX 2.8.0 (arm64). Sin firma PACE, sin AAX Validator y sin probar en Pro Tools.
- Kit de interfaz: `VButton` con lámpara dibuja el texto en el color de la tinta (antes salía negro al 50 %, también en «BYPASS»); `GlassView` admite un bisel fino opcional.
- README del repositorio y este historial.
- Versión de cada plugin en `cmake/Versions.cmake` (todos en 1.0.0), propagada al paquete.
- Documentación en PDF de Tufi, Poty, Cremita, Nel, ChauS y Maru (`docs/`), con identidad propia y generador (`docs/construir.py`).

## Maru 1.0.0 — 2026-10-31

Supresor de ruido de un solo control y gate (VST3 y AU). Reúne los hitos M0 a M5.
- Estimador de piso de ruido (MCRA) y motor de Supresión espectral (STFT √Hann, ganancia LSA con decision-directed, protección de transitorios, enlace estéreo, latencia N exacta de 1024 o 512 muestras a 48 kHz).
- Motor Gate: detector con HP y LP o sidechain externo, umbral automático sobre el piso de ruido de la banda, histéresis, hold, ataque y release con rampa de coseno, lookahead de 0 a 10 ms.
- 19 parámetros (IDs fijados), 10 presets de fábrica, A/B, deshacer y rehacer; interfaz de rack de la maqueta con ventana del piso de ruido, vúmetro y lámpara, y una segunda fila de controles en Gate.
- Sin clics, sin reservas de memoria en `processBlock` y CPU de 0,41 % (estéreo, 48 kHz).
- Validación: `auval` y `pluginval` nivel 10 con interfaz (VST3 y AU). Con tus grabaciones: la voz cantada queda a −0,02 dB de mediana y las pausas bajan 8–11 dB al 50 %. **Límites conocidos:** el estimador se infla ≈ 7 dB con voz continua, el Rango no se alcanza (≈ 21,5 dB), y falta escucharlo, el STOI y el AAX. Detalle en la ficha.

## ChauS 1.0.0 — 2026-10-31

De-esser con EQ dinámico que sigue la sibilante (VST3 y AU). Reúne los hitos M0 a M5.
- Detector de sibilantes: seis rasgos espectrales (los cinco de la ficha más el «aire»), regresión logística con histéresis (0,6 / 0,4) y suavizado de 5 ms; nivel de banda en el dominio del tiempo; umbral adaptativo (percentil 90 de 5 s).
- EQ dinámico: campana o estante con seguimiento de frecuencia y de ancho, banda ancha, lookahead de 0 a 15 ms con la latencia informada, enlace estéreo, Mid y Side, sidechain externo, escuchas de sidechain y de disparo, salida y bypass del anfitrión.
- 24 parámetros (IDs fijados), 10 presets de fábrica, A/B, deshacer y rehacer; interfaz de la maqueta con espectro por bandas de 1/6 de octava, la campana real del filtro y una línea de tiempo de 3 s, más un menú de ajustes avanzados.
- Sin clics, sin reservas de memoria en `processBlock` y CPU de 0,6 % (estéreo, 48 kHz).
- Valores de fábrica probados con grabaciones reales (Umbral −36, Rango 10: reducción media de unos 3 dB en las eses de una voz) y entrada progresiva de la reducción de 8 ms para plosivas y chasquidos.
- Validación: `auval` y `pluginval` nivel 10 con interfaz (VST3 y AU). **Límites conocidos:** el detector se ajustó con señales sintéticas (hi-hat y «t» activan el detector; sin medir con voz grabada) y la reducción no llega a 0 ms respecto del inicio de la s. Detalle en la ficha.

## Cremita 1.0.0 — 2026-10-31

Inflator sin compresión (VST3 y AU). Reúne los hitos M0 a M5.
- Curva de transferencia propia (Curva y Sesgo), ADAA de primer orden con reposición de respuesta plana, sobremuestreo 1× a 8×.
- Tres bandas LR4 con cruces y peso por banda, Mid/Side y Clip 0 dB al final de la cadena.
- Igualar sonoridad (BS.1770), vúmetros de aguja, testigos de Efecto y Sobrecarga.
- 18 parámetros, 10 presets de fábrica, A/B, deshacer y rehacer, ventana redimensionable del 75 % al 200 %.
- Latencia constante de 47 muestras; sin clics y sin reservas de memoria en `processBlock`.
- Validación: `auval` y `pluginval` nivel 10 (VST3 y AU).

## Poty 1.0.0 — 2026-10-31

Enhancer multibanda (VST3 y AU). Reúne los hitos 1 a 11 (28 oct 2026).
- Cinco bandas de realce con cinco tipos de saturación (Tubo, Cinta, Sólido, Diodo Ge, Digital) y ADAA.
- Calidades 2×, 4× y 8× con latencia constante de 47 muestras; modos Estéreo, Mid y Side.
- Analizador de espectro y curva del ecualizador con nodos arrastrables; 37 parámetros; 5 presets de fábrica.
- Sin clics y sin reservas de memoria en `processBlock`.
- Validación: `auval` y `pluginval` nivel 10 (VST3 y AU).

## Tufi 1.0.0 — 2026-10-31

Saturador armónico analógico (VST3 y AU). Reúne los hitos 1 a 9 (28 oct 2026).
- Tres etapas (Triodo, Pentodo, Diodo) con controles de forma (Nivel bajo, Picos, Pares), ADAA y sobremuestreo hasta 8×.
- Latencia constante de 47 muestras; osciloscopio de la curva, VU y válvula; 5 presets de fábrica.
- Sin clics y sin reservas de memoria en `processBlock`.
- Validación: `auval` y `pluginval` nivel 10 (VST3 y AU).

## Nel 1.0.0

Limitador (VST3 y AU), con motor, sobremuestreador de 1× a 32×, parámetros, presets, A/B, interfaz vintage y validación (`pluginval` nivel 10). Versión 1.0.0 asignada; documentado en PDF; falta la prueba en los anfitriones.

## Maru y ChauS

Por hacer; su 1.0.0 está prevista pero aún no existe el plugin.
