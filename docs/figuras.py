"""Figuras SVG de la documentación: curvas reales (volcadas del código del plugin) y diagramas de flujo.

Las curvas salen de docs/herramientas/curvas.cpp, que compila el mismo código del plugin; así la
documentación no puede contradecir al motor. Todo se dibuja con la paleta de la identidad FERB.
"""
from __future__ import annotations

from html import escape

INK = "#2A1D14"
SOFT = "#6B5B4B"
GRID = "#E3DAC6"
PAPER = "#FFFFFF"
FONT = "Manrope, sans-serif"
MONO = "'JetBrains Mono', monospace"


def _svg(w: int, h: int, body: str) -> str:
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w}" height="{h}" '
            f'font-family="{FONT}">{body}</svg>')


def _text(x, y, s, size=11, fill=INK, anchor="start", weight=500, mono=False):
    fam = f' font-family="{MONO}"' if mono else ""
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" fill="{fill}" text-anchor="{anchor}" '
            f'font-weight="{weight}"{fam}>{escape(s)}</text>')


def parse_curves(raw: str) -> dict[str, list[tuple[float, float]]]:
    out: dict[str, list[tuple[float, float]]] = {}
    for line in raw.splitlines():
        name, x, y = line.split()
        out.setdefault(name, []).append((float(x), float(y)))
    return out


def plot(series: list[tuple[str, str, list[tuple[float, float]]]], xlim: float, ylim: float,
         xlabel: str, ylabel: str, width=330, height=330, ticks=(-1, -0.5, 0, 0.5, 1), diagonal=True,
         legend=True, ylow: float | None = None) -> str:
    """Curvas f(x) en un recuadro cuadrado. `series`: (rótulo, color, puntos)."""
    ml, mr, mt, mb = 46, 14, 14, 40
    pw, ph = width - ml - mr, height - mt - mb

    def px(x):
        return ml + (x + xlim) / (2 * xlim) * pw

    y0 = -ylim if ylow is None else ylow

    def py(y):
        return mt + (1 - (y - y0) / (ylim - y0)) * ph

    b = [f'<rect x="{ml}" y="{mt}" width="{pw}" height="{ph}" fill="{PAPER}" stroke="{GRID}"/>']
    for t in ticks:
        if abs(t) <= xlim + 1e-9:
            b.append(f'<line x1="{px(t):.1f}" y1="{mt}" x2="{px(t):.1f}" y2="{mt + ph}" stroke="{GRID}" stroke-width="{1.4 if t == 0 else 0.6}"/>')
            b.append(_text(px(t), mt + ph + 14, f"{t:g}".replace(".", ","), 9, SOFT, "middle", mono=True))
        if y0 - 1e-9 <= t <= ylim + 1e-9:
            b.append(f'<line x1="{ml}" y1="{py(t):.1f}" x2="{ml + pw}" y2="{py(t):.1f}" stroke="{GRID}" stroke-width="{1.4 if t == 0 else 0.6}"/>')
            b.append(_text(ml - 6, py(t) + 3, f"{t:g}".replace(".", ","), 9, SOFT, "end", mono=True))
    if diagonal and ylow is None:
        d = min(xlim, ylim)
        b.append(f'<line x1="{px(-d):.1f}" y1="{py(-d):.1f}" x2="{px(d):.1f}" y2="{py(d):.1f}" stroke="{SOFT}" stroke-width="0.8" stroke-dasharray="3 3"/>')
    for _label, color, pts in series:
        path = " ".join(f"{'M' if i == 0 else 'L'}{px(x):.1f},{py(max(y0, min(ylim, y))):.1f}" for i, (x, y) in enumerate(pts))
        b.append(f'<path d="{path}" fill="none" stroke="{color}" stroke-width="2" stroke-linejoin="round"/>')
    b.append(_text(ml + pw / 2, height - 6, xlabel, 10, SOFT, "middle", 600))
    b.append(f'<text transform="translate(11 {mt + ph / 2:.1f}) rotate(-90)" font-size="10" fill="{SOFT}" text-anchor="middle" font-weight="600">{escape(ylabel)}</text>')
    if legend:
        for i, (label, color, _pts) in enumerate(series):
            y = mt + 14 + i * 14
            b.append(f'<line x1="{ml + 10}" y1="{y - 3}" x2="{ml + 26}" y2="{y - 3}" stroke="{color}" stroke-width="2.4"/>')
            b.append(_text(ml + 31, y, label, 9.5, INK))
    return _svg(width, height, "".join(b))


def flow(rows: list[list[str]], accent: str, width=640, notes: dict[str, str] | None = None) -> str:
    """Diagrama de flujo: filas de cajas unidas por flechas (cada fila va de izquierda a derecha).

    Una caja cuyo texto empieza por «*» se rellena con el color de acento (la parte no lineal).
    """
    bw, bh, gap, vgap, pad = 0, 40, 26, 30, 12
    maxn = max(len(r) for r in rows)
    bw = (width - 2 * pad - gap * (maxn - 1)) / maxn
    height = pad * 2 + len(rows) * bh + (len(rows) - 1) * vgap
    b = [f'<defs><marker id="ar" viewBox="0 0 8 8" refX="7" refY="4" markerWidth="7" markerHeight="7" orient="auto">'
         f'<path d="M0,0 L8,4 L0,8 z" fill="{SOFT}"/></marker></defs>']
    centres = []
    for r, row in enumerate(rows):
        y = pad + r * (bh + vgap)
        total = len(row) * bw + (len(row) - 1) * gap
        x0 = pad + (width - 2 * pad - total) / 2
        cs = []
        for i, label in enumerate(row):
            x = x0 + i * (bw + gap)
            hot = label.startswith("*")
            label = label.lstrip("*")
            fill = accent if hot else "#F6F1E3"
            tcol = "#FFFFFF" if hot else INK
            b.append(f'<rect x="{x:.1f}" y="{y}" width="{bw:.1f}" height="{bh}" rx="5" fill="{fill}" stroke="{accent}" stroke-width="1.2"/>')
            lines = label.split("\n")
            for j, ln in enumerate(lines):
                ty = y + bh / 2 + 3.5 + (j - (len(lines) - 1) / 2) * 12
                b.append(_text(x + bw / 2, ty, ln, 10, tcol, "middle", 600))
            cs.append((x, y))
            if i > 0:
                b.append(f'<line x1="{x - gap + 2:.1f}" y1="{y + bh / 2}" x2="{x - 2:.1f}" y2="{y + bh / 2}" stroke="{SOFT}" stroke-width="1.2" marker-end="url(#ar)"/>')
        centres.append(cs)
        if r > 0:  # salto de fila: del último de la fila anterior al primero de esta
            ax, ay = centres[r - 1][-1][0] + bw / 2, centres[r - 1][-1][1] + bh
            bx, by = cs[0][0] + bw / 2, cs[0][1]
            my = ay + vgap / 2
            b.append(f'<path d="M{ax:.1f},{ay} L{ax:.1f},{my} L{bx:.1f},{my} L{bx:.1f},{by - 2}" fill="none" stroke="{SOFT}" stroke-width="1.2" marker-end="url(#ar)"/>')
    return _svg(width, int(height), "".join(b))
