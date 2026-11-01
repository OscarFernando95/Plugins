#!/usr/bin/env python3
"""Genera los PDF de documentación de los plugins de FERB.

    python3 docs/construir.py                 # todos los plugins con fuente en docs/fuentes/
    python3 docs/construir.py Tufi Cremita    # solo esos
    python3 docs/construir.py --capturas      # además, vuelve a renderizar las capturas de la interfaz

Entrada : docs/fuentes/<Plugin>.md (Markdown), docs/estilo/ferb-doc.css (identidad), docs/figuras.py
Salida  : docs/<Plugin> — Documentación.pdf
La versión sale de cmake/Versions.cmake. Hace falta Google Chrome (imprime el PDF) y el entorno
.cache/docs-venv con el paquete `markdown` (python3 -m venv .cache/docs-venv && .cache/docs-venv/bin/pip install markdown).
"""
from __future__ import annotations

import datetime
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
CACHE = ROOT / ".cache" / "docs"
CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"

sys.path.insert(0, str(DOCS))
import figuras as fg  # noqa: E402

try:
    import markdown
except ImportError:
    venv = ROOT / ".cache" / "docs-venv" / "bin" / "python"
    if venv.exists() and Path(sys.executable) != venv:
        raise SystemExit(subprocess.call([str(venv), __file__, *sys.argv[1:]]))
    raise SystemExit("Falta el paquete «markdown»: python3 -m venv .cache/docs-venv && .cache/docs-venv/bin/pip install markdown")

MESES = ["enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto", "septiembre", "octubre", "noviembre", "diciembre"]

PLUGINS = {
    "Tufi": dict(var="TUFI", code="Tufi", theme="tufi", tagline="Saturador armónico · Modelo 1", kind="Saturador armónico analógico",
                 formats="VST3 · AU (AUv2, efecto)", channels="Mono y estéreo", cat="Fx|Distortion", bundle="com.ferb.tufi", snapshot="Tufi/TufiSnapshot"),
    "Poty": dict(var="POTY", code="Poty", theme="poty", tagline="Enhancer multibanda", kind="Enhancer multibanda de saturación en paralelo",
                 formats="VST3 · AU (AUv2, efecto)", channels="Mono y estéreo (modos Estéreo, Mid y Side)", cat="Fx|Distortion", bundle="com.ferb.poty", snapshot="Poty/PotySnapshot"),
    "Nel": dict(var="NEL", code="Nel1", theme="nel", tagline="Limitador de true peak", kind="Limitador de true peak con medición de sonoridad",
                formats="VST3 · AU (AUv2, efecto)", channels="Mono y estéreo", cat="Fx|Dynamics", bundle="com.ferb.nel", snapshot="Nel/NelSnapshot"),
    "Maru": dict(var="MARU", code="Maru", theme="maru", tagline="Supresor de ruido · Gate", kind="Supresor de ruido y gate de un solo control",
                 formats="VST3 · AU (AUv2, efecto)", channels="Mono y estéreo (enlace estéreo), sidechain externo opcional", cat="Fx|Restoration", bundle="com.ferb.maru", snapshot="Maru/MaruSnapshot"),
    "ChauS": dict(var="CHAUS", code="ChaS", theme="chaus", tagline="De-esser · EQ dinámico", kind="De-esser con EQ dinámico que sigue la sibilante",
                  formats="VST3 · AU (AUv2, efecto)", channels="Mono y estéreo (opciones Mid y Side), sidechain externo opcional", cat="Fx|Dynamics", bundle="com.ferb.chaus", snapshot="ChauS/ChausSnapshot"),
    "DraMcPoti": dict(var="DRAMCPOTI", code="McPo", theme="dramcpoti", display="Dra. McPoti", tagline="Excitador armónico multibanda", kind="Excitador armónico multibanda y dinámico (Vocal y Master, Vintage y Moderno)",
                      formats="VST3 · AU (AUv2, efecto)", channels="Mono y estéreo (Ancho y Mono seguro solo en estéreo)", cat="Fx|Distortion", bundle="com.ferb.dramcpoti", snapshot="DraMcPoti/DraMcPotiSnapshot",
                      estado="Edición 1.0.0 · verificado con pruebas automáticas y validadores · revisión inicial del autor correcta · sin pruebas sistemáticas en anfitriones ni validación del AAX"),
    "Cremita": dict(var="CREMITA", code="Crem", theme="cremita", tagline="Inflator · sonoridad y calidez", kind="Inflator (sonoridad y calidez sin comprimir)",
                    formats="VST3 · AU (AUv2, efecto)", channels="Mono y estéreo (opción Mid/Side)", cat="Fx|Distortion", bundle="com.ferb.cremita", snapshot="Cremita/CremitaSnapshot"),
}
# Estado que se declara en la portada y en el control del documento.
ESTADO = "Edición 1.0.0 · verificado con pruebas automáticas y validadores · sin prueba manual en anfitriones"


# ───────────────────────────────────────────── versiones, curvas y capturas
def version_of(var: str) -> str:
    text = (ROOT / "cmake" / "Versions.cmake").read_text()
    m = re.search(rf'set\({var}_VERSION\s+"([^"]+)"\)', text)
    if not m:
        raise SystemExit(f"No encuentro {var}_VERSION en cmake/Versions.cmake")
    return m.group(1)


def check_readme() -> None:
    """Avisa si la versión de la tabla del README no coincide con cmake/Versions.cmake."""
    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    text = (ROOT / "cmake" / "Versions.cmake").read_text()
    for var, ver in re.findall(r'set\((\w+)_VERSION\s+"([^"]+)"\)', text):
        shown = next((m.get("display", n) for n, m in PLUGINS.items() if m["var"] == var), var)  # el README usa el nombre que se muestra
        m = re.search(rf"\*\*{re.escape(shown)}\*\*\s*\|\s*([\d.]+)", readme, re.I)
        if not m:
            print(f"AVISO: {var.title()} no está en la tabla del README")
        elif m.group(1) != ver:
            print(f"AVISO: README dice {var.title()} {m.group(1)}, pero cmake/Versions.cmake dice {ver}")


def dump_curves() -> dict:
    exe = CACHE / "curvas"
    CACHE.mkdir(parents=True, exist_ok=True)
    srcs = [DOCS / "herramientas" / "curvas.cpp", ROOT / "Tufi/source/dsp/StageCurves.cpp", ROOT / "core/Saturators.cpp"]
    subprocess.check_call(["clang++", "-std=c++20", "-O1", "-I", str(ROOT / "core"), "-I", str(ROOT / "Tufi/source"),
                           "-I", str(ROOT / "Cremita/source"), "-I", str(ROOT / "Nel/source"), *map(str, srcs), "-o", str(exe)])
    return fg.parse_curves(subprocess.check_output([str(exe)], text=True))


def render_screenshots(names: list[str]) -> None:
    for name in names:
        exe = ROOT / "build" / PLUGINS[name]["snapshot"]
        if not exe.exists():
            raise SystemExit(f"Falta {exe}: compila antes (cmake --build build --target {name}Snapshot)")
        out = str(DOCS / "imagenes" / f"{name.lower()}.png")
        if name == "DraMcPoti":  # preset de voz, con unos segundos de audio de prueba para los vúmetros y la ventana de armónicos
            subprocess.check_call([str(exe), out, "2", "Radio setentas"], env={**os.environ, "FERB_SNAPSHOT_BLOCKS": "400"})
            continue
        if name == "Maru":  # voz sintética con pausas y ruido de fondo
            subprocess.check_call([str(exe), out, "2", "Voz de podcast"], env={**os.environ, "FERB_SNAPSHOT_BLOCKS": "500"})
        elif name == "ChauS":  # voz sintética con eses: la captura cae en medio de una «s», con un preset de voz
            subprocess.check_call([str(exe), out, "2", "Voz masculina"], env={**os.environ, "FERB_SNAPSHOT_BLOCKS": "232"})
        else:
            subprocess.check_call([str(exe), out, "2"])


# ───────────────────────────────────────────── figuras
def figure(svg: str, caption: str, cls: str = "") -> str:
    return f'<figure class="{cls}">{svg}<figcaption>{caption}</figcaption></figure>'


def figures(name: str, curves: dict) -> dict[str, str]:
    c = curves
    out: dict[str, str] = {}
    if name == "Tufi":
        out["curvas"] = figure(
            fg.plot([("Triodo", "#2A1D14", c["tufi_triodo"]), ("Pentodo", "#B08D4F", c["tufi_pentodo"]), ("Diodo", "#A8321D", c["tufi_diodo"])],
                    2, 1.2, "entrada (tras Entrada)", "salida", ticks=(-2, -1, 0, 1, 2, -1.2, 1.2)),
            "Curva de transferencia de cada etapa con los ajustes de fábrica (Nivel bajo 3,5 · Picos 5,5 · Dureza 4,0 · Pares 3,0 y 4,5). "
            "Línea discontinua: recta de referencia (transparente). Calculada con el código del motor.")
        out["flujo"] = figure(fg.flow([
            ["Entrada\n(dB, suavizada)", "Sobremuestreo\n1× a 8× (FIR)", "*Curva de\ntransferencia (LUT)", "*ADAA de\nprimer orden"],
            ["Submuestreo", "Filtro de DC\n10 Hz", "Salida\n(dB, suavizada)", "Medidor y VU"]],
            "#B08D4F"), "Cadena de señal de Tufi, por canal y sin enlace estéreo. Las cajas rellenas son la parte no lineal.")
    elif name == "Poty":
        out["satur"] = figure(
            fg.plot([("Tubo", "#FF6B8A", c["poty_tubo"]), ("Cinta", "#E2A13A", c["poty_cinta"]), ("Sólido", "#39A85B", c["poty_solido"]),
                     ("Diodo Ge", "#12708C", c["poty_germanio"]), ("Digital", "#8A63E0", c["poty_digital"])],
                    3, 1.5, "v = d·Δ (entrada del saturador)", "f(v)", ticks=(-3, -2, -1, 0, 1, 2, 3, -1.5, 1.5), diagonal=True, width=420, height=330),
            "Los cinco tipos de saturación, f(v) con f′(0) = 1. Calculadas con el código del motor. La cinta se muestra solo con su tanh: "
            "los filtros de pre y de-énfasis son aparte.")
        out["flujo"] = figure(fg.flow([
            ["Entrada\n(ganancia)", "Mid/Side\n(según el modo)", "Realce por\nbanda (biquad ×5)", "Δᵢ = realceᵢ(x) − x"],
            ["*Sobremuestreo\n2× a 8×", "*Saturador\ngᵢ = f(d·Δᵢ)/d + ADAA", "Σ bandas\n× Mezcla", "Suma con el original\n(retrasado 47 muestras)"],
            ["Salida\n(ganancia)", "Analizador\n(entrada y salida)"]],
            "#12708C", width=640), "Flujo de señal de Poty. El realce de cada banda se satura por separado y solo lo añadido se suma a la señal original.")
    elif name == "Nel":
        out["knee"] = figure(
            fg.plot([("Knee 0 dB", "#2A1D14", c["nel_knee_0"]), ("Knee 3 dB", "#8A5CF0", c["nel_knee_3"]),
                     ("Knee 6 dB", "#C9A24D", c["nel_knee_6"]), ("Knee 10 dB", "#A8321D", c["nel_knee_10"])],
                    12, 12, "nivel del pico sobre el umbral (dB)", "reducción de ganancia (dB)", ylow=0,
                    ticks=(-12, -6, 0, 6, 12), legend=True),
            "Codo suave de la etapa de cuerpo (B) con distintos valores de Knee. Calculada con el código del motor: relación infinita; "
            "con knee 0 es una esquina, y con valores mayores la reducción empieza antes y de forma gradual.")
        out["flujo"] = figure(fg.flow([
            ["Entrada\n+ Ganancia", "Filtro de DC\n(opcional)", "Sobremuestreo\n1× a 32×", "*Carácter y\nrecorte previo (ADAA)"],
            ["*Etapa B: cuerpo\n(frec. base)", "*Etapa A: picos\ncon lookahead", "*Seguro de pico\n(margen adaptativo)", "Submuestreo"],
            ["Dither\n(opcional)", "Ganancia unitaria\ny Audición", "Salida", "Medidores y\nsonoridad"]],
            "#8A5CF0"), "Flujo de señal de Nel. La etapa A es la que garantiza el techo; la B hace el trabajo lento para que A solo recorte lo que sobra. Las cajas rellenas son las que modifican la señal.")
    elif name == "Cremita":
        out["curvas"] = figure(
            fg.plot([("Curva −50", "#6B5B4B", c["crem_m50"]), ("Curva 0", "#E08A2E", c["crem_0"]), ("Curva +50", "#A8321D", c["crem_p50"])],
                    1, 1, "entrada (tras Input)", "salida", ticks=(-1, -0.5, 0, 0.5, 1)),
            "Curva de transferencia de Cremita con Sesgo 0 %. Con Curva 0 la pendiente en cero es +1,0 dB; con +50, +3,5 dB; con −50, −2,5 dB. Línea discontinua: recta de referencia.")
        out["sesgo"] = figure(
            fg.plot([("Sesgo 0 %", "#E08A2E", c["crem_0"]), ("Sesgo 100 %", "#4A2D1A", c["crem_0_sesgo"])],
                    1, 1, "entrada (tras Input)", "salida", ticks=(-1, -0.5, 0, 0.5, 1)),
            "Efecto del Sesgo con Curva 0: la curva se vuelve asimétrica y aparecen armónicos pares (calidez).")
        out["flujo"] = figure(fg.flow([
            ["Input\n(+ Mid/Side)", "Bandas LR4\n(opcional, 3)", "*Sobremuestreo\n+ curva + ADAA", "Mezcla\nEfecto"],
            ["Filtro DC\n(solo el aporte)", "Igualar\nsonoridad", "Output", "Clip 0 dB\n(al final)"]],
            "#E08A2E"), "Cadena de señal de Cremita. No hay detector ni envolvente: el procesado es estático.")
    elif name == "Maru":
        out["flujo"] = figure(fg.flow([
            ["Entrada\n(mono o estéreo)", "STFT √Hann\n(N = 1024, salto N/4)", "*Estimador de piso\n(MCRA, sobre la suma mid)", "*Ganancia por bin\n(LSA + decision-directed)"],
            ["Suavizado en\nfrecuencia y tiempo", "STFT inversa\ny solape-suma", "Escuchar lo quitado\ny Salida", "Bypass del\nanfitrión"],
            ["Gate: detector HP/LP\n+ umbral = piso + margen", "Gate: ganancia\ncon lookahead"]],
            "#62786B"), "Flujo de señal de Maru. El estimador de piso de ruido es común a los dos motores; la Supresión actúa por bin en la STFT y el Gate actúa en el tiempo, con la señal retrasada su lookahead. Las cajas rellenas son las que deciden la reducción.")
    elif name == "ChauS":
        out["flujo"] = figure(fg.flow([
            ["Entrada\n(+ Mid/Side)", "Detector: nivel de\nbanda + 6 rasgos", "*Reducción\n(umbral adaptativo)", "Balística\n(ataque 0,5 ms)"],
            ["Retardo de\nlookahead", "*Campana o estante\n(frecuencia y Q seguidas)", "Mid/Side\ninverso", "Salida y\nbypass"],
            ["Pantalla\n(anillos y telemetría)"]],
            "#FF6B5E"), "Flujo de señal de ChauS, por canal. El detector mira la señal sin retardo; el audio se retrasa el lookahead, así la reducción llega antes que la «s». Las cajas rellenas son las que deciden y aplican la reducción.")
    return out


# ───────────────────────────────────────────── ensamblado del HTML
def number_headings(html: str) -> tuple[str, list[tuple[int, str, str, str]]]:
    """Numera h1 (1, 2, …) y h2 (1.1, …); «{.ap}» en un h1 = apéndice sin número. Devuelve el HTML y la lista del índice."""
    toc: list[tuple[int, str, str, str]] = []
    n1 = n2 = 0

    def repl(m: re.Match) -> str:
        nonlocal n1, n2
        level, attrs, inner = int(m.group(1)), m.group(2) or "", m.group(3)
        hid = re.search(r'id="([^"]+)"', attrs).group(1)
        text = re.sub(r"<[^>]+>", "", inner)
        if level == 1:
            if 'class="ap"' in attrs:
                toc.append((1, "", hid, text))
                return f'<h1{attrs}><span class="num">Apéndice</span>{inner}</h1>'
            n1, n2 = n1 + 1, 0
            toc.append((1, f"{n1:02d}", hid, text))
            return f'<h1{attrs}><span class="num">Sección {n1:02d}</span>{inner}</h1>'
        n2 += 1
        toc.append((2, f"{n1}.{n2}", hid, text))
        return f'<h2{attrs}><span class="num">{n1}.{n2}</span>{inner}</h2>'

    return re.sub(r"<h([12])((?: [^>]*)?)>(.*?)</h\1>", repl, html, flags=re.S), toc


def build(name: str, curves: dict, fecha: str) -> Path:
    meta = PLUGINS[name]
    shown = meta.get("display", name)  # nombre que se muestra (el de la clave no lleva punto ni espacio)
    version = version_of(meta["var"])
    src = (DOCS / "fuentes" / f"{name}.md").read_text(encoding="utf-8")

    figs = figures(name, curves)
    uid = 0
    for key, svg in figs.items():
        uid += 1
        figs[key] = svg.replace('id="ar"', f'id="ar{uid}"').replace("url(#ar)", f"url(#ar{uid})")
    src = re.sub(r"\{\{fig:(\w+)\}\}", lambda m: figs[m.group(1)], src)
    src = src.replace("{{version}}", version).replace("{{fecha}}", fecha)
    src = src.replace("{{img}}", f"file://{DOCS}/imagenes")

    md = markdown.Markdown(extensions=["tables", "fenced_code", "attr_list", "md_in_html", "sane_lists", "toc"])
    body, toc = number_headings(md.convert(src))

    toc_html = "".join(f'<li class="l{lvl}"><span class="n">{num}</span><a href="#{hid}">{text}</a></li>' for lvl, num, hid, text in toc)
    doc_id = f"FERB-{name.upper()}-DOC"
    shot = f"file://{DOCS}/imagenes/{name.lower()}.png"
    bars = '<div class="bars">' + "".join(f'<i style="background:{c}"></i>' for c in ("#FF6B8A", "#FFB454", "#7EE08A", "#4CC9F0", "#B18CFF")) + "</div>" if name == "Poty" else ""

    cover = f"""
<section class="cover"><div class="rail l"></div><div class="rail r"></div><div class="frame"></div>
 <div class="inner">
  <div class="brandline">By FERB · Plugins de audio</div>
  <div class="wordmark">{shown}</div>{bars}
  <div class="tagline">{meta['tagline']}</div>
  <div class="shot"><img src="{shot}"></div>
  <div class="docmeta">
    <div class="what">Documentación del producto<small>Guía de uso · referencia técnica · verificación<br>{fecha}</small></div>
    <div class="ver">v{version}<small>{doc_id}</small></div>
  </div>
 </div>
</section>"""
    front = f"""
<section class="front">
  <h2 class="plain">Control del documento</h2>
  <table class="ctl">
   <tr><td>Documento</td><td>{doc_id} · Documentación del producto</td></tr>
   <tr><td>Producto</td><td>{shown} — {meta['kind']}</td></tr>
   <tr><td>Versión</td><td><b>{version}</b> (SemVer; fuente: <code>cmake/Versions.cmake</code>)</td></tr>
   <tr><td>Fecha</td><td>{fecha}</td></tr>
   <tr><td>Estado</td><td>{meta.get('estado', ESTADO)}</td></tr>
   <tr><td>Formatos</td><td>{meta['formats']} · macOS 11 o superior, binario universal (arm64 y x86_64)</td></tr>
   <tr><td>Canales</td><td>{meta['channels']}</td></tr>
   <tr><td>Identificadores</td><td>Código de plugin <code>{meta['code']}</code> · fabricante <code>Ferb</code> · paquete <code>{meta['bundle']}</code> · AU <code>aufx {meta['code']} Ferb</code></td></tr>
   <tr><td>Autor</td><td>Oscar Fernando · By FERB</td></tr>
   <tr><td>Uso</td><td>Personal. Sin distribución comercial (ver el apéndice de avisos)</td></tr>
  </table>
  <div class="toc-title">Contenido</div>
  <ul class="toc">{toc_html}</ul>
</section>"""

    css = (DOCS / "estilo" / "ferb-doc.css").read_text(encoding="utf-8").replace("@@ROOT@@", str(ROOT))
    css = css.replace("@@RUNNING@@", f"{shown} · Documentación {version}")
    html = f'<!DOCTYPE html><html lang="es"><head><meta charset="utf-8"><title>{shown} — Documentación {version}</title><style>{css}</style></head><body class="{meta["theme"]}">{cover}{front}{body}</body></html>'

    CACHE.mkdir(parents=True, exist_ok=True)
    page = CACHE / f"{name}.html"
    page.write_text(html, encoding="utf-8")
    out = DOCS / f"{shown} — Documentación.pdf"
    subprocess.check_call([CHROME, "--headless=new", "--disable-gpu", "--no-pdf-header-footer", "--generate-pdf-document-outline",
                           "--virtual-time-budget=8000", f"--print-to-pdf={out}", f"file://{page}"],
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return out


def main() -> None:
    args = sys.argv[1:]
    shots = "--capturas" in args
    names = [a for a in args if not a.startswith("--")] or [n for n in PLUGINS if (DOCS / "fuentes" / f"{n}.md").exists()]
    check_readme()
    if shots:
        render_screenshots(names)
    d = datetime.date.today()
    fecha = f"{d.day} de {MESES[d.month - 1]} de {d.year}"
    curves = dump_curves()
    for n in names:
        print("→", build(n, curves, fecha))


if __name__ == "__main__":
    main()
