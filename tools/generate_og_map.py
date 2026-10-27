#!/usr/bin/env python3
"""Social-card image of the LQ8 live map: docs/img/og_map.png (1200x630).

Draws the world exactly as docs/live/lqlive.js does at zoom 1 in its default theme (BRIGHT_FUTURE): equirectangular,
the full map fills the height, the sides are cropped; land/coast/borders from docs/live/world.js, the Maidenhead field
grid (20 x 10 degrees) on top. Only a tiny "lq8.org" corner mark. Needs matplotlib + Pillow.
Usage: python3 tools/generate_og_map.py"""
import json, os, re
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.path import Path
from matplotlib.patches import PathPatch
from PIL import Image

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H = 1200, 630
OCEAN, LAND, COAST = "#1E477A", "#3B82B6", "#CBD5E1"
BORDER, GRID = (226/255, 232/255, 240/255, 0.7), (1, 1, 1, 0.22)
S = H / 180.0          # pixels per degree (the page: max(H/180, W/360))
PX = 72.0 / 100.0      # matplotlib points per pixel at dpi=100

def load():
    s = open(os.path.join(BASE, "docs", "live", "world.js"), encoding="utf-8").read()
    return json.loads(s[s.index("window.LQ8_MAP =") + 16:].strip().rstrip(";"))

def parse(d, q):
    """SVG path data (M, m, l, z only) -> matplotlib verts/codes in pixel coordinates (y down)."""
    verts, codes = [], []
    toks = re.findall(r"[MmLlZz]|-?\d*\.?\d+", d)
    i, cmd, x, y, sx, sy, first = 0, None, 0.0, 0.0, 0.0, 0.0, True
    def pt(px, py):
        return ((px / q - 180) * S + W / 2, py / q * S)   # lon = x/q-180 ; y = (90-lat)*S
    while i < len(toks):
        t = toks[i]
        if t in "MmLlZz":
            cmd = t; i += 1
            if t in "Zz":
                x, y = sx, sy
                verts.append(pt(x, y)); codes.append(Path.CLOSEPOLY); cmd = None
            continue
        a, b = float(toks[i]), float(toks[i + 1]); i += 2
        rel = cmd in "ml"
        x, y = (x + a, y + b) if rel else (a, b)
        if cmd in "Mm":
            verts.append(pt(x, y)); codes.append(Path.MOVETO); sx, sy = x, y
            cmd = "l" if cmd == "m" else "L"
        else:
            verts.append(pt(x, y)); codes.append(Path.LINETO)
    return verts, codes

def main():
    m = load(); q = m.get("q", 1)
    fig = plt.figure(figsize=(W / 100, H / 100), dpi=100)
    ax = fig.add_axes([0, 0, 1, 1]); ax.set_xlim(0, W); ax.set_ylim(H, 0); ax.axis("off")
    ax.add_patch(plt.Rectangle((0, 0), W, H, fc=OCEAN, ec="none"))
    def add(key, **kw):
        if not m.get(key): return
        v, c = parse(m[key], q)
        ax.add_patch(PathPatch(Path(v, c), joinstyle="round", **kw))
    add("fill", fc=LAND, ec="none")
    add("land", fc=LAND, ec=COAST, lw=1.35 * PX)
    add("coast", fc="none", ec=COAST, lw=1.35 * PX)
    add("borders", fc="none", ec=BORDER, lw=1.0 * PX)
    for lon in range(-180, 181, 20):
        x = lon * S + W / 2; ax.plot([x, x], [0, H], color=GRID, lw=0.6 * PX)
    for lat in range(-90, 91, 10):
        y = (90 - lat) * S; ax.plot([0, W], [y, y], color=GRID, lw=0.6 * PX)
    ax.text(W - 14, H - 12, "lq8.org", ha="right", va="bottom", color="#F8FAFC", fontsize=11, alpha=0.85,
            fontweight="bold", path_effects=[__import__("matplotlib.patheffects", fromlist=["x"]).withStroke(linewidth=2.5, foreground="#0F172A")])
    out = os.path.join(BASE, "docs", "img", "og_map.png")
    tmp = out + ".tmp.png"
    fig.savefig(tmp, dpi=100)
    Image.open(tmp).convert("RGB").quantize(colors=128, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE).save(out, optimize=True)
    os.remove(tmp)
    print(out, os.path.getsize(out), "bytes")

if __name__ == "__main__":
    main()
