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
update_translations_data.py

Updates the 4 frequency section keys across all 64 language dictionaries in
docs/languages/lang*.js to include the expanded 7-band HF allocations:
40m, 30m, 20m, 17m, 15m, 12m, and 10m.
"""

import json
import os
import re
import subprocess
import sys

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
LANG_DIR = os.path.join(BASE_DIR, "docs", "languages")

# Definitions for all 64 supported languages
# (band_list_7, future_list, warc_list, classic_list)
LANG_CONFIGS = {
    'en': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m, and 10m',
        'future': '80m, 160m, 60m, and VHF/UHF bands',
        'warc': '30m, 17m, and 12m',
        'classic': '40m, 20m, 15m, and 10m'
    },
    'es': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m y 10m',
        'future': '80m, 160m, 60m y bandas de VHF/UHF',
        'warc': '30m, 17m y 12m',
        'classic': '40m, 20m, 15m y 10m'
    },
    'pt': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m e 10m',
        'future': '80m, 160m, 60m e bandas VHF/UHF',
        'warc': '30m, 17m e 12m',
        'classic': '40m, 20m, 15m e 10m'
    },
    'fr': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m et 10m',
        'future': '80m, 160m, 60m et bandes VHF/UHF',
        'warc': '30m, 17m et 12m',
        'classic': '40m, 20m, 15m et 10m'
    },
    'it': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m e 10m',
        'future': '80m, 160m, 60m e bande VHF/UHF',
        'warc': '30m, 17m e 12m',
        'classic': '40m, 20m, 15m e 10m'
    },
    'de': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m und 10m',
        'future': '80m, 160m, 60m sowie VHF/UHF-Bänder',
        'warc': '30m, 17m und 12m',
        'classic': '40m, 20m, 15m und 10m'
    },
    'ru': {
        'bands_7': '40м, 30м, 20м, 17м, 15м, 12м и 10м',
        'future': '80м, 160м, 60м и диапазоны УКВ (VHF/UHF)',
        'warc': '30м, 17м и 12м',
        'classic': '40м, 20м, 15м и 10м'
    },
    'uk': {
        'bands_7': '40м, 30м, 20м, 17м, 15м, 12м та 10м',
        'future': '80м, 160м, 60м та діапазони УКХ (VHF/UHF)',
        'warc': '30м, 17м та 12м',
        'classic': '40м, 20м, 15м та 10м'
    },
    'zh': {
        'bands_7': '40m、30m、20m、17m、15m、12m 和 10m',
        'future': '80m、160m、60m 及 VHF/UHF 频段',
        'warc': '30m、17m 和 12m',
        'classic': '40m、20m、15m 和 10m'
    },
    'yue': {
        'bands_7': '40m、30m、20m、17m、15m、12m 同 10m',
        'future': '80m、160m、60m 及 VHF/UHF 頻段',
        'warc': '30m、17m 同 12m',
        'classic': '40m、20m、15m 同 10m'
    },
    'wuu': {
        'bands_7': '40m、30m、20m、17m、15m、12m 跟 10m',
        'future': '80m、160m、60m 跟 VHF/UHF 频段',
        'warc': '30m、17m 跟 12m',
        'classic': '40m、20m、15m 跟 10m'
    },
    'ja': {
        'bands_7': '40m・30m・20m・17m・15m・12m・10m',
        'future': '80m、160m、60m、VHF/UHFバンド',
        'warc': '30m、17m、12m',
        'classic': '40m、20m、15m、10m'
    },
    'ko': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m, 10m',
        'future': '80m, 160m, 60m 및 VHF/UHF 대역',
        'warc': '30m, 17m, 12m',
        'classic': '40m, 20m, 15m, 10m'
    },
    'ar': {
        'bands_7': '40 متراً و30 متراً و20 متراً و17 متراً و15 متراً و12 متراً و10 أمتار',
        'future': '80 متراً و160 متراً و60 متراً ونطاقات VHF/UHF',
        'warc': '30 متراً و17 متراً و12 متراً',
        'classic': '40 متراً و20 متراً و15 متراً و10 أمتار'
    },
    'apd': {
        'bands_7': '40 متراً و30 متراً و20 متراً و17 متراً و15 متراً و12 متراً و10 أمتار',
        'future': '80 متراً و160 متراً و60 متراً ونطاقات VHF/UHF',
        'warc': '30 متراً و17 متراً و12 متراً',
        'classic': '40 متراً و20 متراً و15 متراً و10 أمتار'
    },
    'arq': {
        'bands_7': '40 متراً و30 متراً و20 متراً و17 متراً و15 متراً و12 متراً و10 أمتار',
        'future': '80 متراً و160 متراً و60 متراً ونطاقات VHF/UHF',
        'warc': '30 متراً و17 متراً و12 متراً',
        'classic': '40 متراً و20 متراً و15 متراً و10 أمتار'
    },
    'ary': {
        'bands_7': '40 متراً و30 متراً و20 متراً و17 متراً و15 متراً و12 متراً و10 أمتار',
        'future': '80 متراً و160 متراً و60 متراً ونطاقات VHF/UHF',
        'warc': '30 متراً و17 متراً و12 متراً',
        'classic': '40 متراً و20 متراً و15 متراً و10 أمتار'
    },
    'arz': {
        'bands_7': '40 متراً و30 متراً و20 متراً و17 متراً و15 متراً و12 متراً و10 أمتار',
        'future': '80 متراً و160 متراً و60 متراً ونطاقات VHF/UHF',
        'warc': '30 متراً و17 متراً و12 متراً',
        'classic': '40 متراً و20 متراً و15 متراً و10 أمتار'
    },
    'apc': {
        'bands_7': '40 و30 و20 و17 و15 و12 و10 متر',
        'future': '80 و160 و60 متر ونطاقات VHF/UHF',
        'warc': '30 و17 و12 متر',
        'classic': '40 و20 و15 و10 متر'
    },
    'fa': {
        'bands_7': '40، 30، 20، 17، 15، 12 و 10 متر',
        'future': '80، 160، 60 متر و باندهای VHF/UHF',
        'warc': '30، 17 و 12 متر',
        'classic': '40، 20، 15 و 10 متر'
    },
    'he': {
        'bands_7': '40 מטר, 30 מטר, 20 מטר, 17 מטר, 15 מטר, 12 מטר ו-10 מטר',
        'future': '80 מטר, 160 מטר, 60 מטר ופסי VHF/UHF',
        'warc': '30 מטר, 17 מטר ו-12 מטר',
        'classic': '40 מטר, 20 מטר, 15 מטר ו-10 מטר'
    },
    'hi': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m और 10m',
        'future': '80m, 160m, 60m और VHF/UHF बैंड',
        'warc': '30m, 17m और 12m',
        'classic': '40m, 20m, 15m और 10m'
    },
    'bn': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m এবং 10m',
        'future': '80m, 160m, 60m এবং VHF/UHF ব্যান্ড',
        'warc': '30m, 17m এবং 12m',
        'classic': '40m, 20m, 15m এবং 10m'
    },
    'ur': {
        'bands_7': '40m، 30m، 20m، 17m، 15m، 12m اور 10m',
        'future': '80m، 160m، 60m اور VHF/UHF بینڈز',
        'warc': '30m، 17m اور 12m',
        'classic': '40m، 20m، 15m اور 10m'
    },
    'tr': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m ve 10m',
        'future': '80m, 160m, 60m ve VHF/UHF bantları',
        'warc': '30m, 17m ve 12m',
        'classic': '40m, 20m, 15m ve 10m'
    },
    'vi': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m và 10m',
        'future': '80m, 160m, 60m và các băng tần VHF/UHF',
        'warc': '30m, 17m và 12m',
        'classic': '40m, 20m, 15m và 10m'
    },
    'id': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m, dan 10m',
        'future': '80m, 160m, 60m, serta pita VHF/UHF',
        'warc': '30m, 17m, dan 12m',
        'classic': '40m, 20m, 15m, dan 10m'
    },
    'nl': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m en 10m',
        'future': '80m, 160m, 60m en VHF/UHF-banden',
        'warc': '30m, 17m en 12m',
        'classic': '40m, 20m, 15m en 10m'
    },
    'pl': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m i 10m',
        'future': '80m, 160m, 60m oraz pasma VHF/UHF',
        'warc': '30m, 17m i 12m',
        'classic': '40m, 20m, 15m i 10m'
    },
    'sv': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m och 10m',
        'future': '80m, 160m, 60m och VHF/UHF-band',
        'warc': '30m, 17m och 12m',
        'classic': '40m, 20m, 15m och 10m'
    },
    'fi': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m ja 10m',
        'future': '80m, 160m, 60m sekä VHF/UHF-alueet',
        'warc': '30m, 17m ja 12m',
        'classic': '40m, 20m, 15m ja 10m'
    },
    'da': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m og 10m',
        'future': '80m, 160m, 60m og VHF/UHF-bånd',
        'warc': '30m, 17m og 12m',
        'classic': '40m, 20m, 15m og 10m'
    },
    'no': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m og 10m',
        'future': '80m, 160m, 60m og VHF/UHF-bånd',
        'warc': '30m, 17m og 12m',
        'classic': '40m, 20m, 15m og 10m'
    },
    'cs': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m a 10m',
        'future': '80m, 160m, 60m a pásma VHF/UHF',
        'warc': '30m, 17m a 12m',
        'classic': '40m, 20m, 15m a 10m'
    },
    'sk': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m a 10m',
        'future': '80m, 160m, 60m a pásma VHF/UHF',
        'warc': '30m, 17m a 12m',
        'classic': '40m, 20m, 15m a 10m'
    },
    'hu': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m és 10m',
        'future': '80m, 160m, 60m és VHF/UHF sávok',
        'warc': '30m, 17m és 12m',
        'classic': '40m, 20m, 15m és 10m'
    },
    'ro': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m și 10m',
        'future': '80m, 160m, 60m și benzi VHF/UHF',
        'warc': '30m, 17m și 12m',
        'classic': '40m, 20m, 15m și 10m'
    },
    'el': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m και 10m',
        'future': '80m, 160m, 60m και ζώνες VHF/UHF',
        'warc': '30m, 17m και 12m',
        'classic': '40m, 20m, 15m και 10m'
    },
    'th': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m และ 10m',
        'future': '80m, 160m, 60m และย่าน VHF/UHF',
        'warc': '30m, 17m และ 12m',
        'classic': '40m, 20m, 15m และ 10m'
    },
    'ca': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m i 10m',
        'future': '80m, 160m, 60m i bandes de VHF/UHF',
        'warc': '30m, 17m i 12m',
        'classic': '40m, 20m, 15m i 10m'
    },
    'eu': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m eta 10m',
        'future': '80m, 160m, 60m eta VHF/UHF bandak',
        'warc': '30m, 17m eta 12m',
        'classic': '40m, 20m, 15m eta 10m'
    },
    'gl': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m e 10m',
        'future': '80m, 160m, 60m e bandas de VHF/UHF',
        'warc': '30m, 17m e 12m',
        'classic': '40m, 20m, 15m e 10m'
    },
    'eo': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m kaj 10m',
        'future': '80m, 160m, 60m kaj VHF/UHF-bendoj',
        'warc': '30m, 17m kaj 12m',
        'classic': '40m, 20m, 15m kaj 10m'
    },
    'sw': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m na 10m',
        'future': '80m, 160m, 60m na bendi za VHF/UHF',
        'warc': '30m, 17m na 12m',
        'classic': '40m, 20m, 15m na 10m'
    },
    'ha': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m da 10m',
        'future': '80m, 160m, 60m da zangon VHF/UHF',
        'warc': '30m, 17m da 12m',
        'classic': '40m, 20m, 15m da 10m'
    },
    'yo': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m ati 10m',
        'future': '80m, 160m, 60m ati awọn ẹgbẹ VHF/UHF',
        'warc': '30m, 17m ati 12m',
        'classic': '40m, 20m, 15m ati 10m'
    },
    'am': {
        'bands_7': '40m፣ 30m፣ 20m፣ 17m፣ 15m፣ 12m እና 10m',
        'future': '80m፣ 160m፣ 60m እና የ VHF/UHF ባንዶች',
        'warc': '30m፣ 17m እና 12m',
        'classic': '40m፣ 20m፣ 15m እና 10m'
    },
    'az': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m və 10m',
        'future': '80m, 160m, 60m və VHF/UHF diapazonları',
        'warc': '30m, 17m və 12m',
        'classic': '40m, 20m, 15m və 10m'
    },
    'uz': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m va 10m',
        'future': '80m, 160m, 60m va VHF/UHF diapazonlari',
        'warc': '30m, 17m va 12m',
        'classic': '40m, 20m, 15m va 10m'
    },
    'ta': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m மற்றும் 10m',
        'future': '80m, 160m, 60m மற்றும் VHF/UHF அலைவரிசைகள்',
        'warc': '30m, 17m மற்றும் 12m',
        'classic': '40m, 20m, 15m மற்றும் 10m'
    },
    'te': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m మరియు 10m',
        'future': '80m, 160m, 60m మరియు VHF/UHF బ్యాండ్లు',
        'warc': '30m, 17m మరియు 12m',
        'classic': '40m, 20m, 15m మరియు 10m'
    },
    'mr': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m आणि 10m',
        'future': '80m, 160m, 60m आणि VHF/UHF बँड',
        'warc': '30m, 17m आणि 12m',
        'classic': '40m, 20m, 15m आणि 10m'
    },
    'gu': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m અને 10m',
        'future': '80m, 160m, 60m અને VHF/UHF બેન્ડ્સ',
        'warc': '30m, 17m અને 12m',
        'classic': '40m, 20m, 15m અને 10m'
    },
    'kn': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m ಮತ್ತು 10m',
        'future': '80m, 160m, 60m ಮತ್ತು VHF/UHF ಬ್ಯಾಂಡ್‌ಗಳು',
        'warc': '30m, 17m ಮತ್ತು 12m',
        'classic': '40m, 20m, 15m ಮತ್ತು 10m'
    },
    'ml': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m, 10m',
        'future': '80m, 160m, 60m, VHF/UHF ബാൻഡുകൾ',
        'warc': '30m, 17m, 12m',
        'classic': '40m, 20m, 15m, 10m'
    },
    'pa': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m ਅਤੇ 10m',
        'future': '80m, 160m, 60m ਅਤੇ VHF/UHF ਬੈਂਡ',
        'warc': '30m, 17m ਅਤੇ 12m',
        'classic': '40m, 20m, 15m ਅਤੇ 10m'
    },
    'or': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m ଏବଂ 10m',
        'future': '80m, 160m, 60m ଏବଂ VHF/UHF ବ୍ୟାଣ୍ଡ',
        'warc': '30m, 17m ଏବଂ 12m',
        'classic': '40m, 20m, 15m ଏବଂ 10m'
    },
    'my': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m နှင့် 10m',
        'future': '80m, 160m, 60m နှင့် VHF/UHF လှိုင်းခွင်များ',
        'warc': '30m, 17m နှင့် 12m',
        'classic': '40m, 20m, 15m နှင့် 10m'
    },
    'tl': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m, at 10m',
        'future': '80m, 160m, 60m, at mga VHF/UHF band',
        'warc': '30m, 17m, at 12m',
        'classic': '40m, 20m, 15m, at 10m'
    },
    'jv': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m, lan 10m',
        'future': '80m, 160m, 60m, lan pita VHF/UHF',
        'warc': '30m, 17m, lan 12m',
        'classic': '40m, 20m, 15m, lan 10m'
    },
    'su': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m, sareng 10m',
        'future': '80m, 160m, 60m, sareng pita VHF/UHF',
        'warc': '30m, 17m, sareng 12m',
        'classic': '40m, 20m, 15m, sareng 10m'
    },
    'bho': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m आ 10m',
        'future': '80m, 160m, 60m आ VHF/UHF बैंड',
        'warc': '30m, 17m आ 12m',
        'classic': '40m, 20m, 15m आ 10m'
    },
    'pcm': {
        'bands_7': '40m, 30m, 20m, 17m, 15m, 12m, and 10m',
        'future': '80m, 160m, 60m, and VHF/UHF bands',
        'warc': '30m, 17m, and 12m',
        'classic': '40m, 20m, 15m, and 10m'
    }
}


def update_language_file(lang_code):
    file_path = os.path.join(LANG_DIR, f"lang{lang_code}.js")
    if not os.path.exists(file_path):
        return False

    cfg = LANG_CONFIGS.get(lang_code, LANG_CONFIGS['en'])
    b7 = cfg['bands_7']
    fut = cfg['future']
    warc = cfg['warc']
    classic = cfg['classic']

    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Pattern for 3-band list across all scripts
    pattern_3b = r'(?:30\s*متراً\s*و\s*15\s*متراً\s*و\s*10\s*أمتار|30\s*و\s*15\s*و\s*10\s*متر|30\s*متر[،,]\s*15\s*متر\s*و\s*10\s*متر|30\s*מטר[،,]\s*15\s*מטר\s*ו-10\s*מטר|30m[、,・\s]+15m[、,・\s\w]+10m|30м[、,・\s]+15м[、,・\s\w]+10м)'

    # Pattern for 6-band list across all scripts
    pattern_6b = r'(?:40\s*متراً\s*و\s*20\s*متراً\s*و\s*12\s*متراً\s*و\s*17\s*متراً\s*و\s*80\s*متراً\s*و\s*160\s*متراً|40\s*و\s*20\s*و\s*12\s*و\s*17\s*و\s*80\s*و\s*160\s*متر|40[،,]\s*20[،,]\s*12[،,]\s*17[،,]\s*80\s*و\s*160\s*متر|40\s*מטר[،,]\s*20\s*מטר[،,]\s*12\s*מטר[،,]\s*17\s*מטר[،,]\s*80\s*מטר\s*ו-160\s*מטר|40m[、,・\s]+20m[、,・\s]+12m[、,・\s]+17m[、,・\s]+80m[、,・\s\w]+160m|40м[、,・\s]+20м[、,・\s]+12м[、,・\s]+17м[、,・\s]+80м[、,・\s\w]+160м)'

    # 1. Update FREQUENCIES_BOX_TITLE
    def repl_box_title(m):
        full_line = m.group(0)
        return re.sub(pattern_3b, b7, full_line)

    content = re.sub(r'\"FREQUENCIES_BOX_TITLE\":\s*\"[^\"]+\"', repl_box_title, content)

    # 2. Update FREQUENCIES_BOX_P1
    def repl_box_p1(m):
        full_line = m.group(0)
        def repl_strong(sm):
            inner = sm.group(1)
            return f"<strong>{re.sub(pattern_3b, b7, inner)}</strong>"
        return re.sub(r'<strong>([^<]*?(?:30m|30м|30\s*متراً|30\s*و|30\s*متر)[^<]*?)</strong>', repl_strong, full_line)

    content = re.sub(r'\"FREQUENCIES_BOX_P1\":\s*\"[^\"]+\"', repl_box_p1, content)

    # 3. Update FREQUENCIES_BOX_P2
    def repl_box_p2(m):
        full_line = m.group(0)
        pattern_warc = r'<strong>(30m|30м|30\s*متراً|30\s*متر)</strong>'
        pattern_classic = r'<strong>(15m|15м|15\s*متراً|15\s*متر)</strong>.*?<strong>(10m|10м|10\s*أمتار|10\s*متر)</strong>'
        new_line = re.sub(pattern_warc, f"<strong>{warc}</strong>", full_line)
        new_line = re.sub(pattern_classic, f"<strong>{classic}</strong>", new_line)
        return new_line

    content = re.sub(r'\"FREQUENCIES_BOX_P2\":\s*\"[^\"]+\"', repl_box_p2, content)

    # 4. Update FREQUENCIES_FUTURE_BANDS_NOTE
    def repl_note(m):
        full_line = m.group(0)
        # Update future list
        new_line = re.sub(pattern_6b, fut, full_line)
        # Update active bands list if present
        new_line = re.sub(pattern_3b, b7, new_line)
        return new_line

    content = re.sub(r'\"FREQUENCIES_FUTURE_BANDS_NOTE\":\s*\"[^\"]+\"', repl_note, content)

    with open(file_path, "w", encoding="utf-8") as f:
        f.write(content)

    return True


def main():
    print(f"Updating frequency translations in {LANG_DIR}...")
    count = 0
    for lang in LANG_CONFIGS.keys():
        if update_language_file(lang):
            count += 1
            print(f"  ✓ Updated lang{lang}.js")
        else:
            print(f"  ✗ Failed lang{lang}.js")
    print(f"\nCompleted {count} files successfully.")


if __name__ == "__main__":
    main()
