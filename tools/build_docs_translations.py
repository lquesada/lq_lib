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
build_docs_translations.py

Expands the docs/ web portal to pre-rendered static HTML files for all supported
languages (e.g. docs/es/index.html, docs/de/index.html, docs/fr/index.html),
complete with localized metadata, SEO canonical/hreflang tags, a streamlined
single-line top navigation bar with a native language dropdown selector,
bulletproof inter-language switching across all pages, auto-redirection on root
index.html, and a multi-language sitemap.xml.
"""

import json
import os
import re
import subprocess
import sys

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DOCS_DIR = os.path.join(BASE_DIR, "docs")
LANG_DIR = os.path.join(DOCS_DIR, "languages")
BASE_URL = "https://lq8.org"

# Ordered list of supported languages
SUPPORTED_LANGUAGES = [
    'en', 'es', 'pt', 'fr', 'it', 'de', 'ru', 'zh', 'ja', 'ko',
    'ar', 'hi', 'bn', 'ur', 'tr', 'vi', 'id', 'nl', 'pl', 'sv',
    'fi', 'da', 'no', 'cs', 'sk', 'hu', 'ro', 'el', 'he', 'th',
    'ca', 'eu', 'gl', 'eo', 'fa', 'uk', 'sw', 'ha', 'yo', 'am',
    'az', 'uz', 'ta', 'te', 'mr', 'gu', 'kn', 'ml', 'pa', 'or',
    'my', 'tl', 'jv', 'su', 'bho', 'pcm', 'wuu', 'yue',
    'apc', 'apd', 'arq', 'ary', 'arz'
]

LANGUAGE_NAMES = {
    'en': 'English',
    'es': 'Español',
    'pt': 'Português',
    'fr': 'Français',
    'it': 'Italiano',
    'de': 'Deutsch',
    'ru': 'Русский',
    'zh': '中文',
    'ja': '日本語',
    'ko': '한국어',
    'ar': 'العربية',
    'hi': 'हिन्दी',
    'bn': 'বাংলা',
    'ur': 'اردو',
    'tr': 'Türkçe',
    'vi': 'Tiếng Việt',
    'id': 'Bahasa Indonesia',
    'nl': 'Nederlands',
    'pl': 'Polski',
    'sv': 'Svenska',
    'fi': 'Suomi',
    'da': 'Dansk',
    'no': 'Norsk',
    'cs': 'Čeština',
    'sk': 'Slovenčina',
    'hu': 'Magyar',
    'ro': 'Română',
    'el': 'Ελληνικά',
    'he': 'עברית',
    'th': 'ไทย',
    'ca': 'Català',
    'eu': 'Euskara',
    'gl': 'Galego',
    'eo': 'Esperanto',
    'fa': 'فارسی',
    'uk': 'Українська',
    'sw': 'Kiswahili',
    'ha': 'Hausa',
    'yo': 'Yorùbá',
    'am': 'አማርኛ',
    'az': 'Azərbaycan',
    'uz': 'Oʻzbekcha',
    'ta': 'தமிழ்',
    'te': 'తెలుగు',
    'mr': 'मराठी',
    'gu': 'ગુજરાતી',
    'kn': 'ಕನ್ನಡ',
    'ml': 'മലയാളം',
    'pa': 'ਪੰਜਾਬੀ',
    'or': 'ଓଡ଼ିଆ',
    'my': 'မြန်မာဘာသာ',
    'tl': 'Tagalog',
    'jv': 'Basa Jawa',
    'su': 'Basa Sunda',
    'bho': 'भोजपुरी',
    'pcm': 'Naijá',
    'wuu': '吴语',
    'yue': '粵語',
    'apc': 'شامي (Levantine)',
    'apd': 'سوداني (Sudanese)',
    'arq': 'دزيرية (Algerian)',
    'ary': 'دارجة (Moroccan)',
    'arz': 'مصري (Egyptian)'
}

RTL_LANGUAGES = {'ar', 'he', 'fa', 'ur', 'apc', 'apd', 'arq', 'ary', 'arz'}


def load_translations(lang):
    """Loads translations dictionary for a given language code."""
    file_path = os.path.join(LANG_DIR, f"lang{lang}.js")
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"Translation file not found: {file_path}")

    # Use Node.js to evaluate the JS object reliably
    cmd = [
        "node", "-e",
        f"""
        const fs = require('fs');
        const code = fs.readFileSync({json.dumps(file_path)}, 'utf8');
        const data = new Function('const window={{}}; ' + code.replace(/const langData =/, 'return'))();
        console.log(JSON.stringify(data));
        """
    ]
    res = subprocess.run(cmd, capture_output=True, text=True, check=True)
    return json.loads(res.stdout)


def build_hreflang_tags(current_lang):
    """Constructs SEO canonical and hreflang alternate link tags."""
    if current_lang == 'en':
        canonical_url = f"{BASE_URL}/"
    else:
        canonical_url = f"{BASE_URL}/{current_lang}/"

    lines = [
        "  <!-- SEO Canonical & Hreflang Tags -->",
        f'  <link rel="canonical" href="{canonical_url}">',
        f'  <link rel="alternate" hreflang="x-default" href="{BASE_URL}/">',
        f'  <link rel="alternate" hreflang="en" href="{BASE_URL}/">'
    ]

    for lang in SUPPORTED_LANGUAGES:
        if lang == 'en':
            continue
        lines.append(f'  <link rel="alternate" hreflang="{lang}" href="{BASE_URL}/{lang}/">')

    lines.append("  <!-- End SEO Canonical & Hreflang Tags -->")
    return "\n".join(lines)


def build_lang_dropdown_html(current_lang):
    """Generates the streamlined HTML markup for the language selector dropdown."""
    lines = [
        '        <div class="lang-selector-wrapper">',
        '          <span class="lang-icon" aria-hidden="true">🌐</span>',
        '          <select id="lang-select" class="lang-dropdown" onchange="onLanguageChange(this.value)" aria-label="Select Language">'
    ]
    for lang in SUPPORTED_LANGUAGES:
        name = LANGUAGE_NAMES.get(lang, lang)
        selected_attr = " selected" if lang == current_lang else ""
        lines.append(f'            <option value="{lang}"{selected_attr}>{name}</option>')
    lines.append('          </select>')
    lines.append('        </div>')
    return "\n".join(lines)


def build_script_head(current_lang):
    """Generates the language switcher and auto-redirect JavaScript block."""
    langs_js = json.dumps(SUPPORTED_LANGUAGES)

    redirect_logic = ""
    if current_lang == 'en':
        redirect_logic = """
        // Auto-redirect block for root English page
        (function() {
          try {
            const urlParams = new URLSearchParams(window.location.search);
            const queryLang = urlParams.get('lang');
            const isFile = window.location.protocol === 'file:';

            if (queryLang && supportedLanguages.includes(queryLang)) {
              try { localStorage.setItem('lq_user_lang', queryLang); } catch (e) {}
              if (queryLang !== 'en') {
                const target = isFile ? queryLang + '/index.html' : queryLang + '/';
                window.location.replace(target);
                return;
              }
            }

            if (isFile || sessionStorage.getItem('lq_redirected')) return;

            const savedLang = localStorage.getItem('lq_user_lang');
            if (savedLang && savedLang !== 'en' && supportedLanguages.includes(savedLang)) {
              sessionStorage.setItem('lq_redirected', '1');
              window.location.replace(savedLang + '/');
              return;
            }

            if (!savedLang) {
              const browserLang = (navigator.language || navigator.userLanguage || '').split('-')[0].toLowerCase();
              if (browserLang && browserLang !== 'en' && supportedLanguages.includes(browserLang)) {
                sessionStorage.setItem('lq_redirected', '1');
                window.location.replace(browserLang + '/');
                return;
              }
            }
          } catch (e) {}
        })();
        """

    change_logic = """
        function onLanguageChange(lang) {
          if (!supportedLanguages.includes(lang)) return;
          try { localStorage.setItem('lq_user_lang', lang); } catch (e) {}
          const isFile = window.location.protocol === 'file:';

          // Determine relative root prefix universally from current URL pathname
          let rootPrefix = './';
          const pathParts = window.location.pathname.split('/').filter(Boolean);
          const lastPart = pathParts[pathParts.length - 1] || '';
          const secondLastPart = pathParts[pathParts.length - 2] || '';
          if (supportedLanguages.includes(lastPart) || (lastPart === 'index.html' && supportedLanguages.includes(secondLastPart))) {
            rootPrefix = '../';
          }

          if (lang === 'en') {
            window.location.href = isFile ? rootPrefix + 'index.html' : rootPrefix;
          } else {
            window.location.href = isFile ? rootPrefix + lang + '/index.html' : rootPrefix + lang + '/';
          }
        }
    """

    return f"""  <!-- Language Switcher & Redirection -->
  <script>
    const supportedLanguages = {langs_js};
{change_logic}{redirect_logic}  </script>
  <!-- End Language Switcher & Redirection -->"""


def find_i18n_elements(html):
    """Scans HTML for elements marked with data-i18n, identifying their exact boundaries."""
    comment_ranges = [(m.start(), m.end()) for m in re.finditer(r'<!--[\s\S]*?-->', html)]
    pattern = re.compile(r'<([a-zA-Z0-9]+)\b([^>]*\bdata-i18n=\"([^\"]+)\"[^>]*)>', re.IGNORECASE)
    elements = []

    for m in pattern.finditer(html):
        open_tag_start = m.start()
        open_tag_end = m.end()

        # Skip elements that are inside HTML comments
        if any(c_start <= open_tag_start and open_tag_end <= c_end for c_start, c_end in comment_ranges):
            continue

        tag = m.group(1).lower()
        attrs = m.group(2)
        key = m.group(3)

        if tag == 'img':
            elements.append({
                'type': 'img',
                'tag': tag,
                'key': key,
                'open_start': open_tag_start,
                'open_end': open_tag_end,
                'attrs': attrs
            })
            continue

        depth = 1
        pos = open_tag_end
        close_tag_pattern = re.compile(rf'</?{tag}\b[^>]*>', re.IGNORECASE)

        end_pos = -1
        close_tag_end = -1
        while True:
            cm = close_tag_pattern.search(html, pos)
            if not cm:
                break
            matched_str = cm.group(0)
            if matched_str.startswith('</'):
                depth -= 1
                if depth == 0:
                    end_pos = cm.start()
                    close_tag_end = cm.end()
                    break
            else:
                depth += 1
            pos = cm.end()

        if end_pos != -1:
            elements.append({
                'type': 'paired',
                'tag': tag,
                'key': key,
                'open_start': open_tag_start,
                'open_end': open_tag_end,
                'content_start': open_tag_end,
                'content_end': end_pos,
                'close_end': close_tag_end
            })
        else:
            raise ValueError(f"Unmatched tag for data-i18n='{key}' <{tag}>")

    return elements


def streamline_translations(data, lang):
    """Cleans and shortens navigation terms so navbar never breaks into double lines."""
    d = dict(data)
    if 'NAV_SPEEDUP' in d:
        d['NAV_SPEEDUP'] = re.sub(r'\s*\([^)]*\)', '', d['NAV_SPEEDUP']).strip()

    if 'FREQUENCIES_SUBTITLE' in d and '<!--' not in d['FREQUENCIES_SUBTITLE']:
        d['FREQUENCIES_SUBTITLE'] = re.sub(
            r'(</strong>)(\s*\S+\s*<strong>\s*LQ4\s*</strong>)',
            r'\1<!-- \2 -->',
            d['FREQUENCIES_SUBTITLE']
        )

    if lang == 'en':
        d['NAV_TECH_SPECS'] = 'Specs'
        d['NAV_COMMUNITY'] = 'Community'
        d['NAV_SPEEDUP'] = 'Speed-Up'
    else:
        if 'NAV_TECH_SPECS' in d:
            val = re.sub(r'\s*\([^)]*\)', '', d['NAV_TECH_SPECS']).strip()
            val = re.split(r'\s+(&|&amp;|\+|and|und|et|y|e|و|e|아|및|และ)\s+', val)[0].strip()
            d['NAV_TECH_SPECS'] = val

        if 'NAV_COMMUNITY' in d:
            val = re.sub(r'\s*\([^)]*\)', '', d['NAV_COMMUNITY']).strip()
            val = re.split(r'\s+(&|&amp;|\+|and|und|et|y|e|و|e|아|및|และ)\s+', val)[0].strip()
            d['NAV_COMMUNITY'] = val

        if 'NAV_SHARE' in d:
            val = re.sub(r'\s*\([^)]*\)', '', d['NAV_SHARE']).strip()
            d['NAV_SHARE'] = val

    return d


def apply_translations_to_html(html, lang, data):
    """Pre-renders translated text into HTML markup."""
    streamlined = streamline_translations(data, lang)
    elements = find_i18n_elements(html)

    # Replace backwards by position so offsets remain valid
    modified = html
    for el in reversed(elements):
        key = el['key']
        if key not in streamlined:
            continue
        val = streamlined[key]

        if el['type'] == 'paired':
            modified = modified[:el['content_start']] + val + modified[el['content_end']:]
        elif el['type'] == 'img':
            tag_str = modified[el['open_start']:el['open_end']]
            new_tag_str = re.sub(r'alt=\"[^\"]*\"', f'alt="{val}"', tag_str)
            modified = modified[:el['open_start']] + new_tag_str + modified[el['open_end']:]

    # Update metadata tags
    meta_title = streamlined.get('HERO_MODE_TITLE', 'The LQ8 Digital Mode')
    meta_desc = streamlined.get('META_DESC', '')
    og_desc = streamlined.get('OG_DESC', meta_desc)
    twitter_desc = streamlined.get('TWITTER_DESC', og_desc)

    modified = re.sub(r'<title>.*?</title>', f'<title>{meta_title}</title>', modified, flags=re.DOTALL)
    modified = re.sub(r'<meta name=\"description\" content=\".*?\">', f'<meta name="description" content="{meta_desc}">', modified)
    modified = re.sub(r'<meta property=\"og:title\" content=\".*?\">', f'<meta property="og:title" content="{meta_title}">', modified)
    modified = re.sub(r'<meta property=\"og:description\" content=\".*?\">', f'<meta property="og:description" content="{og_desc}">', modified)
    modified = re.sub(r'<meta name=\"twitter:title\" content=\".*?\">', f'<meta name="twitter:title" content="{meta_title}">', modified)
    modified = re.sub(r'<meta name=\"twitter:description\" content=\".*?\">', f'<meta name="twitter:description" content="{twitter_desc}">', modified)

    # OpenGraph & Twitter URL
    page_url = f"{BASE_URL}/" if lang == 'en' else f"{BASE_URL}/{lang}/"
    modified = re.sub(r'<meta property=\"og:url\" content=\".*?\">', f'<meta property="og:url" content="{page_url}">', modified)
    modified = re.sub(r'<meta name=\"twitter:url\" content=\".*?\">', f'<meta name="twitter:url" content="{page_url}">', modified)

    return modified


def update_head_tags(html, lang):
    """Updates SEO hreflangs and language switcher script in <head> for the specified language."""
    hreflang_block = build_hreflang_tags(lang)
    script_block = build_script_head(lang)

    # Replace or insert hreflangs
    if '<!-- SEO Canonical & Hreflang Tags -->' in html:
        html = re.sub(
            r'\s*<!-- SEO Canonical & Hreflang Tags -->[\s\S]*?<!-- End SEO Canonical & Hreflang Tags -->',
            f"\n{hreflang_block}",
            html
        )
    else:
        html = html.replace('  <link rel="icon"', f"{hreflang_block}\n\n  <link rel=\"icon\"")

    # Replace or insert script block (cleanly consuming any trailing duplicated comments)
    script_pattern = r'\s*<!-- Language Switcher & Redirection -->[\s\S]*?(?:<!-- End Language Switcher & Redirection -->|</script>)(?:\s*<!-- End Language Switcher & Redirection -->)*'
    if re.search(script_pattern, html):
        html = re.sub(script_pattern, f"\n{script_block}", html, count=1)
    elif '<script>' in html and 'supportedLanguages' in html:
        html = re.sub(r'\s*<script>[\s\S]*?const supportedLanguages =[\s\S]*?</script>', f"\n{script_block}", html, count=1)
    else:
        html = html.replace('  <link rel="icon"', f"{script_block}\n\n  <link rel=\"icon\"")

    return html


def build_sitemap():
    """Generates the multi-language XML sitemap."""
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9"',
        '        xmlns:xhtml="http://www.w3.org/1999/xhtml">'
    ]

    for lang in SUPPORTED_LANGUAGES:
        loc = f"{BASE_URL}/" if lang == 'en' else f"{BASE_URL}/{lang}/"
        priority = "1.0" if lang == 'en' else "0.8"

        lines.append('  <url>')
        lines.append(f'    <loc>{loc}</loc>')
        lines.append(f'    <xhtml:link rel="alternate" hreflang="x-default" href="{BASE_URL}/"/>')
        lines.append(f'    <xhtml:link rel="alternate" hreflang="en" href="{BASE_URL}/"/>')

        for other_lang in SUPPORTED_LANGUAGES:
            if other_lang == 'en':
                continue
            lines.append(f'    <xhtml:link rel="alternate" hreflang="{other_lang}" href="{BASE_URL}/{other_lang}/"/>')

        lines.append('    <changefreq>weekly</changefreq>')
        lines.append(f'    <priority>{priority}</priority>')
        lines.append('  </url>')

    lines.append('</urlset>')
    return "\n".join(lines)


def main():
    print("================================================================================")
    print("  LQ Digital Mode Portal — Static Translations Builder")
    print("================================================================================")

    base_html_path = os.path.join(DOCS_DIR, "index.html")
    with open(base_html_path, "r", encoding="utf-8") as f:
        raw_html = f.read()

    fixed_html = raw_html

    # 1. Streamline Brand Logo: Replace long title with punchy LQ8 logo (no wrapping)
    logo_target = r'<a href="#" class="brand-logo"[^>]*>.*?</a>'
    logo_replacement = '<a href="#" class="brand-logo"><span class="gradient-text">LQ8</span></a>'
    fixed_html = re.sub(logo_target, logo_replacement, fixed_html)
    print("✓ Streamlined brand logo to punchy 'LQ8' mark.")

    # 2. Streamline base nav-links labels in base template
    nav_links_target = r'<ul class="nav-links">[\s\S]*?</ul>'
    nav_links_replacement = """<ul class="nav-links">
        <li><a href="#overview" data-i18n="NAV_OVERVIEW">Overview</a></li>
        <li><a href="#operation" data-i18n="NAV_OPERATION">Operation</a></li>
        <li><a href="#speedup" data-i18n="NAV_SPEEDUP">Speed-Up</a></li>
        <li><a href="#roadmap" data-i18n="NAV_ROADMAP">Roadmap</a></li>
        <li><a href="#technical" data-i18n="NAV_TECH_SPECS">Specs</a></li>
        <li><a href="#community" data-i18n="NAV_COMMUNITY">Community</a></li>
      </ul>"""
    fixed_html = re.sub(nav_links_target, nav_links_replacement, fixed_html)
    print("✓ Streamlined nav-links items in base template.")

    # 3. Ensure language dropdown markup is present in nav-container
    dropdown_html = build_lang_dropdown_html('en')
    if 'class="lang-selector-wrapper"' not in fixed_html:
        fixed_html = re.sub(nav_links_target, lambda m: m.group(0) + '\n\n' + dropdown_html, fixed_html)
    else:
        fixed_html = re.sub(r'<div class="lang-selector-wrapper">[\s\S]*?</div>', dropdown_html.strip(), fixed_html)

    # 4. Streamline Header and Navigation CSS
    streamlined_css = """    /* Header */
    .site-header {
      position: sticky;
      top: 0;
      z-index: 1000;
      background: rgba(7, 9, 14, 0.92);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border-bottom: 1px solid var(--border-subtle);
    }
    .nav-container {
      max-width: var(--max-width);
      margin: 0 auto;
      padding: 10px 24px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 16px;
    }
    .brand-logo {
      color: var(--text-primary);
      font-family: var(--font-heading);
      font-size: 1.45rem;
      font-weight: 900;
      letter-spacing: -0.02em;
      white-space: nowrap;
      flex-shrink: 0;
      line-height: 1;
    }
    .brand-logo:hover { text-decoration: none; }
    .nav-links {
      display: flex;
      align-items: center;
      gap: 20px;
      list-style: none;
      margin: 0;
      padding: 0;
    }
    .nav-links li {
      flex-shrink: 0;
    }
    .nav-links a {
      color: var(--text-secondary);
      font-size: 0.88rem;
      font-weight: 500;
      white-space: nowrap;
      transition: color var(--transition);
      line-height: 1;
    }
    .nav-links a:hover {
      color: var(--cyan);
      text-decoration: none;
    }

    /* Header Actions & Share Button */
    .nav-actions {
      display: flex;
      align-items: center;
      gap: 10px;
      flex-shrink: 0;
    }
    .btn-nav-share {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      background: rgba(56, 189, 248, 0.12);
      border: 1px solid rgba(56, 189, 248, 0.35);
      border-radius: var(--radius-full);
      padding: 5px 12px;
      color: var(--cyan);
      font-family: var(--font-body);
      font-size: 0.82rem;
      font-weight: 600;
      cursor: pointer;
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.25);
      transition: all var(--transition);
      flex-shrink: 0;
      line-height: 1;
      text-decoration: none;
    }
    .btn-nav-share:hover {
      background: rgba(56, 189, 248, 0.22);
      border-color: var(--cyan);
      color: #ffffff;
      transform: translateY(-1px);
      box-shadow: 0 4px 12px rgba(56, 189, 248, 0.3);
    }

    /* Language Selector Dropdown */
    .lang-selector-wrapper {
      display: inline-flex;
      align-items: center;
      gap: 5px;
      background: rgba(16, 23, 38, 0.85);
      border: 1px solid var(--border-card);
      border-radius: var(--radius-full);
      padding: 4px 10px;
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.25);
      transition: border-color var(--transition), background var(--transition);
      flex-shrink: 0;
    }
    .lang-selector-wrapper:hover {
      border-color: var(--cyan);
      background: rgba(22, 32, 54, 0.95);
    }
    .lang-icon {
      font-size: 0.92rem;
      line-height: 1;
    }
    .lang-dropdown {
      background: transparent;
      color: var(--text-primary);
      border: none;
      font-family: var(--font-body);
      font-size: 0.82rem;
      font-weight: 500;
      cursor: pointer;
      outline: none;
      padding: 1px 2px;
    }
    .lang-dropdown option {
      background: #07090e;
      color: #f8fafc;
    }

    @media (max-width: 1120px) {
      .nav-links { gap: 14px; }
      .nav-links a { font-size: 0.82rem; }
      .nav-container { padding: 9px 18px; gap: 12px; }
    }
    @media (max-width: 920px) {
      .nav-links { gap: 10px; }
      .nav-links a { font-size: 0.78rem; }
      .brand-logo { font-size: 1.25rem; }
      .lang-selector-wrapper { padding: 3px 7px; gap: 3px; }
      .lang-dropdown { font-size: 0.78rem; }
      .btn-nav-share { padding: 4px 9px; font-size: 0.78rem; gap: 4px; }
    }
    @media (max-width: 768px) {
      .nav-links { display: none; }
      .nav-container { padding: 9px 16px; }
    }
    @media (max-width: 480px) {
      .btn-nav-share span { display: none; }
      .btn-nav-share { padding: 5px 8px; }
      .nav-actions { gap: 6px; }
    }"""

    header_css_pattern = r'\n\s*/\* Header \*/[\s\S]*?(?=\n[ \t]*\.btn\s*\{)'
    if re.search(header_css_pattern, fixed_html):
        fixed_html = re.sub(header_css_pattern, '\n\n' + streamlined_css + '\n', fixed_html)
        print("✓ Updated CSS with streamlined, non-wrapping header styles.")

    # 5. Build root English index.html
    root_html = update_head_tags(fixed_html, 'en')

    with open(base_html_path, "w", encoding="utf-8") as f:
        f.write(root_html)
    print(f"✓ Updated root {base_html_path}")

    # 6. Generate translated files for each supported language
    total_languages = len(SUPPORTED_LANGUAGES)
    print(f"\nBuilding streamlined static portals for {total_languages - 1} non-English languages...")

    for lang in SUPPORTED_LANGUAGES:
        if lang == 'en':
            continue

        translations = load_translations(lang)
        lang_dir = os.path.join(DOCS_DIR, lang)
        os.makedirs(lang_dir, exist_ok=True)
        lang_file_path = os.path.join(lang_dir, "index.html")

        # Start from base fixed HTML
        page_html = fixed_html

        # Update <html> lang and dir attributes
        dir_attr = ' dir="rtl"' if lang in RTL_LANGUAGES else ''
        page_html = re.sub(r'<html lang="[a-zA-Z\-]+"( dir=\"rtl\")?>', f'<html lang="{lang}"{dir_attr}>', page_html)

        # Update language selector active option
        lang_dropdown = build_lang_dropdown_html(lang)
        page_html = re.sub(r'<div class="lang-selector-wrapper">[\s\S]*?</div>', lang_dropdown.strip(), page_html)

        # Update head with localized hreflangs and switcher script
        page_html = update_head_tags(page_html, lang)

        # Update relative assets to parent directory
        page_html = page_html.replace('href="favicon.svg"', 'href="../favicon.svg"')
        page_html = page_html.replace('href="lq_digitalmode.pdf"', 'href="../lq_digitalmode.pdf"')
        page_html = page_html.replace('src="qft8.jpg"', 'src="../qft8.jpg"')
        page_html = page_html.replace('src="tones_waterfall_lq8.png"', 'src="../tones_waterfall_lq8.png"')

        # Apply translations across all data-i18n elements and meta tags
        page_html = apply_translations_to_html(page_html, lang, translations)

        with open(lang_file_path, "w", encoding="utf-8") as f:
            f.write(page_html)

        print(f"  [{lang:>3}] {LANGUAGE_NAMES.get(lang, lang):<22} -> docs/{lang}/index.html ({len(page_html):,} bytes)")

    # 7. Generate sitemap.xml
    sitemap_xml = build_sitemap()
    sitemap_path = os.path.join(DOCS_DIR, "sitemap.xml")
    with open(sitemap_path, "w", encoding="utf-8") as f:
        f.write(sitemap_xml)
    print(f"\n✓ Generated {sitemap_path} ({len(sitemap_xml):,} bytes, {total_languages} URLs with cross-hreflangs)")

    print("\n✓ Streamlined build completed successfully!")


if __name__ == "__main__":
    main()
