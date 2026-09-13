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
update_award_translations.py

Adds localized strings for COMMUNITY_AWARD_LABEL, COMMUNITY_FIRST_QSO_USING,
and COMMUNITY_FIRST_POTA_QSO_USING across all 63 language files in
docs/languages/lang*.js.
"""

import json
import os
import re

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DOCS_DIR = os.path.join(BASE_DIR, "docs")
LANG_DIR = os.path.join(DOCS_DIR, "languages")

TRANSLATIONS = {
    'en': {'award': 'award', 'first_qso': 'First QSO using LQ8', 'first_pota_qso': 'First POTA QSO using LQ8'},
    'es': {'award': 'diploma', 'first_qso': 'Primer QSO usando LQ8', 'first_pota_qso': 'Primer QSO POTA usando LQ8'},
    'pt': {'award': 'diploma', 'first_qso': 'Primeiro QSO usando LQ8', 'first_pota_qso': 'Primeiro QSO POTA usando LQ8'},
    'fr': {'award': 'diplôme', 'first_qso': 'Premier QSO avec LQ8', 'first_pota_qso': 'Premier QSO POTA avec LQ8'},
    'it': {'award': 'diploma', 'first_qso': 'Primo QSO usando LQ8', 'first_pota_qso': 'Primo QSO POTA usando LQ8'},
    'de': {'award': 'Diplom', 'first_qso': 'Erstes QSO mit LQ8', 'first_pota_qso': 'Erstes POTA-QSO mit LQ8'},
    'ru': {'award': 'диплом', 'first_qso': 'Первое QSO с использованием LQ8', 'first_pota_qso': 'Первое POTA QSO с использованием LQ8'},
    'zh': {'award': '奖状', 'first_qso': '首次使用 LQ8 的 QSO', 'first_pota_qso': '首次使用 LQ8 的 POTA QSO'},
    'ja': {'award': 'アワード', 'first_qso': 'LQ8を使用した初QSO', 'first_pota_qso': 'LQ8を使用した初のPOTA QSO'},
    'ko': {'award': '어워드', 'first_qso': 'LQ8을 사용한 첫 QSO', 'first_pota_qso': 'LQ8을 사용한 첫 POTA QSO'},
    'ar': {'award': 'جائزة', 'first_qso': 'أول اتصال QSO باستخدام LQ8', 'first_pota_qso': 'أول اتصال POTA QSO باستخدام LQ8'},
    'hi': {'award': 'पुरस्कार', 'first_qso': 'LQ8 का उपयोग करके पहला QSO', 'first_pota_qso': 'LQ8 का उपयोग करके पहला POTA QSO'},
    'bn': {'award': 'পুরস্কার', 'first_qso': 'LQ8 ব্যবহার করে প্রথম QSO', 'first_pota_qso': 'LQ8 ব্যবহার করে প্রথম POTA QSO'},
    'ur': {'award': 'ایوارڈ', 'first_qso': 'LQ8 کا استعمال کرتے ہوئے پہلا QSO', 'first_pota_qso': 'LQ8 کا استعمال کرتے ہوئے پہلا POTA QSO'},
    'tr': {'award': 'ödül', 'first_qso': 'LQ8 kullanılarak yapılan ilk QSO', 'first_pota_qso': 'LQ8 kullanılarak yapılan ilk POTA QSO'},
    'vi': {'award': 'giải thưởng', 'first_qso': 'QSO đầu tiên sử dụng LQ8', 'first_pota_qso': 'POTA QSO đầu tiên sử dụng LQ8'},
    'id': {'award': 'penghargaan', 'first_qso': 'QSO pertama menggunakan LQ8', 'first_pota_qso': 'QSO POTA pertama menggunakan LQ8'},
    'nl': {'award': 'diploma', 'first_qso': 'Eerste QSO met behulp van LQ8', 'first_pota_qso': 'Eerste POTA-QSO met behulp van LQ8'},
    'pl': {'award': 'dyplom', 'first_qso': 'Pierwsze QSO przy użyciu LQ8', 'first_pota_qso': 'Pierwsze QSO POTA przy użyciu LQ8'},
    'sv': {'award': 'diplom', 'first_qso': 'Första QSO med LQ8', 'first_pota_qso': 'Första POTA-QSO med LQ8'},
    'fi': {'award': 'awardi', 'first_qso': 'Ensimmäinen QSO LQ8:lla', 'first_pota_qso': 'Ensimmäinen POTA-QSO LQ8:lla'},
    'da': {'award': 'diplom', 'first_qso': 'Første QSO med LQ8', 'first_pota_qso': 'Første POTA-QSO med LQ8'},
    'no': {'award': 'diplom', 'first_qso': 'Første QSO med LQ8', 'first_pota_qso': 'Første POTA-QSO med LQ8'},
    'cs': {'award': 'diplom', 'first_qso': 'První QSO pomocí LQ8', 'first_pota_qso': 'První QSO POTA pomocí LQ8'},
    'sk': {'award': 'diplom', 'first_qso': 'Prvé QSO pomocou LQ8', 'first_pota_qso': 'Prvé QSO POTA pomocou LQ8'},
    'hu': {'award': 'oklevél', 'first_qso': 'Első QSO LQ8 használatával', 'first_pota_qso': 'Első POTA QSO LQ8 használatával'},
    'ro': {'award': 'diplomă', 'first_qso': 'Primul QSO utilizând LQ8', 'first_pota_qso': 'Primul QSO POTA utilizând LQ8'},
    'el': {'award': 'βραβείο', 'first_qso': 'Πρώτο QSO με χρήση LQ8', 'first_pota_qso': 'Πρώτο POTA QSO με χρήση LQ8'},
    'he': {'award': 'תעודה', 'first_qso': 'QSO ראשון באמצעות LQ8', 'first_pota_qso': 'QSO של POTA ראשון באמצעות LQ8'},
    'th': {'award': 'รางวัล', 'first_qso': 'QSO แรกโดยใช้ LQ8', 'first_pota_qso': 'QSO POTA แรกโดยใช้ LQ8'},
    'ca': {'award': 'diploma', 'first_qso': 'Primer QSO usant LQ8', 'first_pota_qso': 'Primer QSO POTA usant LQ8'},
    'eu': {'award': 'saria', 'first_qso': 'Lehen QSOa LQ8 erabiliz', 'first_pota_qso': 'Lehen POTA QSOa LQ8 erabiliz'},
    'gl': {'award': 'diploma', 'first_qso': 'Primeiro QSO usando LQ8', 'first_pota_qso': 'Primeiro QSO POTA usando LQ8'},
    'eo': {'award': 'diplomo', 'first_qso': 'Unua QSO uzante LQ8', 'first_pota_qso': 'Unua POTA-QSO uzante LQ8'},
    'fa': {'award': 'جایزه', 'first_qso': 'اولین QSO با استفاده از LQ8', 'first_pota_qso': 'اولین POTA QSO با استفاده از LQ8'},
    'uk': {'award': 'диплом', 'first_qso': 'Перше QSO з використанням LQ8', 'first_pota_qso': 'Перше POTA QSO з використанням LQ8'},
    'sw': {'award': 'tuzo', 'first_qso': 'QSO ya kwanza kwa kutumia LQ8', 'first_pota_qso': 'QSO ya kwanza ya POTA kwa kutumia LQ8'},
    'ha': {'award': 'lambar yabo', 'first_qso': 'QSO na farko ta amfani da LQ8', 'first_pota_qso': 'QSO na POTA na farko ta amfani da LQ8'},
    'yo': {'award': 'àmi-ẹ̀yẹ', 'first_qso': 'QSO akọkọ nipa lilo LQ8', 'first_pota_qso': 'QSO POTA akọkọ nipa lilo LQ8'},
    'am': {'award': 'ሽልማት', 'first_qso': 'በLQ8 የመጀመሪያው የQSO ግንኙነት', 'first_pota_qso': 'በLQ8 የመጀመሪያው የPOTA QSO ግንኙነት'},
    'az': {'award': 'diplom', 'first_qso': 'LQ8 istifadə edərək ilk QSO', 'first_pota_qso': 'LQ8 istifadə edərək ilk POTA QSO'},
    'uz': {'award': 'diplom', 'first_qso': 'LQ8 yordamida birinchi QSO', 'first_pota_qso': 'LQ8 yordamida birinchi POTA QSO'},
    'ta': {'award': 'விருது', 'first_qso': 'LQ8 ஐப் பயன்படுத்தி முதல் QSO', 'first_pota_qso': 'LQ8 ஐப் பயன்படுத்தி முதல் POTA QSO'},
    'te': {'award': 'అవార్డు', 'first_qso': 'LQ8 ని ఉపయోగించి మొదటి QSO', 'first_pota_qso': 'LQ8 ని ఉపయోగించి మొదటి POTA QSO'},
    'mr': {'award': 'पुरस्कार', 'first_qso': 'LQ8 चा वापर करून पहिले QSO', 'first_pota_qso': 'LQ8 चा वापर करून पहिले POTA QSO'},
    'gu': {'award': 'એવોર્ડ', 'first_qso': 'LQ8 નો ઉપયોગ કરીને પ્રથમ QSO', 'first_pota_qso': 'LQ8 નો ઉપયોગ કરીને પ્રથમ POTA QSO'},
    'kn': {'award': 'ಪ್ರಶಸ್ತಿ', 'first_qso': 'LQ8 ಬಳಸಿಕೊಂಡು ಮೊದಲ QSO', 'first_pota_qso': 'LQ8 ಬಳಸಿಕೊಂಡು ಮೊದಲ POTA QSO'},
    'ml': {'award': 'അവാർഡ്', 'first_qso': 'LQ8 ഉപയോഗിച്ചുള്ള ആദ്യ QSO', 'first_pota_qso': 'LQ8 ഉപയോഗിച്ചുള്ള ആദ്യ POTA QSO'},
    'pa': {'award': 'ਅਵਾਰਡ', 'first_qso': 'LQ8 ਦੀ ਵਰਤੋਂ ਕਰਕੇ ਪਹਿਲਾ QSO', 'first_pota_qso': 'LQ8 ਦੀ ਵਰਤੋਂ ਕਰਕੇ ਪਹਿਲਾ POTA QSO'},
    'or': {'award': 'ପୁରସ୍କାର', 'first_qso': 'LQ8 ବ୍ୟବହାର କରି ପ୍ରଥମ QSO', 'first_pota_qso': 'LQ8 ବ୍ୟବହାର କରି ପ୍ରଥମ POTA QSO'},
    'my': {'award': 'ဆုတံဆိပ်', 'first_qso': 'LQ8 အသုံးပြု၍ ပထမဆုံး QSO', 'first_pota_qso': 'LQ8 အသုံးပြု၍ ပထမဆုံး POTA QSO'},
    'tl': {'award': 'parangal', 'first_qso': 'Unang QSO gamit ang LQ8', 'first_pota_qso': 'Unang POTA QSO gamit ang LQ8'},
    'jv': {'award': 'penghargaan', 'first_qso': 'QSO pisanan nggunakake LQ8', 'first_pota_qso': 'QSO POTA pisanan nggunakake LQ8'},
    'su': {'award': 'panghargaan', 'first_qso': 'QSO munggaran ngagunakeun LQ8', 'first_pota_qso': 'QSO POTA munggaran ngagunakeun LQ8'},
    'bho': {'award': 'पुरस्कार', 'first_qso': 'LQ8 के इस्तेमाल से पहिला QSO', 'first_pota_qso': 'LQ8 के इस्तेमाल से पहिला POTA QSO'},
    'pcm': {'award': 'award', 'first_qso': 'First QSO using LQ8', 'first_pota_qso': 'First POTA QSO using LQ8'},
    'wuu': {'award': '奖状', 'first_qso': '头一趟使用 LQ8 个 QSO', 'first_pota_qso': '头一趟使用 LQ8 个 POTA QSO'},
    'yue': {'award': '獎狀', 'first_qso': '首次使用 LQ8 嘅 QSO', 'first_pota_qso': '首次使用 LQ8 嘅 POTA QSO'},
    'apc': {'award': 'جائزة', 'first_qso': 'أول اتصال QSO باستخدام LQ8', 'first_pota_qso': 'أول اتصال POTA QSO باستخدام LQ8'},
    'apd': {'award': 'جائزة', 'first_qso': 'أول اتصال QSO باستخدام LQ8', 'first_pota_qso': 'أول اتصال POTA QSO باستخدام LQ8'},
    'arq': {'award': 'جائزة', 'first_qso': 'أول اتصال QSO باستخدام LQ8', 'first_pota_qso': 'أول اتصال POTA QSO باستخدام LQ8'},
    'ary': {'award': 'جائزة', 'first_qso': 'أول اتصال QSO باستعمال LQ8', 'first_pota_qso': 'أول اتصال POTA QSO باستعمال LQ8'},
    'arz': {'award': 'جايزة', 'first_qso': 'أول اتصال QSO باستخدام LQ8', 'first_pota_qso': 'أول اتصال POTA QSO باستخدام LQ8'},
}


def main():
    print("Updating language dictionary files with award and QSO keys...")
    updated_count = 0

    for lang, t in TRANSLATIONS.items():
        file_path = os.path.join(LANG_DIR, f"lang{lang}.js")
        if not os.path.exists(file_path):
            print(f"Warning: File not found: {file_path}")
            continue

        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read()

        award_val = json.dumps(t['award'], ensure_ascii=False)
        first_qso_val = json.dumps(t['first_qso'], ensure_ascii=False)
        first_pota_qso_val = json.dumps(t['first_pota_qso'], ensure_ascii=False)
        first_sota_qso_val = json.dumps(t.get('first_sota_qso', t['first_pota_qso'].replace('POTA', 'SOTA')), ensure_ascii=False)

        # Check if already has COMMUNITY_AWARD_LABEL
        if '"COMMUNITY_AWARD_LABEL"' in content:
            # Replace existing keys if needed
            content = re.sub(
                r'"COMMUNITY_AWARD_LABEL":\s*"[^"]*",?',
                f'"COMMUNITY_AWARD_LABEL": {award_val},',
                content
            )
            content = re.sub(
                r'"COMMUNITY_FIRST_QSO_USING":\s*"[^"]*",?',
                f'"COMMUNITY_FIRST_QSO_USING": {first_qso_val},',
                content
            )
            if '"COMMUNITY_FIRST_POTA_QSO_USING"' in content:
                content = re.sub(
                    r'"COMMUNITY_FIRST_POTA_QSO_USING":\s*"[^"]*",?',
                    f'"COMMUNITY_FIRST_POTA_QSO_USING": {first_pota_qso_val},',
                    content
                )
            else:
                match = re.search(r'("COMMUNITY_FIRST_QSO_USING":\s*"[^"]*",)', content)
                if match:
                    content = (
                        content[:match.end()] +
                        f'\n    "COMMUNITY_FIRST_POTA_QSO_USING": {first_pota_qso_val},' +
                        content[match.end():]
                    )
            if '"COMMUNITY_FIRST_SOTA_QSO_USING"' in content:
                content = re.sub(
                    r'"COMMUNITY_FIRST_SOTA_QSO_USING":\s*"[^"]*",?',
                    f'"COMMUNITY_FIRST_SOTA_QSO_USING": {first_sota_qso_val},',
                    content
                )
            else:
                match = re.search(r'("COMMUNITY_FIRST_POTA_QSO_USING":\s*"[^"]*",)', content)
                if match:
                    content = (
                        content[:match.end()] +
                        f'\n    "COMMUNITY_FIRST_SOTA_QSO_USING": {first_sota_qso_val},' +
                        content[match.end():]
                    )
        else:
            # Insert after COMMUNITY_EMPTY_NOTE
            note_match = re.search(r'("COMMUNITY_EMPTY_NOTE":\s*"[^"]*",)', content)
            if note_match:
                insertion = (
                    f'{note_match.group(1)}\n'
                    f'    "COMMUNITY_AWARD_LABEL": {award_val},\n'
                    f'    "COMMUNITY_FIRST_QSO_USING": {first_qso_val},\n'
                    f'    "COMMUNITY_FIRST_POTA_QSO_USING": {first_pota_qso_val},\n'
                    f'    "COMMUNITY_FIRST_SOTA_QSO_USING": {first_sota_qso_val},'
                )
                content = content[:note_match.start()] + insertion + content[note_match.end():]
            else:
                print(f"Error: COMMUNITY_EMPTY_NOTE not found in {lang}")
                continue

        with open(file_path, "w", encoding="utf-8") as f:
            f.write(content)

        updated_count += 1

    print(f"Successfully updated {updated_count} language files.")


if __name__ == "__main__":
    main()
