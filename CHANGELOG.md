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
- Documentación en PDF de Tufi, Poty, Cremita y Nel (`docs/`), con identidad propia y generador (`docs/construir.py`).

### ChauS (en curso, aún sin versión publicada)
- M0: esqueleto (código `ChaS`/`Ferb`, VST3 y AU), paso directo, sidechain opcional, editor base 960×580, `ChauSProcessorTests` y `ChausSnapshot`.
- M1: detector de sibilantes (seis rasgos espectrales, regresión logística con histéresis, nivel de banda en el dominio del tiempo) y umbral adaptativo (percentil 90, 5 s); `ChauSTests`.
- M2: motor de EQ dinámico (campana y estante con seguimiento de frecuencia y Q, banda ancha, lookahead de 0 a 15 ms con latencia informada, enlace estéreo, Mid/Side, sidechain externo, escuchas, salida y bypass).
- M3: 24 parámetros (IDs fijados), 10 presets de fábrica, A/B y deshacer, latencia declarada por el procesador.
- M4: interfaz completa (espectro por bandas de 1/6 de octava con la campana dinámica y lo que se quita, línea de tiempo de 3 s con eventos, umbral efectivo y reducción), controles de la maqueta más un menú de ajustes avanzados.

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
