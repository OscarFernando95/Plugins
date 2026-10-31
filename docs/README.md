# Documentación de los plugins

Un PDF por plugin, con la identidad de [Identidad de documentación.md](Identidad%20de%20documentaci%C3%B3n.md):

| Plugin | Versión | PDF |
| --- | --- | --- |
| Tufi | 1.0.0 | [Tufi — Documentación.pdf](Tufi%20%E2%80%94%20Documentaci%C3%B3n.pdf) |
| Poty | 1.0.0 | [Poty — Documentación.pdf](Poty%20%E2%80%94%20Documentaci%C3%B3n.pdf) |
| Cremita | 1.0.0 | [Cremita — Documentación.pdf](Cremita%20%E2%80%94%20Documentaci%C3%B3n.pdf) |
| Nel | 1.0.0 | [Nel — Documentación.pdf](Nel%20%E2%80%94%20Documentaci%C3%B3n.pdf) |
| ChauS | 1.0.0 | [ChauS — Documentación.pdf](ChauS%20%E2%80%94%20Documentaci%C3%B3n.pdf) |
| Maru | 1.0.0 | [Maru — Documentación.pdf](Maru%20%E2%80%94%20Documentaci%C3%B3n.pdf) |

Cada PDF reúne guía de uso, referencia de parámetros, documentación técnica (DSP, latencia, tiempo real), resultados medidos, validación, limitaciones e historial de versiones.

## Regenerarlos

Hace falta Google Chrome y el entorno de Python con el paquete `markdown` (se crea una vez):

```bash
python3 -m venv .cache/docs-venv && .cache/docs-venv/bin/pip install markdown
.cache/docs-venv/bin/python docs/construir.py                 # todos
.cache/docs-venv/bin/python docs/construir.py Tufi            # solo uno
.cache/docs-venv/bin/python docs/construir.py --capturas      # además, vuelve a renderizar las capturas (requiere build/)
```

## Carpetas

| Ruta | Contenido |
| --- | --- |
| `fuentes/<Plugin>.md` | Texto de cada documento (Markdown). **Aquí se edita.** |
| `estilo/ferb-doc.css` | Hoja de estilo de la identidad |
| `figuras.py` · `herramientas/curvas.cpp` | Figuras: curvas reales volcadas del código del plugin y diagramas de flujo |
| `imagenes/` | Capturas de las interfaces (`*Snapshot`) |
| `construir.py` | Genera los PDF; lee la versión de `cmake/Versions.cmake` |

Cuando cambie un plugin (parámetros, medidas, versión): actualizar su fuente, ejecutar `construir.py` y subir el PDF junto con el cambio.
