# Identidad de documentación FERB

Reglas para que todos los documentos de los plugins se vean y se lean como una sola familia. La implementación es [estilo/ferb-doc.css](estilo/ferb-doc.css); esto explica el porqué y lo que hay que respetar al añadir un plugin.

## Idea

La documentación se parece a lo que documenta: la **portada reproduce el frente del plugin** (marfil y nogal para Tufi y Cremita, oscuro con las cinco barras para Poty, negro y morado para Nel) y el **interior es sobrio, claro y apto para imprimir**. Lo que une a todos es la marca **By FERB** (latón), la tipografía y la estructura fija del documento.

Tono: español neutro, directo, con coma decimal. Cada cifra va con su condición de medida y fecha; lo que no se ha medido o probado se dice (ver «Limitaciones y pendientes»). Nunca se presenta como resultado un valor inicial sin afinar.

## Paleta

| Rol | Valor | Uso |
| --- | --- | --- |
| Tinta | `#2A1D14` | Texto |
| Tinta suave | `#6B5B4B` | Texto secundario, pies |
| Latón | `#B08D4F` | Marca de la familia: «By FERB», filetes de portada |
| Nogal | `#4A2D1A` | Laterales de las portadas vintage |
| Crema | `#F6F1E3` | Fondo de notas, código y tablas con cebra |
| Filete | `#E3DAC6` | Líneas finas |

**Acento por plugin** (viene de su interfaz; es el color de filetes, números de sección y cabeceras de tabla):

| Plugin | Acento | Texto sobre blanco | Fondo de portada |
| --- | --- | --- | --- |
| Tufi | `#B08D4F` latón | `#7E5F22` | `#E4D6B4` marfil |
| Cremita | `#E08A2E` ámbar | `#A8570E` | `#E6DABD` marfil |
| Poty | `#4CC9F0` cian | `#12708C` | `#0E1116` negro azulado |
| Nel | `#8A5CF0` morado | `#6A3FD0` | `#1C1822` negro con morado |

El acento claro solo se usa en rellenos y filetes; el texto usa la variante oscura (contraste suficiente sobre blanco).

## Tipografía

Las mismas fuentes que incrustan los plugins (todas con licencia abierta, en sus carpetas `assets/fonts`):

| Uso | Fuente |
| --- | --- |
| Título de sección y logotipo de Tufi | DM Serif Display |
| Logotipo de Cremita y Nel | Limelight |
| Logotipo de Poty y texto corrido | Manrope (500 y 700) |
| Rótulos en mayúsculas espaciadas | Josefin Sans (600) |
| Código, valores e IDs | JetBrains Mono |

## Estructura fija de cada documento

1. **Portada:** marca, nombre, subtítulo, captura de la interfaz, versión y código de documento (`FERB-<PLUGIN>-DOC`).
2. **Control del documento:** versión, fecha, estado, formatos, identificadores, autor y uso. **Contenido.**
3. **01 Visión general** · **02 Instalación y compatibilidad** · **03 Guía de uso** · **04 Referencia de parámetros** · **05 Documentación técnica** · **06 Calidad y verificación** · **07 Compatibilidad y validación** · **08 Limitaciones y pendientes** · **09 Historial de versiones** · **Apéndice: Avisos.**

Los bloques que se reutilizan: tarjetas de cifras clave (`.kpi`), notas (`> **Nota.**` o `> **Aviso.**`), fórmulas (`.formula`), figuras con pie y tablas de IDs.

## Reglas

- Un PDF por plugin, en `docs/`, con el nombre `<Plugin> — Documentación.pdf`. Se regenera, no se edita a mano.
- La **versión** sale de `cmake/Versions.cmake`; no se escribe a mano en el texto (usa `{{version}}`).
- Las **curvas** de las figuras salen del código del plugin (`docs/herramientas/curvas.cpp`), para que la documentación no pueda contradecir al motor.
- Las **capturas** salen de `*Snapshot` (`python3 docs/construir.py --capturas`).
- Si la ficha y el código discrepan, manda el código y se corrige la ficha.
- Marcas de terceros: solo como referencia funcional, con el aviso del apéndice.

## Añadir un plugin nuevo

1. Escribir `docs/fuentes/<Plugin>.md` siguiendo la estructura de los existentes.
2. Añadirlo al diccionario `PLUGINS` de `docs/construir.py` (acento, formatos, IDs) y, si tiene figuras, a `figures()`.
3. Añadir su variable de versión a `cmake/Versions.cmake` (ya están todas) y su estilo de portada en `ferb-doc.css` (`body.<tema>`).
4. `python3 docs/construir.py --capturas <Plugin>` y revisar el PDF.

## Antes de comercializar (no hecho)

Lo que hoy falta si algún día se distribuye: licencia de JUCE (comercial o GPL), contrato de uso (EULA) y datos de contacto/soporte en el apéndice, comprobar que los nombres de los plugins no chocan con marcas registradas, firma con Developer ID y notarización, instalador, y las pruebas manuales en los anfitriones.
