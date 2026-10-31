# Historial de cambios

Formato basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/); cada plugin sigue [SemVer](https://semver.org/lang/es/) con su propio número (ver `cmake/Versions.cmake`). Las fechas son AAAA-MM-DD.

- **Mayor:** algo rompe presets o sesiones (IDs de parámetros, formato del estado).
- **Menor:** funciones nuevas compatibles.
- **Parche:** correcciones sin cambio de funciones.

## Sin publicar

### Repositorio
- Kit de interfaz: `VButton` con lámpara dibuja el texto en el color de la tinta (antes salía negro al 50 %, también en «BYPASS»); `GlassView` admite un bisel fino opcional.
- README del repositorio y este historial.
- Versión de cada plugin en `cmake/Versions.cmake` (todos en 1.0.0), propagada al paquete.
- Documentación en PDF de Tufi, Poty, Cremita, Nel y ChauS (`docs/`), con identidad propia y generador (`docs/construir.py`).

## ChauS 1.0.0 — 2026-10-31

De-esser con EQ dinámico que sigue la sibilante (VST3 y AU). Reúne los hitos M0 a M5.
- Detector de sibilantes: seis rasgos espectrales (los cinco de la ficha más el «aire»), regresión logística con histéresis (0,6 / 0,4) y suavizado de 5 ms; nivel de banda en el dominio del tiempo; umbral adaptativo (percentil 90 de 5 s).
- EQ dinámico: campana o estante con seguimiento de frecuencia y de ancho, banda ancha, lookahead de 0 a 15 ms con la latencia informada, enlace estéreo, Mid y Side, sidechain externo, escuchas de sidechain y de disparo, salida y bypass del anfitrión.
- 24 parámetros (IDs fijados), 10 presets de fábrica, A/B, deshacer y rehacer; interfaz de la maqueta con espectro por bandas de 1/6 de octava, la campana real del filtro y una línea de tiempo de 3 s, más un menú de ajustes avanzados.
- Sin clics, sin reservas de memoria en `processBlock` y CPU de 0,6 % (estéreo, 48 kHz).
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
