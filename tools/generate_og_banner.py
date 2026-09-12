#!/usr/bin/env python3
# =============================================================================
# The LQ Digital Mode Family — Reference Implementation (lq_lib)
#
# Author:  Luis Quesada (HB9IPH)
# Web:     https://luisquesada.com
# Portal:  https://lquesada.github.io/lq_lib/
# GitHub:  https://github.com/lquesada/lq_lib
# App:     qFT8 — Portable Amateur Radio for Android (https://qft8.com)
#
# License: MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)
#
# Copyright (c) 2026 Luis Quesada (HB9IPH)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# =============================================================================

"""
generate_og_banner.py

Generates the 1200x630 Open Graph (OG) social card banner (docs/img/og_banner.png)
featuring the website hero pitch with crisp typography, glowing cyan branding,
and optimal contrast for link previews in Telegram, Twitter, Discord, and Facebook.
"""

import os
import subprocess
import sys
import tempfile
from PIL import Image

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
OUTPUT_IMAGE = os.path.join(BASE_DIR, "docs", "img", "og_banner.png")

HTML_TEMPLATE = """<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=Outfit:wght@700;800;900&display=swap" rel="stylesheet">
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    html, body {
      width: 1200px;
      height: 630px;
      overflow: hidden;
      background: #07090e;
      color: #f8fafc;
      font-family: 'Inter', system-ui, -apple-system, sans-serif;
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      text-align: center;
      padding: 36px 70px;
      -webkit-font-smoothing: antialiased;
      -moz-osx-font-smoothing: grayscale;
    }
    .hero-mode-title {
      font-family: 'Outfit', 'Inter', sans-serif;
      font-size: 29px;
      font-weight: 900;
      letter-spacing: 0.02em;
      text-transform: uppercase;
      background: linear-gradient(135deg, #ffffff 10%, #bae6fd 60%, #38bdf8 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      filter: drop-shadow(0 2px 14px rgba(56, 189, 248, 0.45));
      margin-bottom: 22px;
    }
    h1 {
      font-family: 'Outfit', 'Inter', sans-serif;
      font-size: 49px;
      font-weight: 800;
      line-height: 1.18;
      letter-spacing: -0.03em;
      color: #f1f5f9;
      margin-bottom: 24px;
    }
    p {
      font-family: 'Inter', system-ui, -apple-system, sans-serif;
      font-size: 19.5px;
      line-height: 1.65;
      color: #94a3b8;
      max-width: 960px;
    }
    strong {
      color: #f8fafc;
      font-weight: 600;
    }
  </style>
</head>
<body>
  <div class="hero-mode-title">The LQ8 Digital Mode</div>
  <h1>
    Full 4-Step Contacts in 60 Seconds<br>
    4× Pileup Speed with Single-Carrier (240 vs 60 QSOs/h)<br>
    0.0 dB Power Penalty
  </h1>
  <p>
    <strong>LQ8</strong> is a next-generation amateur radio weak-signal protocol that re-engineers message packing using variable-length prefix codes over the exact, battle-tested <strong>FT8 physical transport layer</strong> (8-GFSK, 50&nbsp;Hz, &minus;21.0&nbsp;dB SNR). Complete full contacts with bidirectional grid, report, and 73 confirmation in <strong>4 transmissions (60s vs 75–90s)</strong> and achieve <strong>240 QSOs/h in pileups (4× over single-carrier FT8, 2 QSOs every 30s)</strong> with 0.0 dB RF power penalty (scaling to <strong>480 QSOs/h</strong> dual-carrier).
  </p>
</body>
</html>
"""

def generate_banner():
    os.makedirs(os.path.dirname(OUTPUT_IMAGE), exist_ok=True)
    with tempfile.NamedTemporaryFile(mode="w", suffix=".html", delete=False, encoding="utf-8") as f:
        f.write(HTML_TEMPLATE)
        temp_html = f.name

    try:
        cmd = [
            "google-chrome",
            "--headless",
            "--disable-gpu",
            "--window-size=1200,630",
            "--hide-scrollbars",
            "--virtual-time-budget=5000",
            f"--screenshot={OUTPUT_IMAGE}",
            f"file://{temp_html}"
        ]
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            print(f"Error running Chrome: {res.stderr}", file=sys.stderr)
            sys.exit(res.returncode)

        if not os.path.exists(OUTPUT_IMAGE):
            print(f"Error: Output image {OUTPUT_IMAGE} was not generated.", file=sys.stderr)
            sys.exit(1)

        with Image.open(OUTPUT_IMAGE) as img:
            print(f"Successfully generated {OUTPUT_IMAGE} with size {img.size} and mode {img.mode}")
            assert img.size == (1200, 630), f"Unexpected size: {img.size}"

    finally:
        if os.path.exists(temp_html):
            os.remove(temp_html)

if __name__ == "__main__":
    generate_banner()
