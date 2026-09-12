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
update_pileup_translations.py

Updates HERO_TITLE_2, PITCH_CARD_2_*, SPEEDUP_PL_*, META_DESC, OG_DESC,
TWITTER_DESC, and HERO_SUBTITLE across all 63 non-English language files
in docs/languages/lang*.js to reflect:
"4× Pileup Speed with Single-Carrier (240 vs 60 QSOs/h)",
the 2 QSOs every 30s pipelined overlap, and FT8 multi-stream power penalties.
"""

import json
import os
import re
import sys

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DOCS_DIR = os.path.join(BASE_DIR, "docs")
LANG_DIR = os.path.join(DOCS_DIR, "languages")

# Dictionary of localized strings for all 63 non-English languages
TRANSLATIONS = {
    'es': {
        'hero_title_2': "4× de velocidad en pileup con portadora única (240 frente a 60 QSOs/h)",
        'badge': "4× de velocidad en pileup",
        'stat_sub': "Portadora única (240 vs 60/h)",
        'desc': "En pileups continuos, la ranura 3 se solapa con nuevos corresponsales en frecuencias divididas, completando <strong>2 QSOs cada 30 segundos (240 QSOs/h, 4× frente a FT8 monoportadora)</strong> en una sola portadora con 0.0 dB de penalización (frente a 60–120 QSOs/h en FT8 Fox &amp; Hound con división de potencia), ¡escalando a <strong>480 QSOs/h</strong> con doble portadora!",
        'li_1': "240 QSOs/h (1 portadora, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 portadoras, 100 Hz)",
        'ft8_summary': "<strong>Intercambio Dirigido:</strong> 105 segundos (7 ranuras para 2 QSOs) &mdash; ~69 QSOs/h. En pileup continuo donde el mensaje 7 se solapa con nuevas llamadas, el ciclo es de 90s (6 ranuras) = 80 QSOs/h (hasta 120 QSOs/h con entrelazado de 2 flujos; 60 QSOs/h con portadora única). La paralelización multiflujo en Fox &amp; Hound incurre en severas pérdidas por división de potencia (&minus;3.0 a &minus;7.0 dB) y reducción de linealidad (back-off), comprometiendo el alcance de señales débiles.",
        'lq8_summary': "<strong>Intercambio Dirigido:</strong> 45 segundos (3 ranuras para 2 QSOs) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">En pileup continuo donde el mensaje 3 se solapa con nuevas llamadas: 2 QSOs cada 30 segundos = 240 QSOs/h (4× frente a FT8 monoportadora) con 0.0 dB de penalización de potencia!</span>",
        'meta_speed': "4× de velocidad en pileup con portadora única (240 frente a 60 QSOs/h)"
    },
    'pt': {
        'hero_title_2': "4× de velocidade em pileup com portadora única (240 vs 60 QSOs/h)",
        'badge': "4× de velocidade em pileup",
        'stat_sub': "Portadora única (240 vs 60/h)",
        'desc': "Em pileups contínuos, a janela 3 sobrepõe-se a novos chamadores em frequências divididas, completando <strong>2 QSOs a cada 30 segundos (240 QSOs/h, 4× sobre FT8 de portadora única)</strong> em uma única portadora com 0.0 dB de penalidade (vs 60–120 QSOs/h no FT8 Fox &amp; Hound com divisão de potência), escalando para <strong>480 QSOs/h</strong> com portadora dupla!",
        'li_1': "240 QSOs/h (1 portadora, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 portadoras, 100 Hz)",
        'ft8_summary': "<strong>Intercâmbio Dirigido:</strong> 105 segundos (7 janelas para 2 QSOs) &mdash; ~69 QSOs/h. Em pileup contínuo onde a mensagem 7 se sobrepõe a novas chamadas, o ciclo é de 90s (6 janelas) = 80 QSOs/h (até 120 QSOs/h com intercalação de 2 fluxos; 60 QSOs/h em portadora única). A paralelização multifluxo em Fox &amp; Hound acarreta penalidades severas de divisão de potência (&minus;3.0 a &minus;7.0 dB) e back-off do amplificador, comprometendo o alcance de sinais fracos.",
        'lq8_summary': "<strong>Intercâmbio Dirigido:</strong> 45 segundos (3 janelas para 2 QSOs) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">Em pileup contínuo onde a mensagem 3 se sobrepõe a novas chamadas: 2 QSOs a cada 30 segundos = 240 QSOs/h (4× sobre FT8 de portadora única) com 0.0 dB de penalidade de potência!</span>",
        'meta_speed': "4× de velocidade em pileup com portadora única (240 vs 60 QSOs/h)"
    },
    'fr': {
        'hero_title_2': "Vitesse de pileup 4× en porteuse unique (240 vs 60 QSOs/h)",
        'badge': "Vitesse de pileup 4×",
        'stat_sub': "Porteuse unique (240 vs 60/h)",
        'desc': "En pileup continu, le créneau 3 chevauche les nouveaux correspondants sur fréquences séparées, finalisant <strong>2 QSOs toutes les 30 secondes (240 QSOs/h, 4× par rapport au FT8 monoporteuse)</strong> sur une seule porteuse sans perte de puissance (vs 60–120 QSOs/h en FT8 Fox &amp; Hound avec division de puissance), atteignant <strong>480 QSOs/h</strong> en double porteuse !",
        'li_1': "240 QSOs/h (1 porteuse, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 porteuses, 100 Hz)",
        'ft8_summary': "<strong>Échange dirigé :</strong> 105 secondes (7 créneaux pour 2 QSOs) &mdash; ~69 QSOs/h. En pileup continu où le message 7 chevauche les nouveaux appels, le cycle est de 90s (6 créneaux) = 80 QSOs/h (jusqu'à 120 QSOs/h en entrelacement 2 flux ; 60 QSOs/h en porteuse unique). La parallélisation multiflux en Fox &amp; Hound entraîne de lourdes pertes de puissance (&minus;3.0 à &minus;7.0 dB) et un recul de l'amplificateur, réduisant la portée des signaux faibles.",
        'lq8_summary': "<strong>Échange dirigé :</strong> 45 secondes (3 créneaux pour 2 QSOs) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">En pileup continu où le message 3 chevauche les nouveaux appels : 2 QSOs toutes les 30 secondes = 240 QSOs/h (4× par rapport au FT8 monoporteuse) avec 0.0 dB de perte de puissance !</span>",
        'meta_speed': "vitesse de pileup 4× en porteuse unique (240 vs 60 QSOs/h)"
    },
    'it': {
        'hero_title_2': "4× di velocità in pileup a portante singola (240 vs 60 QSO/h)",
        'badge': "4× di velocità in pileup",
        'stat_sub': "Portante singola (240 vs 60/h)",
        'desc': "Nei pileup continui, l'intervallo 3 si sovrappone ai nuovi chiamanti su frequenze suddivise, completando <strong>2 QSO ogni 30 secondi (240 QSO/h, 4× rispetto a FT8 a portante singola)</strong> su una singola portante con 0.0 dB di penalità (rispetto a 60–120 QSO/h in FT8 Fox &amp; Hound con divisione di potenza), fino a <strong>480 QSO/h</strong> a doppia portante!",
        'li_1': "240 QSO/h (1 portante, 50 Hz, 0 dB)",
        'li_2': "480 QSO/h (2 portanti, 100 Hz)",
        'ft8_summary': "<strong>Scambio Diretto:</strong> 105 secondi (7 intervalli per 2 QSO) &mdash; ~69 QSO/h. Nel pileup continuo in cui il messaggio 7 si sovrappone alle nuove chiamate, il ciclo è di 90s (6 intervalli) = 80 QSO/h (fino a 120 QSO/h con interleaving a 2 flussi; 60 QSO/h a portante singola). La parallelizzazione multiflusso in Fox &amp; Hound comporta pesanti penalità di divisione della potenza (&minus;3.0 a &minus;7.0 dB) e back-off dell'amplificatore, compromettendo la portata dei segnali deboli.",
        'lq8_summary': "<strong>Scambio Diretto:</strong> 45 secondi (3 intervalli per 2 QSO) &mdash; 160 QSO/h. <span style=\"color: var(--emerald); font-weight: 700;\">Nel pileup continuo in cui il messaggio 3 si sovrappone alle nuove chiamate: 2 QSO ogni 30 secondi = 240 QSO/h (4× rispetto a FT8 a portante singola) con 0.0 dB di penalità di potenza!</span>",
        'meta_speed': "4× di velocità in pileup a portante singola (240 vs 60 QSO/h)"
    },
    'de': {
        'hero_title_2': "4× Pileup-Geschwindigkeit auf Einzelträger (240 vs. 60 QSOs/h)",
        'badge': "4× Pileup-Durchsatz",
        'stat_sub': "Einzelträger (240 vs. 60/h)",
        'desc': "Bei kontinuierlichen Pileups überlappt Schlitz 3 mit neuen Anrufern auf Split-Frequenzen und schließt <strong>2 QSOs alle 30 Sekunden ab (240 QSOs/h, 4× gegenüber FT8-Einzelträger)</strong> auf einem einzelnen Träger mit 0.0 dB Leistungsverlust (gegenüber 60–120 QSOs/h in FT8 Fox &amp; Hound mit Leistungsteilung), skalierbar auf <strong>480 QSOs/h</strong> mit zwei Trägern!",
        'li_1': "240 QSOs/h (1 Träger, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 Träger, 100 Hz)",
        'ft8_summary': "<strong>Gezielter Austausch:</strong> 105 Sekunden (7 Schlitze für 2 QSOs) &mdash; ~69 QSOs/h. Im kontinuierlichen Pileup, bei dem Nachricht 7 mit eingehenden Rufen überlappt, beträgt der Zyklus 90s (6 Schlitze) = 80 QSOs/h (bis zu 120 QSOs/h mit 2-Stream-Verschachtelung; 60 QSOs/h bei Einzelträger). Die Multi-Stream-Parallelisierung in Fox &amp; Hound verursacht erhebliche Leistungsteilungseinbußen (&minus;3.0 bis &minus;7.0 dB) sowie Verstärker-Back-off, was die Reichweite bei schwachen Signalen beeinträchtigt.",
        'lq8_summary': "<strong>Gezielter Austausch:</strong> 45 Sekunden (3 Schlitze für 2 QSOs) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">Im kontinuierlichen Pileup, bei dem Nachricht 3 mit neuen Rufen überlappt: 2 QSOs alle 30 Sekunden = 240 QSOs/h (4× gegenüber FT8-Einzelträger) mit 0.0 dB Leistungsverlust!</span>",
        'meta_speed': "4× Pileup-Geschwindigkeit auf Einzelträger (240 vs. 60 QSOs/h)"
    },
    'ru': {
        'hero_title_2': "4× скорость в пайлапах на одной несущей (240 против 60 QSO/ч)",
        'badge': "4× скорость в пайлапе",
        'stat_sub': "Одна несущая (240 против 60/ч)",
        'desc': "В непрерывных пайлапах слот 3 перекрывается с новыми вызывающими станциями на сплит-частотах, завершая <strong>2 QSO каждые 30 секунд (240 QSO/ч, в 4 раза быстрее FT8 на одной несущей)</strong> без потери мощности 0.0 дБ (по сравнению с 60–120 QSO/ч в FT8 Fox &amp; Hound с делением мощности), масштабируясь до <strong>480 QSO/ч</strong> на двух несущих!",
        'li_1': "240 QSO/ч (1 несущая, 50 Гц, 0 дБ)",
        'li_2': "480 QSO/ч (2 несущие, 100 Гц)",
        'ft8_summary': "<strong>Направленный обмен:</strong> 105 секунд (7 слотов на 2 QSO) &mdash; ~69 QSO/ч. В непрерывном пайлапе при перекрытии сообщения 7 с новыми вызовами период составляет 90 с (6 слотов) = 80 QSO/ч (до 120 QSO/ч при 2-потоковом чередовании; 60 QSO/ч на одной несущей). Многопоточная параллелизация в Fox &amp; Hound влечет значительные потери мощности из-за деления (&minus;3.0...&minus;7.0 дБ) и требует снижения мощности усилителя (back-off), ухудшая прием слабых сигналов.",
        'lq8_summary': "<strong>Направленный обмен:</strong> 45 секунд (3 слота на 2 QSO) &mdash; 160 QSO/ч. <span style=\"color: var(--emerald); font-weight: 700;\">В непрерывном пайлапе при перекрытии сообщения 3 с новыми вызовами: 2 QSO каждые 30 секунд = 240 QSO/ч (в 4 раза быстрее FT8 на одной несущей) без потери мощности 0.0 дБ!</span>",
        'meta_speed': "4× скорость в пайлапах на одной несущей (240 против 60 QSO/ч)"
    },
    'zh': {
        'hero_title_2': "单载波 4× 堆叠通联速度 (240 vs 60 QSOs/h)",
        'badge': "4× 堆叠速度",
        'stat_sub': "单载波 (240 vs 60/h)",
        'desc': "在连续堆叠通联中，第3时隙与分频呼叫的新电台重叠，在单载波上以 0.0 dB 功率损耗实现<strong>每30秒完成2个QSO (240 QSOs/h，是单载波 FT8 的4倍)</strong>（对比 FT8 Fox &amp; Hound 严重功率分流下的 60–120 QSOs/h），双载波更可扩展至 <strong>480 QSOs/h</strong>！",
        'li_1': "240 QSOs/h (单载波, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (双载波, 100 Hz)",
        'ft8_summary': "<strong>定向交换：</strong>105 秒（7 个时隙完成 2 个 QSO）&mdash; 约 69 QSOs/h。在消息 7 与新呼叫重叠的连续堆叠中，周期为 90 秒（6 个时隙）= 80 QSOs/h（双流交织下最高 120 QSOs/h；单载波为 60 QSOs/h）。Fox &amp; Hound 的多流并行会导致严重的功率分流损耗（&minus;3.0 至 &minus;7.0 dB）并需要功放回退，大幅削弱弱信号传播。",
        'lq8_summary': "<strong>定向交换：</strong>45 秒（3 个时隙完成 2 个 QSO）&mdash; 160 QSOs/h。<span style=\"color: var(--emerald); font-weight: 700;\">在消息 3 与新呼叫重叠的连续堆叠中：每 30 秒完成 2 个 QSO = 240 QSOs/h（单载波 FT8 的 4 倍），具有 0.0 dB 功率损耗！</span>",
        'meta_speed': "单载波 4× 堆叠速度 (240 vs 60 QSOs/h)"
    },
    'ja': {
        'hero_title_2': "単一キャリアで4倍のパイルアップ速度 (240 vs 60 QSOs/h)",
        'badge': "4× パイルアップ速度",
        'stat_sub': "単一キャリア (240 vs 60/h)",
        'desc': "連続パイルアップ運用ではスロット3がスプリット周波数での新規呼出と重なり、単一キャリアで0.0 dBの電力損失なしに<strong>30秒ごとに2 QSO (240 QSOs/h、単一キャリアFT8の4倍)</strong>を完了（電力分割によるFT8 Fox &amp; Houndの60〜120 QSOs/hに対し）、2キャリアで<strong>480 QSOs/h</strong>へ拡張可能です！",
        'li_1': "240 QSOs/h (1キャリア, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2キャリア, 100 Hz)",
        'ft8_summary': "<strong>直接交換：</strong>105秒（2 QSOあたり7スロット）&mdash; 約69 QSOs/h。メッセージ7が新規呼出と重なる連続パイルアップでは周期90秒（6スロット）= 80 QSOs/h（2ストリーム連動で最大120 QSOs/h、単一キャリアでは60 QSOs/h）。Fox &amp; Houndのマルチストリーム並列化は深刻な電力分割損失（&minus;3.0〜&minus;7.0 dB）とアンプのバックオフを招き、微弱信号の到達性を低下させます。",
        'lq8_summary': "<strong>直接交換：</strong>45秒（2 QSOあたり3スロット）&mdash; 160 QSOs/h。<span style=\"color: var(--emerald); font-weight: 700;\">メッセージ3が新規呼出と重なる連続パイルアップでは：30秒ごとに2 QSO = 240 QSOs/h（単一キャリアFT8の4倍）を0.0 dBの電力損失なしで達成！</span>",
        'meta_speed': "単一キャリアで4倍のパイルアップ速度 (240 vs 60 QSOs/h)"
    },
    'ko': {
        'hero_title_2': "단일 반송파 4× 파일업 속도 (240 vs 60 QSOs/h)",
        'badge': "4× 파일업 속도",
        'stat_sub': "단일 반송파 (240 vs 60/h)",
        'desc': "연속 파일업 환경에서 슬롯 3은 분할 주파수의 새 호출자와 중첩되어 단일 반송파에서 0.0 dB 전력 손실 없이 <strong>30초마다 2 QSO (240 QSOs/h, 단일 반송파 FT8 대비 4배)</strong>를 완료하며(전력 분할이 있는 FT8 Fox &amp; Hound의 60–120 QSOs/h 대비), 듀얼 반송파 시 <strong>480 QSOs/h</strong>까지 확장됩니다!",
        'li_1': "240 QSOs/h (1 반송파, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 반송파, 100 Hz)",
        'ft8_summary': "<strong>직접 교환:</strong> 105초 (2 QSO당 7개 슬롯) &mdash; 약 69 QSOs/h. 메시지 7이 새 호출과 중첩되는 연속 파일업의 주기는 90초 (6개 슬롯) = 80 QSOs/h (2스트림 인터리빙 시 최대 120 QSOs/h, 단일 반송파 60 QSOs/h). Fox &amp; Hound의 다중 스트림 병렬화는 심각한 전력 분할 손실(&minus;3.0 ~ &minus;7.0 dB)과 증폭기 백오프를 초래하여 미약 신호 도달 범위를 저하시킵니다.",
        'lq8_summary': "<strong>직접 교환:</strong> 45초 (2 QSO당 3개 슬롯) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">메시지 3이 새 호출과 중첩되는 연속 파일업: 30초마다 2 QSO = 240 QSOs/h (단일 반송파 FT8 대비 4배), 0.0 dB 전력 손실!</span>",
        'meta_speed': "단일 반송파 4× 파일업 속도 (240 vs 60 QSOs/h)"
    },
    'ar': {
        'hero_title_2': "سرعة بايل أب 4× بحامل فردي (240 مقابل 60 QSO/ساعة)",
        'badge': "سرعة بايل أب 4×",
        'stat_sub': "حامل فردي (240 مقابل 60/ساعة)",
        'desc': "في عمليات البايل أب المستمرة، تتداخل الفترة 3 مع المتصلين الجدد عبر الترددات المقسمة لتنجز <strong>اتصالين كل 30 ثانية (240 QSO/ساعة، 4× مقارنة بـ FT8 بحامل فردي)</strong> على حامل إشارة واحد دون أي فقد في القدرة (مقابل 60–120 QSO/ساعة في FT8 Fox &amp; Hound مع تجزئة القدرة)، وتصل إلى <strong>480 QSO/ساعة</strong> بحاملين!",
        'li_1': "240 QSO/ساعة (حامل واحد، 50 هرتز، 0 ديسيبل)",
        'li_2': "480 QSO/ساعة (حاملان، 100 هرتز)",
        'ft8_summary': "<strong>التبادل المباشر:</strong> 105 ثانية (7 فترات لاتصالين) &mdash; ~69 QSO/ساعة. في البايل أب المستمر حيث تتداخل الرسالة 7 مع المكالمات الجديدة، تصبح الدورة 90 ثانية (6 فترات) = 80 QSO/ساعة (تصل إلى 120 QSO/ساعة بتداخل مسارين؛ 60 QSO/ساعة بحامل فردي). التوازي متعدد المسارات في Fox &amp; Hound يتكبد خسائر فادحة في تجزئة القدرة (&minus;3.0 إلى &minus;7.0 ديسيبل) وخفض طاقة المضخم، مما يعيق انتشار الإشارات الضعيفة.",
        'lq8_summary': "<strong>التبادل المباشر:</strong> 45 ثانية (3 فترات لاتصالين) &mdash; 160 QSO/ساعة. <span style=\"color: var(--emerald); font-weight: 700;\">في البايل أب المستمر حيث تتداخل الرسالة 3 مع المكالمات الجديدة: اتصالان كل 30 ثانية = 240 QSO/ساعة (4× مقارنة بـ FT8 بحامل فردي) دون أي فقد في القدرة!</span>",
        'meta_speed': "سرعة بايل أب 4× بحامل فردي (240 مقابل 60 QSO/ساعة)"
    },
    'hi': {
        'hero_title_2': "सिंगल-कैरियर के साथ 4× पाइलअप गति (240 बनाम 60 QSOs/h)",
        'badge': "4× पाइलअप गति",
        'stat_sub': "सिंगल-कैरियर (240 बनाम 60/h)",
        'desc': "निरंतर पाइलअप में, स्लॉट 3 स्प्लिट आवृत्तियों पर नए कॉलर्स के साथ ओवरलैप होता है, जिससे 0.0 dB पावर पेनल्टी के साथ सिंगल कैरियर पर <strong>हर 30 सेकंड में 2 QSO (240 QSOs/h, सिंगल-कैरियर FT8 से 4×)</strong> पूरे होते हैं (FT8 Fox &amp; Hound में पावर स्प्लिट के साथ 60–120 QSOs/h के मुकाबले), जो डुअल-कैरियर में <strong>480 QSOs/h</strong> तक पहुँचता है!",
        'li_1': "240 QSOs/h (1 कैरियर, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 कैरियर, 100 Hz)",
        'ft8_summary': "<strong>निर्देशित विनिमय:</strong> 105 सेकंड (2 QSO के लिए 7 स्लॉट) &mdash; ~69 QSOs/h। निरंतर पाइलअप में जहाँ संदेश 7 नए कॉल के साथ ओवरलैप होता है, चक्र 90s (6 स्लॉट) = 80 QSOs/h होता है (2-स्ट्रीम इंटरलीविंग के साथ 120 QSOs/h तक; सिंगल कैरियर 60 QSOs/h)। Fox &amp; Hound में मल्टी-स्ट्रीम पैरेललाइजेशन से गंभीर पावर-स्प्लिट पेनल्टी (&minus;3.0 से &minus;7.0 dB) और एम्पलीफायर बैक-ऑफ होता है, जिससे कमजोर सिग्नल पहुँच प्रभावित होती है।",
        'lq8_summary': "<strong>निर्देशित विनिमय:</strong> 45 सेकंड (2 QSO के लिए 3 स्लॉट) &mdash; 160 QSOs/h। <span style=\"color: var(--emerald); font-weight: 700;\">निरंतर पाइलअप में जहाँ संदेश 3 नए कॉल के साथ ओवरलैप होता है: हर 30 सेकंड में 2 QSO = 240 QSOs/h (सिंगल-कैरियर FT8 से 4×) 0.0 dB पावर पेनल्टी के साथ!</span>",
        'meta_speed': "सिंगल-कैरियर के साथ 4× पाइलअप गति (240 बनाम 60 QSOs/h)"
    },
    'bn': {
        'hero_title_2': "একক ক্যারিয়ারে 4× পাইলআপ গতি (240 বনাম 60 QSOs/h)",
        'badge': "4× পাইলআপ গতি",
        'stat_sub': "একক ক্যারিয়ার (240 বনাম 60/h)",
        'desc': "একটানা পাইলআপে, স্লট 3 স্প্লিট ফ্রিকোয়েন্সিতে নতুন কলারদের সাথে ওভারল্যাপ করে, একক ক্যারিয়ারে 0.0 dB পাওয়ার পেনাল্টি সহ <strong>প্রতি 30 সেকেন্ডে 2টি QSO (240 QSOs/h, একক-ক্যারিয়ার FT8 এর চেয়ে 4×)</strong> সম্পন্ন করে (FT8 Fox &amp; Hound-এ পাওয়ার স্প্লিট সহ 60–120 QSOs/h এর বিপরীতে), যা ডুয়াল-ক্যারিয়ারে <strong>480 QSOs/h</strong> পর্যন্ত বৃদ্ধি পায়!",
        'li_1': "240 QSOs/h (1 ক্যারিয়ার, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 ক্যারিয়ার, 100 Hz)",
        'ft8_summary': "<strong>নির্দেশিত বিনিময়:</strong> 105 সেকেন্ড (2টি QSO-র জন্য 7টি স্লট) &mdash; ~69 QSOs/h। একটানা পাইলআপে যেখানে বার্তা 7 নতুন কলের সাথে ওভারল্যাপ করে, চক্রটি 90s (6টি স্লট) = 80 QSOs/h হয় (2-স্ট্রিম ইন্টারলিভিং সহ 120 QSOs/h পর্যন্ত; একক ক্যারিয়ারে 60 QSOs/h)। Fox &amp; Hound-এ মাল্টি-স্ট্রিম সমান্তরালকরণে মারাত্মক পাওয়ার-স্প্লিটিং জরিমানা (&minus;3.0 থেকে &minus;7.0 dB) এবং অ্যামপ্লিফায়ার ব্যাক-অফ ঘটে, যা দুর্বল সংকেতের বিস্তারকে ক্ষতিগ্রস্ত করে।",
        'lq8_summary': "<strong>নির্দেশিত বিনিময়:</strong> 45 সেকেন্ড (2টি QSO-র জন্য 3টি স্লট) &mdash; 160 QSOs/h। <span style=\"color: var(--emerald); font-weight: 700;\">একটানা পাইলআপে যেখানে বার্তা 3 নতুন কলের সাথে ওভারল্যাপ করে: প্রতি 30 সেকেন্ডে 2টি QSO = 240 QSOs/h (একক-ক্যারিয়ার FT8 এর চেয়ে 4×) 0.0 dB পাওয়ার পেনাল্টি সহ!</span>",
        'meta_speed': "একক ক্যারিয়ারে 4× পাইলআপ গতি (240 বনাম 60 QSOs/h)"
    },
    'ur': {
        'hero_title_2': "سنگل کیریئر کے ساتھ 4× پائل اپ رفتار (240 بمقابلہ 60 QSOs/h)",
        'badge': "4× پائل اپ رفتار",
        'stat_sub': "سنگل کیریئر (240 بمقابلہ 60/h)",
        'desc': "مسلسل پائل اپ میں، سلاٹ 3 الگ فریکوئنسیوں پر نئے کالرز کے ساتھ اوورلیپ ہوتا ہے، جس سے 0.0 dB پاور پنالٹی کے ساتھ سنگل کیریئر پر <strong>ہر 30 سیکنڈ میں 2 QSOs (240 QSOs/h، سنگل کیریئر FT8 سے 4×)</strong> مکمل ہوتے ہیں (FT8 Fox &amp; Hound میں 60–120 QSOs/h کے مقابلے)، جو ڈوئل کیریئر میں <strong>480 QSOs/h</strong> تک پہنچ جاتا ہے!",
        'li_1': "240 QSOs/h (1 کیریئر، 50 Hz، 0 dB)",
        'li_2': "480 QSOs/h (2 کیریئرز، 100 Hz)",
        'ft8_summary': "<strong>براہ راست تبادلہ:</strong> 105 سیکنڈ (2 QSOs کے لیے 7 سلاٹس) &mdash; ~69 QSOs/h۔ مسلسل پائل اپ میں جہاں پیغام 7 نئی کالز کے ساتھ اوورلیپ ہوتا ہے، دورانیہ 90s (6 سلاٹس) = 80 QSOs/h ہوتا ہے (2 اسٹریمز کے ساتھ 120 QSOs/h تک؛ سنگل کیریئر میں 60 QSOs/h)۔ Fox &amp; Hound میں ملٹی اسٹریم پیرللائزیشن سے شدید پاور اسپلٹ نقصان (&minus;3.0 تا &minus;7.0 dB) اور ایمپلیفائر بیک آف ہوتا ہے، جو کمزور سگنلز کی رسائی کو متاثر کرتا ہے۔",
        'lq8_summary': "<strong>براہ راست تبادلہ:</strong> 45 سیکنڈ (2 QSOs کے لیے 3 سلاٹس) &mdash; 160 QSOs/h। <span style=\"color: var(--emerald); font-weight: 700;\">مسلسل پائل اپ میں جہاں پیغام 3 نئی کالز کے ساتھ اوورلیپ ہوتا ہے: ہر 30 سیکنڈ میں 2 QSOs = 240 QSOs/h (سنگل کیریئر FT8 سے 4×) بغیر کسی پاور نقصان کے!</span>",
        'meta_speed': "سنگل کیریئر کے ساتھ 4× پائل اپ رفتار (240 بمقابلہ 60 QSOs/h)"
    },
    'tr': {
        'hero_title_2': "Tek Taşıyıcı ile 4× Pileup Hızı (240 vs 60 QSOs/saat)",
        'badge': "4× Pileup Hızı",
        'stat_sub': "Tek Taşıyıcı (240 vs 60/s)",
        'desc': "Sürekli pileup operasyonunda 3. dilim split frekanslardaki yeni çağrılarla örtüşür; 0.0 dB güç kaybı olmadan tek bir taşıyıcıda <strong>her 30 saniyede 2 QSO (240 QSO/saat, tek taşıyıcılı FT8'e göre 4×)</strong> tamamlar (güç bölüşümlü FT8 Fox &amp; Hound'daki 60–120 QSO/saat'e kıyasla), çift taşıyıcıda <strong>480 QSO/saat</strong>e ulaşır!",
        'li_1': "240 QSO/saat (1 taşıyıcı, 50 Hz, 0 dB)",
        'li_2': "480 QSO/saat (2 taşıyıcı, 100 Hz)",
        'ft8_summary': "<strong>Yönlendirilmiş Değişim:</strong> 105 saniye (2 QSO için 7 dilim) &mdash; ~69 QSO/saat. Mesaj 7'nin yeni çağrılarla örtüştüğü sürekli pileup'ta döngü 90s (6 dilim) = 80 QSO/saat olur (2 akışlı ardışık düzende 120 QSO/saat'e kadar; tek taşıyıcıda 60 QSO/saat). Fox &amp; Hound'daki çoklu akış paralelleştirmesi ağır güç bölme kayıplarına (&minus;3.0 ila &minus;7.0 dB) ve amfi geri çekilmesine (back-off) yol açarak zayıf sinyal yayılımını olumsuz etkiler.",
        'lq8_summary': "<strong>Yönlendirilmiş Değişim:</strong> 45 saniye (2 QSO için 3 dilim) &mdash; 160 QSO/saat. <span style=\"color: var(--emerald); font-weight: 700;\">Mesaj 3'ün yeni çağrılarla örtüştüğü sürekli pileup'ta: her 30 saniyede 2 QSO = 240 QSO/saat (tek taşıyıcılı FT8'e göre 4×) 0.0 dB güç kaybı ile!</span>",
        'meta_speed': "tek taşıyıcı ile 4× pileup hızı (240 vs 60 QSOs/saat)"
    },
    'vi': {
        'hero_title_2': "Tốc độ pileup 4× với sóng mang đơn (240 so với 60 QSOs/giờ)",
        'badge': "Tốc độ pileup 4×",
        'stat_sub': "Sóng mang đơn (240 vs 60/h)",
        'desc': "Trong pileup liên tục, khe 3 gối lên các trạm gọi mới trên tần số chia tách, hoàn thành <strong>2 QSO mỗi 30 giây (240 QSO/giờ, gấp 4× so với FT8 sóng mang đơn)</strong> trên một sóng mang duy nhất với 0.0 dB suy hao công suất (so với 60–120 QSO/giờ trong FT8 Fox &amp; Hound bị chia công suất), mở rộng lên <strong>480 QSO/giờ</strong> với sóng mang kép!",
        'li_1': "240 QSO/giờ (1 sóng mang, 50 Hz, 0 dB)",
        'li_2': "480 QSO/giờ (2 sóng mang, 100 Hz)",
        'ft8_summary': "<strong>Trao đổi định hướng:</strong> 105 giây (7 khe cho 2 QSO) &mdash; ~69 QSO/giờ. Trong pileup liên tục khi tin nhắn 7 gối lên các cuộc gọi mới, chu kỳ là 90s (6 khe) = 80 QSO/giờ (lên đến 120 QSO/giờ với xen kẽ 2 luồng; 60 QSO/giờ sóng mang đơn). Việc song song hóa đa luồng trong Fox &amp; Hound gây ra tổn hao chia công suất nghiêm trọng (&minus;3.0 đến &minus;7.0 dB) và phải giảm công suất khuếch đại (back-off), làm giảm khả năng thu tín hiệu yếu.",
        'lq8_summary': "<strong>Trao đổi định hướng:</strong> 45 giây (3 khe cho 2 QSO) &mdash; 160 QSO/giờ. <span style=\"color: var(--emerald); font-weight: 700;\">Trong pileup liên tục khi tin nhắn 3 gối lên các cuộc gọi mới: 2 QSO mỗi 30 giây = 240 QSO/giờ (gấp 4× so với FT8 sóng mang đơn) với 0.0 dB suy hao công suất!</span>",
        'meta_speed': "tốc độ pileup 4× với sóng mang đơn (240 so với 60 QSOs/giờ)"
    },
    'id': {
        'hero_title_2': "Kecepatan Pileup 4× dengan Pembawa Tunggal (240 vs 60 QSOs/jam)",
        'badge': "Kecepatan Pileup 4×",
        'stat_sub': "Pembawa Tunggal (240 vs 60/jam)",
        'desc': "Dalam operasi pileup berkelanjutan, slot 3 tumpang tindih dengan pemanggil baru pada frekuensi split, menyelesaikan <strong>2 QSO setiap 30 detik (240 QSO/jam, 4× lipat FT8 pembawa tunggal)</strong> pada pembawa tunggal dengan penalti daya 0.0 dB (vs 60–120 QSO/jam di FT8 Fox &amp; Hound dengan pembagian daya), meningkat hingga <strong>480 QSO/jam</strong> dengan pembawa ganda!",
        'li_1': "240 QSO/jam (1 pembawa, 50 Hz, 0 dB)",
        'li_2': "480 QSO/jam (2 pembawa, 100 Hz)",
        'ft8_summary': "<strong>Pertukaran Terarah:</strong> 105 detik (7 slot untuk 2 QSO) &mdash; ~69 QSO/jam. Pada pileup berkelanjutan di mana pesan 7 tumpang tindih dengan panggilan baru, siklusnya adalah 90 detik (6 slot) = 80 QSO/jam (hingga 120 QSO/jam dengan interleaving 2 aliran; 60 QSO/jam pembawa tunggal). Paralelisasi multi-aliran di Fox &amp; Hound menimbulkan penalti pembagian daya yang parah (&minus;3.0 hingga &minus;7.0 dB) dan back-off amplifier, mengurangi jangkauan sinyal lemah.",
        'lq8_summary': "<strong>Pertukaran Terarah:</strong> 45 detik (3 slot untuk 2 QSO) &mdash; 160 QSO/jam. <span style=\"color: var(--emerald); font-weight: 700;\">Pada pileup berkelanjutan di mana pesan 3 tumpang tindih dengan panggilan baru: 2 QSO setiap 30 detik = 240 QSO/jam (4× lipat FT8 pembawa tunggal) dengan penalti daya 0.0 dB!</span>",
        'meta_speed': "kecepatan pileup 4× dengan pembawa tunggal (240 vs 60 QSOs/jam)"
    },
    'nl': {
        'hero_title_2': "4× Pileup-snelheid met enkele draaggolf (240 vs 60 QSOs/u)",
        'badge': "4× Pileup-snelheid",
        'stat_sub': "Enkele draaggolf (240 vs 60/u)",
        'desc': "Bij continue pileup overlapt slot 3 met nieuwe aanroepers op gesplitste frequenties, waardoor <strong>2 QSO's per 30 seconden (240 QSO's/u, 4× t.o.v. FT8 op enkele draaggolf)</strong> worden voltooid op een enkele draaggolf met 0.0 dB vermogensverlies (vs 60–120 QSO's/u in FT8 Fox &amp; Hound met vermogenssplitsing), opschalend naar <strong>480 QSO's/u</strong> met dubbele draaggolf!",
        'li_1': "240 QSO's/u (1 draaggolf, 50 Hz, 0 dB)",
        'li_2': "480 QSO's/u (2 draaggolven, 100 Hz)",
        'ft8_summary': "<strong>Gerichte uitwisseling:</strong> 105 seconden (7 slots voor 2 QSO's) &mdash; ~69 QSO's/u. In continue pileup waar bericht 7 overlapt met nieuwe oproepen, is de cyclus 90s (6 slots) = 80 QSO's/u (tot 120 QSO's/u met 2-stream interleaving; 60 QSO's/u op enkele draaggolf). Multi-stream parallellisatie in Fox &amp; Hound leidt tot aanzienlijk vermogensverlies door vermogenssplitsing (&minus;3.0 tot &minus;7.0 dB) en back-off van versterkers, wat het bereik van zwakke signalen aantast.",
        'lq8_summary': "<strong>Gerichte uitwisseling:</strong> 45 seconden (3 slots voor 2 QSO's) &mdash; 160 QSO's/u. <span style=\"color: var(--emerald); font-weight: 700;\">In continue pileup waar bericht 3 overlapt met nieuwe oproepen: 2 QSO's per 30 seconden = 240 QSO's/u (4× t.o.v. FT8 op enkele draaggolf) met 0.0 dB vermogensverlies!</span>",
        'meta_speed': "4× pileup-snelheid met enkele draaggolf (240 vs 60 QSOs/u)"
    },
    'pl': {
        'hero_title_2': "4× Prędkość w pileupie na pojedynczej fali nośnej (240 vs 60 QSOs/h)",
        'badge': "4× Prędkość w pileupie",
        'stat_sub': "Pojedyncza nośna (240 vs 60/h)",
        'desc': "W ciągłym pileupie szczelina 3 nakłada się na nowe wywołania na częstotliwościach split, kończąc <strong>2 łączności co 30 sekund (240 QSOs/h, 4× szybciej niż FT8 na pojedynczej nośnej)</strong> bez utraty mocy 0.0 dB (w porównaniu do 60–120 QSOs/h w FT8 Fox &amp; Hound z podziałem mocy), osiągając <strong>480 QSOs/h</strong> na dwóch nośnych!",
        'li_1': "240 QSOs/h (1 nośna, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 nośne, 100 Hz)",
        'ft8_summary': "<strong>Wymiana ukierunkowana:</strong> 105 sekund (7 szczelin na 2 łączności) &mdash; ~69 QSOs/h. W ciągłym pileupie, gdzie komunikat 7 nakłada się na nowe wywołania, cykl wynosi 90 s (6 szczelin) = 80 QSOs/h (do 120 QSOs/h przy przeplataniu 2 strumieni; 60 QSOs/h na pojedynczej nośnej). Równoległość wielostrumieniowa w Fox &amp; Hound powoduje dotkliwe straty podziału mocy (&minus;3.0 do &minus;7.0 dB) i redukcję mocy wzmacniacza (back-off), ograniczając zasięg słabych sygnałów.",
        'lq8_summary': "<strong>Wymiana ukierunkowana:</strong> 45 sekund (3 szczeliny na 2 łączności) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">W ciągłym pileupie, gdzie komunikat 3 nakłada się na nowe wywołania: 2 łączności co 30 sekund = 240 QSOs/h (4× szybciej niż FT8 na pojedynczej nośnej) z zerową utratą mocy 0.0 dB!</span>",
        'meta_speed': "4× prędkość w pileupie na pojedynczej nośnej (240 vs 60 QSOs/h)"
    },
    'sv': {
        'hero_title_2': "4× Pileup-hastighet med enkel bärvåg (240 vs 60 QSOs/h)",
        'badge': "4× Pileup-hastighet",
        'stat_sub': "Enkel bärvåg (240 vs 60/h)",
        'desc': "Vid kontinuerlig pileup överlappar tidslucka 3 med nya anropare på splitfrekvenser och slutför <strong>2 QSO:er var 30:e sekund (240 QSO:er/h, 4× snabbare än FT8 med enkel bärvåg)</strong> på en bärvåg med 0.0 dB effektförlust (mot 60–120 QSO:er/h i FT8 Fox &amp; Hound med effektdelning), vilket skalar till <strong>480 QSO:er/h</strong> med dubbla bärvågor!",
        'li_1': "240 QSO:er/h (1 bärvåg, 50 Hz, 0 dB)",
        'li_2': "480 QSO:er/h (2 bärvågor, 100 Hz)",
        'ft8_summary': "<strong>Riktat utbyte:</strong> 105 sekunder (7 tidsluckor för 2 QSO:er) &mdash; ~69 QSO:er/h. Vid kontinuerlig pileup där meddelande 7 överlappar med nya anrop är cykeln 90 s (6 tidsluckor) = 80 QSO:er/h (upp till 120 QSO:er/h med 2-strömsflätning; 60 QSO:er/h med enkel bärvåg). Flerströmsdrift i Fox &amp; Hound medför allvarliga effektdelningsförluster (&minus;3.0 till &minus;7.0 dB) och slutstegsback-off, vilket minskar räckvidden för svaga signaler.",
        'lq8_summary': "<strong>Riktat utbyte:</strong> 45 sekunder (3 tidsluckor för 2 QSO:er) &mdash; 160 QSO:er/h. <span style=\"color: var(--emerald); font-weight: 700;\">Vid kontinuerlig pileup där meddelande 3 överlappar med nya anrop: 2 QSO:er var 30:e sekund = 240 QSO:er/h (4× snabbare än FT8 med enkel bärvåg) med 0.0 dB effektförlust!</span>",
        'meta_speed': "4× pileup-hastighet med enkel bärvåg (240 vs 60 QSOs/h)"
    },
    'fi': {
        'hero_title_2': "4× Pileup-nopeus yhdellä kantoaallolla (240 vs 60 QSOs/h)",
        'badge': "4× Pileup-nopeus",
        'stat_sub': "Yksi kantoaalto (240 vs 60/h)",
        'desc': "Jatkuvassa pileupissa aikaväli 3 limittyy uusien kutsujien kanssa split-taajuuksilla saavuttaen <strong>2 QSO:ta 30 sekunnissa (240 QSO/h, 4× yhden kantoaallon FT8:aan verrattuna)</strong> yhdellä kantoaallolla 0.0 dB tehohäviöllä (verrattuna 60–120 QSO/h FT8 Fox &amp; Houndissa tehonjakautumisen vuoksi), skaalautuen <strong>480 QSO/h</strong> kahdella kantoaallolla!",
        'li_1': "240 QSO/h (1 kantoaalto, 50 Hz, 0 dB)",
        'li_2': "480 QSO/h (2 kantoaaltoa, 100 Hz)",
        'ft8_summary': "<strong>Kohdistettu vaihto:</strong> 105 sekuntia (7 aikaväliä 2 QSO:lle) &mdash; ~69 QSO/h. Jatkuvassa pileupissa, jossa viesti 7 limittyy uusien kutsujen kanssa, jakso on 90s (6 aikaväliä) = 80 QSO/h (jopa 120 QSO/h 2-virtaisella lomituksella; 60 QSO/h yhdellä kantoaallolla). Fox &amp; Houndin monivirtarinnakkaistus aiheuttaa merkittäviä tehonjakohäviöitä (&minus;3.0...&minus;7.0 dB) ja vaatii vahvistimen tehon alennusta (back-off), heikentäen heikkojen signaalien kantamaa.",
        'lq8_summary': "<strong>Kohdistettu vaihto:</strong> 45 sekuntia (3 aikaväliä 2 QSO:lle) &mdash; 160 QSO/h. <span style=\"color: var(--emerald); font-weight: 700;\">Jatkuvassa pileupissa, jossa viesti 3 limittyy uusien kutsujen kanssa: 2 QSO:ta 30 sekunnissa = 240 QSO/h (4× yhden kantoaallon FT8:aan verrattuna) 0.0 dB tehohäviöllä!</span>",
        'meta_speed': "4× pileup-nopeus yhdellä kantoaallolla (240 vs 60 QSOs/h)"
    },
    'da': {
        'hero_title_2': "4× Pileup-hastighed med enkelt bærebølge (240 vs 60 QSOs/t)",
        'badge': "4× Pileup-hastighed",
        'stat_sub': "Enkelt bærebølge (240 vs 60/t)",
        'desc': "Ved kontinuerlig pileup overlapper tidsvindue 3 med nye kaldere på split-frekvenser og fuldfører <strong>2 QSO'er hvert 30. sekund (240 QSO'er/t, 4× i forhold til enkeltbærebølge FT8)</strong> på en enkelt bærebølge med 0.0 dB effekttab (mod 60–120 QSO'er/t i FT8 Fox &amp; Hound med effektdeling), hvilket skalerer til <strong>480 QSO'er/t</strong> med dobbelt bærebølge!",
        'li_1': "240 QSO'er/t (1 bærebølge, 50 Hz, 0 dB)",
        'li_2': "480 QSO'er/t (2 bærebølger, 100 Hz)",
        'ft8_summary': "<strong>Målrettet udveksling:</strong> 105 sekunder (7 slots til 2 QSO'er) &mdash; ~69 QSO'er/t. I kontinuerlig pileup, hvor besked 7 overlapper med nye opkald, er cyklussen 90s (6 slots) = 80 QSO'er/t (op til 120 QSO'er/t med 2-stream sammenfletning; 60 QSO'er/t på enkelt bærebølge). Multistream-parallelisering i Fox &amp; Hound medfører store effektdelingstab (&minus;3.0 til &minus;7.0 dB) og forstærker-back-off, hvilket svækker rækkevidden for svage signaler.",
        'lq8_summary': "<strong>Målrettet udveksling:</strong> 45 sekunder (3 slots til 2 QSO'er) &mdash; 160 QSO'er/t. <span style=\"color: var(--emerald); font-weight: 700;\">I kontinuerlig pileup, hvor besked 3 overlapper med nye opkald: 2 QSO'er hvert 30. sekund = 240 QSO'er/t (4× i forhold til enkeltbærebølge FT8) med 0.0 dB effekttab!</span>",
        'meta_speed': "4× pileup-hastighed med enkelt bærebølge (240 vs 60 QSOs/t)"
    },
    'no': {
        'hero_title_2': "4× Pileup-hastighet med enkel bærebølge (240 vs 60 QSOs/t)",
        'badge': "4× Pileup-hastighet",
        'stat_sub': "Enkel bærebølge (240 vs 60/t)",
        'desc': "I kontinuerlig pileup overlapper tidsluke 3 med nye anropere på split-frekvenser og fullfører <strong>2 QSO-er hvert 30. sekund (240 QSO-er/t, 4× over enkeltbærebølge FT8)</strong> på en enkelt bærebølge med 0.0 dB effekttap (mot 60–120 QSO-er/t i FT8 Fox &amp; Hound med effektdeling), med oppskalering til <strong>480 QSO-er/t</strong> med dobbel bærebølge!",
        'li_1': "240 QSO-er/t (1 bærebølge, 50 Hz, 0 dB)",
        'li_2': "480 QSO-er/t (2 bærebølger, 100 Hz)",
        'ft8_summary': "<strong>Målrettet utveksling:</strong> 105 sekunder (7 tidsluker for 2 QSO-er) &mdash; ~69 QSO-er/t. I kontinuerlig pileup der melding 7 overlapper med nye anrop, er syklusen 90s (6 tidsluker) = 80 QSO-er/t (opptil 120 QSO-er/t med 2-strøms fletting; 60 QSO-er/t med enkel bærebølge). Flerstrømsdrift i Fox &amp; Hound medfører store effektdelingstap (&minus;3.0 til &minus;7.0 dB) og forsterker-back-off, noe som svekker rekkevidden for svake signaler.",
        'lq8_summary': "<strong>Målrettet utveksling:</strong> 45 sekunder (3 tidsluker for 2 QSO-er) &mdash; 160 QSO-er/t. <span style=\"color: var(--emerald); font-weight: 700;\">I kontinuerlig pileup der melding 3 overlapper med nye anrop: 2 QSO-er hvert 30. sekund = 240 QSO-er/t (4× over enkeltbærebølge FT8) med 0.0 dB effekttap!</span>",
        'meta_speed': "4× pileup-hastighet med enkel bærebølge (240 vs 60 QSOs/t)"
    },
    'cs': {
        'hero_title_2': "4× Rychlost pileupu na jedné nosné (240 vs 60 QSOs/h)",
        'badge': "4× Rychlost pileupu",
        'stat_sub': "Jedna nosná (240 vs 60/h)",
        'desc': "V nepřetržitém pileupu se slot 3 překrývá s novými volajícími na split frekvencích, čímž dokončí <strong>2 QSO každých 30 sekund (240 QSOs/h, 4× oproti jednonosnému FT8)</strong> na jedné nosné s 0.0 dB výkonovou ztrátou (oproti 60–120 QSOs/h v FT8 Fox &amp; Hound s rozdělením výkonu), škálující na <strong>480 QSOs/h</strong> se dvěma nosnými!",
        'li_1': "240 QSOs/h (1 nosná, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 nosné, 100 Hz)",
        'ft8_summary': "<strong>Cílená výměna:</strong> 105 sekund (7 slotů pro 2 QSO) &mdash; ~69 QSOs/h. V nepřetržitém pileupu, kde se zpráva 7 překrývá s novými voláními, je cyklus 90 s (6 slotů) = 80 QSOs/h (až 120 QSOs/h při prokládání 2 proudů; 60 QSOs/h na jedné nosné). Vícestroudová paralelizace v režimu Fox &amp; Hound způsobuje závažné ztráty rozdělením výkonu (&minus;3.0 až &minus;7.0 dB) a nutnost snížení výkonu zesilovače (back-off), což omezuje dosah slabých signálů.",
        'lq8_summary': "<strong>Cílená výměna:</strong> 45 sekund (3 sloty pro 2 QSO) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">V nepřetržitém pileupu, kde se zpráva 3 překrývá s novými voláními: 2 QSO každých 30 sekund = 240 QSOs/h (4× oproti jednonosnému FT8) s 0.0 dB výkonovou ztrátou!</span>",
        'meta_speed': "4× rychlost pileupu na jedné nosné (240 vs 60 QSOs/h)"
    },
    'sk': {
        'hero_title_2': "4× Rýchlosť pileupu na jednej nosnej (240 vs 60 QSOs/h)",
        'badge': "4× Rýchlosť pileupu",
        'stat_sub': "Jedna nosná (240 vs 60/h)",
        'desc': "V nepretržitom pileupe sa slot 3 prekrýva s novými volajúcimi na split frekvenciách, čím dokončí <strong>2 QSO každých 30 sekúnd (240 QSOs/h, 4× oproti jednonosnému FT8)</strong> na jednej nosnej s 0.0 dB výkonovou stratou (oproti 60–120 QSOs/h v FT8 Fox &amp; Hound s rozdelením výkonu), škálujúc na <strong>480 QSOs/h</strong> s dvoma nosnými!",
        'li_1': "240 QSOs/h (1 nosná, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 nosné, 100 Hz)",
        'ft8_summary': "<strong>Cielená výmena:</strong> 105 sekúnd (7 slotov pre 2 QSO) &mdash; ~69 QSOs/h. V nepretržitom pileupe, kde sa správa 7 prekrýva s novými volaniami, je cyklus 90 s (6 slotov) = 80 QSOs/h (až 120 QSOs/h pri prekladaní 2 prúdov; 60 QSOs/h na jednej nosnej). Viacprúdová paralelizácia v režime Fox &amp; Hound spôsobuje vážne straty rozdelením výkonu (&minus;3.0 až &minus;7.0 dB) a vyžaduje zníženie výkonu zosilňovača (back-off), čo znižuje dosah slabých signálov.",
        'lq8_summary': "<strong>Cielená výmena:</strong> 45 sekúnd (3 sloty pre 2 QSO) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">V nepretržitom pileupe, kde sa správa 3 prekrýva s novými volaniami: 2 QSO každých 30 sekúnd = 240 QSOs/h (4× oproti jednonosnému FT8) s 0.0 dB výkonovou stratou!</span>",
        'meta_speed': "4× rýchlosť pileupu na jednej nosnej (240 vs 60 QSOs/h)"
    },
    'hu': {
        'hero_title_2': "4× Felhalmozódási sebesség egyetlen vivővel (240 vs 60 QSOs/h)",
        'badge': "4× Felhalmozódási sebesség",
        'stat_sub': "Egyetlen vivő (240 vs 60/h)",
        'desc': "Folyamatos pileupban a 3. időszelet átfedésbe kerül az új hívókkal split frekvenciákon, így 0.0 dB teljesítményveszteség nélkül egyetlen vivőn <strong>2 QSO-t teljesít 30 másodpercenként (240 QSO/h, 4× az egyvivős FT8-hoz képest)</strong> (szemben az FT8 Fox &amp; Hound teljesítményosztásos 60–120 QSO/h értékével), kettős vivővel <strong>480 QSO/h</strong>-ra skálázódva!",
        'li_1': "240 QSO/h (1 vivő, 50 Hz, 0 dB)",
        'li_2': "480 QSO/h (2 vivő, 100 Hz)",
        'ft8_summary': "<strong>Irányított csere:</strong> 105 másodperc (7 időszelet 2 QSO-hoz) &mdash; ~69 QSO/h. Folyamatos pileupban, ahol a 7. üzenet átfedésben van az új hívásokkal, a ciklus 90s (6 időszelet) = 80 QSO/h (akár 120 QSO/h 2 adatfolyamos összefűzéssel; 60 QSO/h egyetlen vivővel). A Fox &amp; Hound többszálas párhuzamosítása súlyos teljesítményosztási veszteségekkel jár (&minus;3.0 ... &minus;7.0 dB) és erősítő visszavételt (back-off) igényel, ami lerontja a gyenge jelek terjedését.",
        'lq8_summary': "<strong>Irányított csere:</strong> 45 másodperc (3 időszelet 2 QSO-hoz) &mdash; 160 QSO/h. <span style=\"color: var(--emerald); font-weight: 700;\">Folyamatos pileupban, ahol a 3. üzenet átfedésben van az új hívásokkal: 2 QSO 30 másodpercenként = 240 QSO/h (4× az egyvivős FT8-hoz képest) 0.0 dB teljesítményveszteséggel!</span>",
        'meta_speed': "4× felhalmozódási sebesség egyetlen vivővel (240 vs 60 QSOs/h)"
    },
    'ro': {
        'hero_title_2': "Viteză pileup de 4× cu o singură purtătoare (240 vs 60 QSOs/h)",
        'badge': "Viteză pileup 4×",
        'stat_sub': "O singură purtătoare (240 vs 60/h)",
        'desc': "În pileup continuu, intervalul 3 se suprapune cu noii apelanți pe frecvențe split, realizând <strong>2 QSO-uri la fiecare 30 de secunde (240 QSO/h, 4× față de FT8 pe o singură purtătoare)</strong> pe o singură purtătoare cu 0.0 dB pierdere de putere (față de 60–120 QSO/h în FT8 Fox &amp; Hound cu diviziune de putere), scalând la <strong>480 QSO/h</strong> pe purtătoare dublă!",
        'li_1': "240 QSO/h (1 purtătoare, 50 Hz, 0 dB)",
        'li_2': "480 QSO/h (2 purtătoare, 100 Hz)",
        'ft8_summary': "<strong>Schimb direcționat:</strong> 105 secunde (7 intervale pentru 2 QSO-uri) &mdash; ~69 QSO/h. În pileup continuu în care mesajul 7 se suprapune cu apelurile noi, ciclul este de 90s (6 intervale) = 80 QSO/h (până la 120 QSO/h cu intercalare pe 2 fluxuri; 60 QSO/h pe purtătoare unică). Paralelizarea multi-flux în Fox &amp; Hound produce pierderi severe de diviziune a puterii (&minus;3.0 până la &minus;7.0 dB) și necesită reducerea liniarității amplificatorului (back-off), afectând propagarea semnalelor slabe.",
        'lq8_summary': "<strong>Schimb direcționat:</strong> 45 secunde (3 intervale pentru 2 QSO-uri) &mdash; 160 QSO/h. <span style=\"color: var(--emerald); font-weight: 700;\">În pileup continuu în care mesajul 3 se suprapune cu apelurile noi: 2 QSO-uri la fiecare 30 de secunde = 240 QSO/h (4× față de FT8 pe o singură purtătoare) cu 0.0 dB penalizare de putere!</span>",
        'meta_speed': "viteză pileup de 4× cu o singură purtătoare (240 vs 60 QSOs/h)"
    },
    'el': {
        'hero_title_2': "4× Ταχύτητα Pileup με Μονό Φέρον (240 έναντι 60 QSOs/h)",
        'badge': "4× Ταχύτητα Pileup",
        'stat_sub': "Μονό Φέρον (240 έναντι 60/h)",
        'desc': "Σε συνεχή pileup, η χρονοθυρίδα 3 επικαλύπτεται με νέους καλούντες σε split συχνότητες, ολοκληρώνοντας <strong>2 QSO κάθε 30 δευτερόλεπτα (240 QSO/h, 4× έναντι του μονού φέροντος FT8)</strong> σε ένα μόνο φέρον με 0.0 dB απώλεια ισχύος (έναντι 60–120 QSO/h στο FT8 Fox &amp; Hound με διαχωρισμό ισχύος), φτάνοντας τα <strong>480 QSO/h</strong> με διπλό φέρον!",
        'li_1': "240 QSO/h (1 φέρον, 50 Hz, 0 dB)",
        'li_2': "480 QSO/h (2 φέροντα, 100 Hz)",
        'ft8_summary': "<strong>Κατευθυνόμενη Ανταλλαγή:</strong> 105 δευτερόλεπτα (7 χρονοθυρίδες για 2 QSO) &mdash; ~69 QSO/h. Σε συνεχή λειτουργία όπου το μήνυμα 7 επικαλύπτεται με νέες κλήσεις, ο κύκλος είναι 90s (6 χρονοθυρίδες) = 80 QSO/h (έως 120 QSO/h με πλέξη 2 ροών, 60 QSO/h σε μονό φέρον). Ο πολυροϊκός παραλληλισμός στο Fox &amp; Hound επιφέρει σοβαρές απώλειες διαχωρισμού ισχύος (&minus;3.0 έως &minus;7.0 dB) και back-off ενισχυτή, μειώνοντας την εμβέλεια ασθενών σημάτων.",
        'lq8_summary': "<strong>Κατευθυνόμενη Ανταλλαγή:</strong> 45 δευτερόλεπτα (3 χρονοθυρίδες για 2 QSO) &mdash; 160 QSO/h. <span style=\"color: var(--emerald); font-weight: 700;\">Σε συνεχή λειτουργία όπου το μήνυμα 3 επικαλύπτεται με νέες κλήσεις: 2 QSO κάθε 30 δευτερόλεπτα = 240 QSO/h (4× έναντι FT8 μονού φέροντος) με 0.0 dB ποινή ισχύος!</span>",
        'meta_speed': "4× ταχύτητα pileup με μονό φέρον (240 έναντι 60 QSOs/h)"
    },
    'he': {
        'hero_title_2': "מהירות פיילאפ פי 4 עם גל נושא יחיד (240 לעומת 60 QSOs/h)",
        'badge': "מהירות פיילאפ פי 4",
        'stat_sub': "גל נושא יחיד (240 לעומת 60/שעה)",
        'desc': "בפיילאפ רציף, חריץ 3 חופף לתחנות קוראות חדשות בתדרים מפוצלים, ומשלים <strong>2 קשרי QSO כל 30 שניות (240 QSO/שעה, פי 4 מ-FT8 בעל גל נושא יחיד)</strong> על גל נושא יחיד ללא אובדן הספק של 0.0 dB (לעומת 60–120 ב-FT8 Fox &amp; Hound עם חלוקת הספק), ומגיע ל-<strong>480 QSO/שעה</strong> בגל נושא כפול!",
        'li_1': "240 QSO/שעה (גל נושא 1, 50 הרץ, 0 dB)",
        'li_2': "480 QSO/שעה (2 גלי נושא, 100 הרץ)",
        'ft8_summary': "<strong>החלפה ישירה:</strong> 105 שניות (7 חריצים עבור 2 קשרי QSO) &mdash; כ-69 QSO/שעה. בפיילאפ רציף שבו הודעה 7 חופפת לקריאות חדשות, המחזור הוא 90 שניות (6 חריצים) = 80 QSO/שעה (עד 120 QSO/שעה בשזירת 2 זרמים; 60 בגל נושא יחיד). הפעלה מרובת זרמים ב-Fox &amp; Hound גורמת להפסדי הספק ניכרים (&minus;3.0 עד &minus;7.0 dB) ול-back-off במגבר, דבר הפוגע בקליטת אותות חלשים.",
        'lq8_summary': "<strong>החלפה ישירה:</strong> 45 שניות (3 חריצים עבור 2 קשרי QSO) &mdash; 160 QSO/שעה. <span style=\"color: var(--emerald); font-weight: 700;\">בפיילאפ רציף שבו הודעה 3 חופפת לקריאות חדשות: 2 קשרי QSO כל 30 שניות = 240 QSO/שעה (פי 4 מ-FT8 בגל נושא יחיד) ללא הפסד הספק של 0.0 dB!</span>",
        'meta_speed': "מהירות פיילאפ פי 4 עם גל נושא יחיד (240 לעומת 60 QSOs/h)"
    },
    'th': {
        'hero_title_2': "ความเร็วไพลอัป 4× ด้วยคลื่นพาห์เดี่ยว (240 vs 60 QSOs/ชม.)",
        'badge': "ความเร็วไพลอัป 4×",
        'stat_sub': "คลื่นพาห์เดี่ยว (240 vs 60/ชม.)",
        'desc': "ในการทำงานไพลอัปต่อเนื่อง สล็อต 3 จะเหลื่อมซ้อนกับสถานีเรียกใหม่บนความถี่แยก ทำให้สำเร็จ <strong>2 QSO ทุกๆ 30 วินาที (240 QSOs/ชม., เร็วกว่า FT8 คลื่นพาห์เดี่ยว 4 เท่า)</strong> บนคลื่นพาห์เดี่ยวโดยไม่มีการสูญเสียกำลังส่ง 0.0 dB (เทียบกับ 60–120 QSOs/ชม. ใน FT8 Fox &amp; Hound ที่ต้องแบ่งกำลังส่ง) และเพิ่มเป็น <strong>480 QSOs/ชม.</strong> ด้วยคลื่นพาห์คู่!",
        'li_1': "240 QSOs/ชม. (1 คลื่นพาห์, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/ชม. (2 คลื่นพาห์, 100 Hz)",
        'ft8_summary': "<strong>การแลกเปลี่ยนโดยตรง:</strong> 105 วินาที (7 สล็อตสำหรับ 2 QSO) &mdash; ~69 QSOs/ชม. ในไพลอัปต่อเนื่องที่ข้อความ 7 เหลื่อมซ้อนกับการเรียกใหม่ รอบเวลาจะเหลือ 90 วินาที (6 สล็อต) = 80 QSOs/ชม. (สูงสุด 120 QSOs/ชม. ด้วยการสลับ 2 สตรีม; 60 QSOs/ชม. คลื่นพาห์เดี่ยว) การส่งหลายสตรีมใน Fox &amp; Hound ทำให้สูญเสียกำลังส่งอย่างมาก (&minus;3.0 ถึง &minus;7.0 dB) และต้องลดกำลังขยาย (back-off) ซึ่งส่งผลกระทบต่อสัญญาณที่อ่อน",
        'lq8_summary': "<strong>การแลกเปลี่ยนโดยตรง:</strong> 45 วินาที (3 สล็อตสำหรับ 2 QSO) &mdash; 160 QSOs/ชม. <span style=\"color: var(--emerald); font-weight: 700;\">ในไพลอัปต่อเนื่องที่ข้อความ 3 เหลื่อมซ้อนกับการเรียกใหม่: 2 QSO ทุก 30 วินาที = 240 QSOs/ชม. (เร็วกว่า FT8 คลื่นพาห์เดี่ยว 4 เท่า) โดยไม่มีการสูญเสียกำลังส่ง 0.0 dB!</span>",
        'meta_speed': "ความเร็วไพลอัป 4× ด้วยคลื่นพาห์เดี่ยว (240 vs 60 QSOs/ชม.)"
    },
    'ca': {
        'hero_title_2': "4× de velocitat en pileup amb portadora única (240 vs 60 QSOs/h)",
        'badge': "4× de velocitat en pileup",
        'stat_sub': "Portadora única (240 vs 60/h)",
        'desc': "En pileups continus, la ranura 3 se solapa amb nous corresponsals en freqüències dividides, completant <strong>2 QSOs cada 30 segons (240 QSOs/h, 4× respecte a FT8 monoportaora)</strong> en una sola portadora amb 0.0 dB de penalització (vs 60–120 QSOs/h en FT8 Fox &amp; Hound amb divisió de potència), escalant a <strong>480 QSOs/h</strong> amb doble portadora!",
        'li_1': "240 QSOs/h (1 portadora, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 portadores, 100 Hz)",
        'ft8_summary': "<strong>Intercanvi Dirigit:</strong> 105 segons (7 ranures per a 2 QSOs) &mdash; ~69 QSOs/h. En pileup continu on el missatge 7 se solapa amb noves trucades, el cicle és de 90s (6 ranures) = 80 QSOs/h (fins a 120 QSOs/h amb entrellaçat de 2 fluxos; 60 QSOs/h amb portadora única). La paral·lelització multiflux a Fox &amp; Hound comporta pèrdues greus per divisió de potència (&minus;3.0 a &minus;7.0 dB) i reducció de potència (back-off), perjudicant l'abast dels senyals febles.",
        'lq8_summary': "<strong>Intercanvi Dirigit:</strong> 45 segons (3 ranures per a 2 QSOs) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">En pileup continu on el missatge 3 se solapa amb noves trucades: 2 QSOs cada 30 segons = 240 QSOs/h (4× respecte a FT8 monoportadora) amb 0.0 dB de penalització de potència!</span>",
        'meta_speed': "4× de velocitat en pileup amb portadora única (240 vs 60 QSOs/h)"
    },
    'eu': {
        'hero_title_2': "Pileup-abiadura 4× eramaile bakarrarekin (240 vs 60 QSOs/h)",
        'badge': "Pileup-abiadura 4×",
        'stat_sub': "Eramaile bakarra (240 vs 60/h)",
        'desc': "Pileup jarraituetan, 3. zirrikitua maiztasun banatuetako deitzaile berriekin gainjartzen da, <strong>2 QSO 30 segundoro (240 QSO/h, FT8 eramaile bakarrean baino 4× azkarrago)</strong> osatuz eramaile bakarrean 0.0 dB potentzia-galerarik gabe (FT8 Fox &amp; Hound-eko 60–120 QSO/h-ren aldean), eramaile bikoitzarekin <strong>480 QSO/h</strong>-ra eskalatuz!",
        'li_1': "240 QSO/h (eramaile 1, 50 Hz, 0 dB)",
        'li_2': "480 QSO/h (2 eramaile, 100 Hz)",
        'ft8_summary': "<strong>Truke Zuzendua:</strong> 105 segundo (7 zirrikitu 2 QSOtarako) &mdash; ~69 QSO/h. Pileup jarraituan 7. mezua dei berriekin gainjartzen denean, zikloa 90s (6 zirrikitu) = 80 QSO/h da (120 QSO/h-ra arte 2 korronteko tartekatzearekin; 60 QSO/h eramaile bakarrean). Fox &amp; Hound-eko fluxu anitzeko paralelotasunak potentzia-banaketa galera larriak eragiten ditu (&minus;3.0tik &minus;7.0 dBra) eta anplifikadorearen atzerapena behar du, seinale ahulen irismena kaltetuz.",
        'lq8_summary': "<strong>Truke Zuzendua:</strong> 45 segundo (3 zirrikitu 2 QSOtarako) &mdash; 160 QSO/h. <span style=\"color: var(--emerald); font-weight: 700;\">Pileup jarraituan 3. mezua dei berriekin gainjartzen denean: 2 QSO 30 segundoro = 240 QSO/h (FT8 eramaile bakarrean baino 4× azkarrago) 0.0 dB potentzia-galerarekin!</span>",
        'meta_speed': "pileup-abiadura 4× eramaile bakarrarekin (240 vs 60 QSOs/h)"
    },
    'gl': {
        'hero_title_2': "4× de velocidade en pileup con portadora única (240 vs 60 QSOs/h)",
        'badge': "4× de velocidade en pileup",
        'stat_sub': "Portadora única (240 vs 60/h)",
        'desc': "En pileups continuos, a rañura 3 solápase con novos correspondentes en frecuencias divididas, completando <strong>2 QSOs cada 30 segundos (240 QSOs/h, 4× fronte a FT8 monoportadora)</strong> nunha soa portadora con 0.0 dB de penalización (vs 60–120 QSOs/h en FT8 Fox &amp; Hound con división de potencia), escalando a <strong>480 QSOs/h</strong> con dobre portadora!",
        'li_1': "240 QSOs/h (1 portadora, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 portadoras, 100 Hz)",
        'ft8_summary': "<strong>Intercambio Dirixido:</strong> 105 segundos (7 rañuras para 2 QSOs) &mdash; ~69 QSOs/h. En pileup continuo onde a mensaxe 7 se solapa con novas chamadas, o ciclo é de 90s (6 rañuras) = 80 QSOs/h (ata 120 QSOs/h con entrelazado de 2 fluxos; 60 QSOs/h con portadora única). A paralelización multifluxo en Fox &amp; Hound incorre en severas perdas por división de potencia (&minus;3.0 a &minus;7.0 dB) e redución de potencia (back-off), comprometendo o alcance de sinais febles.",
        'lq8_summary': "<strong>Intercambio Dirixido:</strong> 45 segundos (3 rañuras para 2 QSOs) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">En pileup continuo onde a mensaxe 3 se solapa con novas chamadas: 2 QSOs cada 30 segundos = 240 QSOs/h (4× fronte a FT8 monoportadora) con 0.0 dB de penalización de potencia!</span>",
        'meta_speed': "4× de velocidade en pileup con portadora única (240 vs 60 QSOs/h)"
    },
    'eo': {
        'hero_title_2': "4× Pileup-rapido kun ununura portanto (240 kontraŭ 60 QSOs/h)",
        'badge': "4× Pileup-rapido",
        'stat_sub': "Ununura portanto (240 vs 60/h)",
        'desc': "En kontinua amasiĝo (pileup), fendo 3 interkovras kun novaj alvokantoj sur disigitaj frekvencoj, plenumante <strong>2 QSOjn ĉiun 30 sekundojn (240 QSOj/h, 4× pli rapide ol ununura portanto FT8)</strong> sur ununura portanto kun 0.0 dB-potenca perdo (kontraŭ 60–120 QSOj/h en FT8 Fox &amp; Hound kun potencdividado), skalante al <strong>480 QSOj/h</strong> kun duobla portanto!",
        'li_1': "240 QSOj/h (1 portanto, 50 Hz, 0 dB)",
        'li_2': "480 QSOj/h (2 portantoj, 100 Hz)",
        'ft8_summary': "<strong>Direktita Interŝanĝo:</strong> 105 sekundoj (7 fendoj por 2 QSOj) &mdash; ~69 QSOj/h. En kontinua pileup kie mesaĝo 7 interkovras kun novaj alvokoj, la ciklo estas 90s (6 fendoj) = 80 QSOj/h (ĝis 120 QSOj/h kun 2-fluo; 60 QSOj/h sur ununura portanto). Plurflua paraleleco en Fox &amp; Hound kaŭzas gravajn potencdivitajn perdojn (&minus;3.0 ĝis &minus;7.0 dB) kaj amplifilan redukton (back-off), malhelpante la atingon de malfortaj signaloj.",
        'lq8_summary': "<strong>Direktita Interŝanĝo:</strong> 45 sekundoj (3 fendoj por 2 QSOj) &mdash; 160 QSOj/h. <span style=\"color: var(--emerald); font-weight: 700;\">En kontinua pileup kie mesaĝo 3 interkovras kun novaj alvokoj: 2 QSOj ĉiun 30 sekundojn = 240 QSOj/h (4× pli rapide ol ununura portanto FT8) kun 0.0 dB-potenca perdo!</span>",
        'meta_speed': "4× pileup-rapido kun ununura portanto (240 kontraŭ 60 QSOs/h)"
    },
    'fa': {
        'hero_title_2': "سرعت پایل‌آپ ۴ برابر با تک حامل (۲۴۰ در مقابل ۶۰ QSOs/h)",
        'badge': "سرعت پایل‌آپ ۴ برابر",
        'stat_sub': "تک حامل (۲۴۰ در مقابل ۶۰/ساعت)",
        'desc': "در پایل‌آپ مداوم، اسلات ۳ با تماس‌گیرندگان جدید در فرکانس‌های اسپلیت همپوشانی پیدا می‌کند و <strong>۲ تماس را در هر ۳۰ ثانیه (۲۴۰ QSO در ساعت، ۴ برابر FT8 تک حامل)</strong> روی یک حامل منفرد بدون افت توان ۰.۰ دسی‌بل تکمیل می‌کند (در مقایسه با ۶۰–۱۲۰ در FT8 Fox &amp; Hound با تقسیم توان)، که با دو حامل به <strong>۴۸۰ QSO در ساعت</strong> می‌رسد!",
        'li_1': "۲۴۰ QSO در ساعت (۱ حامل، ۵۰ هرتز، ۰ دسی‌بل)",
        'li_2': "۴۸۰ QSO در ساعت (۲ حامل، ۱۰۰ هرتز)",
        'ft8_summary': "<strong>تبادل هدایت‌شده:</strong> ۱۰۵ ثانیه (۷ اسلات برای ۲ تماس) &mdash; حدود ۶۹ QSO/h. در پایل‌آپ مداوم که پیام ۷ با تماس‌های جدید همپوشانی دارد، چرخه ۹۰ ثانیه (۶ اسلات) = ۸۰ QSO/h است (تا ۱۲۰ QSO/h با ۲ جریان متناوب؛ ۶۰ در تک حامل). موازی‌سازی چند جریانی در Fox &amp; Hound باعث افت توان شدید (&minus;۳.۰ تا &minus;۷.۰ دسی‌بل) و کاهش توان تقویت‌کننده (back-off) می‌شود و انتشار سیگنال‌های ضعیف را تضعیف می‌کند.",
        'lq8_summary': "<strong>تبادل هدایت‌شده:</strong> ۴۵ ثانیه (۳ اسلات برای ۲ تماس) &mdash; ۱۶۰ QSO/h. <span style=\"color: var(--emerald); font-weight: 700;\">در پایل‌آپ مداوم که پیام ۳ با تماس‌های جدید همپوشانی دارد: ۲ تماس در هر ۳۰ ثانیه = ۲۴۰ QSO/h (۴ برابر FT8 تک حامل) با افت توان ۰.۰ دسی‌بل!</span>",
        'meta_speed': "سرعت پایل‌آپ ۴ برابر با تک حامل (۲۴۰ در مقابل ۶۰ QSOs/h)"
    },
    'uk': {
        'hero_title_2': "4× швидкість у пайлапах на одній несучій (240 проти 60 QSOs/h)",
        'badge': "4× швидкість у пайлапі",
        'stat_sub': "Одна несуча (240 проти 60/год)",
        'desc': "У безперервному пайлапі слот 3 перекривається з новими викликами на спліт-частотах, завершуючи <strong>2 QSO кожні 30 секунд (240 QSO/год, у 4 рази швидше за FT8 на одній несучій)</strong> на одній несучій без втрати потужності 0.0 дБ (проти 60–120 QSO/год у FT8 Fox &amp; Hound з поділом потужності), масштабуючись до <strong>480 QSO/год</strong> на двох несучих!",
        'li_1': "240 QSO/год (1 несуча, 50 Гц, 0 дБ)",
        'li_2': "480 QSO/год (2 несучі, 100 Гц)",
        'ft8_summary': "<strong>Спрямований обмін:</strong> 105 секунд (7 слотів на 2 QSO) &mdash; ~69 QSO/год. У безперервному пайлапі, де повідомлення 7 перекривається з новими викликами, цикл становить 90 с (6 слотів) = 80 QSO/год (до 120 QSO/год при 2-потоковому чергуванні; 60 QSO/год на одній несучій). Багатопотокова паралелізація у Fox &amp; Hound спричиняє серйозні втрати потужності (&minus;3.0...&minus;7.0 дБ) та вимагає зниження потужності підсилювача (back-off), погіршуючи проходження слабких сигналів.",
        'lq8_summary': "<strong>Спрямований обмін:</strong> 45 секунд (3 слоти на 2 QSO) &mdash; 160 QSO/год. <span style=\"color: var(--emerald); font-weight: 700;\">У безперервному пайлапі, де повідомлення 3 перекривається з новими викликами: 2 QSO кожні 30 секунд = 240 QSO/год (у 4 рази швидше за FT8 на одній несучій) без втрати потужності 0.0 дБ!</span>",
        'meta_speed': "4× швидкість у пайлапах на одній несучій (240 проти 60 QSOs/h)"
    },
    'sw': {
        'hero_title_2': "Kasi ya Pileup ya 4× kwa Kibeberu Kimoja (240 dhidi ya 60 QSOs/h)",
        'badge': "Kasi ya Pileup ya 4×",
        'stat_sub': "Kibeberu Kimoja (240 dhidi ya 60/h)",
        'desc': "Katika pileup endelevu, nafasi ya 3 huingiliana na wapigaji simu wapya kwenye masafa yaliyogawanyika, ikikamilisha <strong>QSO 2 kila sekunde 30 (QSO 240/h, mara 4× ya FT8 ya kibeberu kimoja)</strong> kwenye kibeberu kimoja bila adhabu ya nguvu ya 0.0 dB (dhidi ya QSO 60–120/h katika FT8 Fox &amp; Hound yenye mgawanyo wa nguvu), ikifikia <strong>QSO 480/h</strong> kwa vibeberu viwili!",
        'li_1': "QSO 240/h (kibeberu 1, 50 Hz, 0 dB)",
        'li_2': "QSO 480/h (vibeberu 2, 100 Hz)",
        'ft8_summary': "<strong>Mabadilishano Yaliyoelekezwa:</strong> Sekunde 105 (nafasi 7 kwa QSO 2) &mdash; ~69 QSOs/h. Katika pileup endelevu ambapo ujumbe wa 7 huingiliana na simu mpya, mzunguko ni 90s (nafasi 6) = 80 QSOs/h (hadi 120 QSOs/h kwa njia 2 zilizounganishwa; 60 QSOs/h kibeberu kimoja). Utendaji wa njia nyingi katika Fox &amp; Hound unaleta hasara kubwa za mgawanyo wa nguvu (&minus;3.0 hadi &minus;7.0 dB) na kupunguza nguvu ya kikuza sauti (back-off), na kuharibu upokeaji wa mawimbi dhaifu.",
        'lq8_summary': "<strong>Mabadilishano Yaliyoelekezwa:</strong> Sekunde 45 (nafasi 3 kwa QSO 2) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">Katika pileup endelevu ambapo ujumbe wa 3 huingiliana na simu mpya: QSO 2 kila sekunde 30 = QSO 240/h (mara 4× ya FT8 ya kibeberu kimoja) bila adhabu ya nguvu ya 0.0 dB!</span>",
        'meta_speed': "kasi ya pileup ya 4× kwa kibeberu kimoja (240 dhidi ya 60 QSOs/h)"
    },
    'ha': {
        'hero_title_2': "Saurin Pileup Sau 4 Tare da Mai Daukar Hoto Daya (240 vs 60 QSOs/h)",
        'badge': "Saurin Pileup Sau 4",
        'stat_sub': "Mai Dauka Daya (240 vs 60/h)",
        'desc': "A cikin ci gaba da pileup, ramin 3 yana haɗuwa da sababbin masu kira a kan mitoci masu rarraba, yana kammala <strong>QSO 2 a kowane daƙiƙa 30 (240 QSOs/h, sau 4 fiye da FT8 mai dauka daya)</strong> a kan mai dauka daya ba tare da asarar ƙarfi ta 0.0 dB ba (sabanin 60–120 QSOs/h a cikin FT8 Fox &amp; Hound tare da raba ƙarfi), yana haura zuwa <strong>480 QSOs/h</strong> tare da masu dauka biyu!",
        'li_1': "240 QSOs/h (mai dauka 1, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (masu dauka 2, 100 Hz)",
        'ft8_summary': "<strong>Musayar da Aka Nufa:</strong> Daƙiƙa 105 (ramummuka 7 don QSO 2) &mdash; ~69 QSOs/h. A cikin pileup inda saƙo na 7 ke haɗuwa da sababbin kira, zagayen shine 90s (ramummuka 6) = 80 QSOs/h (har zuwa 120 QSOs/h tare da hanyoyi 2; 60 QSOs/h ga mai dauka daya). Ayyukan hanyoyi da yawa a Fox &amp; Hound yana haifar da asarar raba ƙarfi mai yawa (&minus;3.0 zuwa &minus;7.0 dB) da raguwar ƙarfin amfilifaya, wanda ke lalata isar raunin sigina.",
        'lq8_summary': "<strong>Musayar da Aka Nufa:</strong> Daƙiƙa 45 (ramummuka 3 don QSO 2) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">A cikin pileup inda saƙo na 3 ke haɗuwa da sababbin kira: QSO 2 kowane daƙiƙa 30 = 240 QSOs/h (sau 4 fiye da FT8 mai dauka daya) ba tare da asarar ƙarfi ta 0.0 dB ba!</span>",
        'meta_speed': "saurin pileup sau 4 tare da mai daukar hoto daya (240 vs 60 QSOs/h)"
    },
    'yo': {
        'hero_title_2': "Iyara Pileup 4× Pẹlu Olugbeja Kan (240 vs 60 QSOs/h)",
        'badge': "Iyara Pileup 4×",
        'stat_sub': "Olugbeja Kan (240 vs 60/h)",
        'desc': "Ninu pileup lemọlemọ, aaye 3 n bori pẹlu awọn olupe titun lori awọn igbohunsafẹfẹ pinpin, ti n pari <strong>QSO 2 ni gbogbo iṣẹju-aaya 30 (240 QSOs/h, 4× lori FT8 olugbeja kan)</strong> lori olugbeja kan laisi ijiya agbara 0.0 dB (vs 60–120 QSOs/h ninu FT8 Fox &amp; Hound pẹlu pipin agbara), ti n de <strong>480 QSOs/h</strong> pẹlu olugbeja meji!",
        'li_1': "240 QSOs/h (olugbeja 1, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (olugbeja 2, 100 Hz)",
        'ft8_summary': "<strong>Paṣipaarọ Taara:</strong> Iṣẹju-aaya 105 (awọn aaye 7 fun QSO 2) &mdash; ~69 QSOs/h. Ninu pileup lemọlemọ nibiti ifiranṣẹ 7 ti n bori pẹlu awọn ipe titun, yiyi jẹ 90s (awọn aaye 6) = 80 QSOs/h (to 120 QSOs/h pẹlu awọn ṣiṣan 2; 60 QSOs/h lori olugbeja kan). Iṣiṣẹpọ ṣiṣan pupọ ni Fox &amp; Hound n fa awọn adanu pipin agbara nla (&minus;3.0 si &minus;7.0 dB) ati idinku agbara amugbooro, ti n ba ifihan agbara ailera jẹ.",
        'lq8_summary': "<strong>Paṣipaarọ Taara:</strong> Iṣẹju-aaya 45 (awọn aaye 3 fun QSO 2) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">Ninu pileup lemọlemọ nibiti ifiranṣẹ 3 ti n bori pẹlu awọn ipe titun: QSO 2 ni gbogbo iṣẹju-aaya 30 = 240 QSOs/h (4× lori FT8 olugbeja kan) pẹlu ijiya agbara 0.0 dB!</span>",
        'meta_speed': "iyara pileup 4× pẹlu olugbeja kan (240 vs 60 QSOs/h)"
    },
    'am': {
        'hero_title_2': "በአንድ ተሸካሚ የ 4× የፓይልአፕ ፍጥነት (240 vs 60 QSOs/h)",
        'badge': "የ 4× የፓይልአፕ ፍጥነት",
        'stat_sub': "አንድ ተሸካሚ (240 vs 60/h)",
        'desc': "በተከታታይ ፓይልአፕ ውስጥ፣ ማስገቢያ 3 ከተከፋፈሉ ድግግሞሾች አዳዲስ ጠሪዎች ጋር ይደራረባል፣ ይህም በ 0.0 dB የኃይል ቅጣት በአንድ ተሸካሚ ላይ <strong>በየ 30 ሰከንዶች 2 QSOዎችን (240 QSOs/h፣ ከ FT8 ነጠላ ተሸካሚ 4× የበለጠ)</strong> ያጠናቅቃል (በ FT8 Fox &amp; Hound ውስጥ ከ 60–120 QSOs/h ጋር ሲነፃፀር)፣ በሁለት ተሸካሚዎች ወደ <strong>480 QSOs/h</strong> ያድጋል!",
        'li_1': "240 QSOs/h (1 ተሸካሚ፣ 50 Hz፣ 0 dB)",
        'li_2': "480 QSOs/h (2 ተሸካሚዎች፣ 100 Hz)",
        'ft8_summary': "<strong>ቀጥተኛ ልውውጥ:</strong> 105 ሰከንዶች (ለ 2 QSOዎች 7 ማስገቢያዎች) &mdash; ~69 QSOs/h። መልእክት 7 ከአዳዲስ ጥሪዎች ጋር በሚደራረብበት ተከታታይ ፓይልአፕ ውስጥ ዑደቱ 90s (6 ማስገቢያዎች) = 80 QSOs/h ይሆናል (በ 2 ዥረቶች እስከ 120 QSOs/h፣ ነጠላ ተሸካሚ 60 QSOs/h)። በ Fox &amp; Hound ውስጥ ባለብዙ ዥረት ማስተላለፍ ከፍተኛ የኃይል ክፍፍል ኪሳራዎችን (&minus;3.0 እስከ &minus;7.0 dB) ያስከትላል እና የማጉያውን ኃይል ይቀንሳል፣ ይህም ደካማ ምልክቶችን መድረስ ይጎዳል።",
        'lq8_summary': "<strong>ቀጥተኛ ልውውጥ:</strong> 45 ሰከንዶች (ለ 2 QSOዎች 3 ማስገቢያዎች) &mdash; 160 QSOs/h። <span style=\"color: var(--emerald); font-weight: 700;\">መልእክት 3 ከአዳዲስ ጥሪዎች ጋር በሚደራረብበት ተከታታይ ፓይልአፕ ውስጥ: በየ 30 ሰከንዶች 2 QSOዎች = 240 QSOs/h (ከነጠላ ተሸካሚ FT8 4× የበለጠ) በ 0.0 dB የኃይል ቅጣት!</span>",
        'meta_speed': "በአንድ ተሸካሚ የ 4× የፓይልአፕ ፍጥነት (240 vs 60 QSOs/h)"
    },
    'az': {
        'hero_title_2': "Tək Daşıyıcı ilə 4× Pileup Sürəti (240 vs 60 QSOs/saat)",
        'badge': "4× Pileup Sürəti",
        'stat_sub': "Tək Daşıyıcı (240 vs 60/s)",
        'desc': "Davamlı pileup rejimində 3-cü interval bölünmüş tezliklərdəki yeni çağırışlarla üst-üstə düşür, 0.0 dB güc itkisi olmadan tək daşıyıcıda <strong>hər 30 saniyədə 2 QSO (240 QSO/saat, tək daşıyıcılı FT8-dən 4× sürətli)</strong> tamamlayır (güc bölünməsi olan FT8 Fox &amp; Hound-dakı 60–120 QSO/saat ilə müqayisədə), iki daşıyıcı ilə <strong>480 QSO/saat</strong>a çatır!",
        'li_1': "240 QSO/saat (1 daşıyıcı, 50 Hz, 0 dB)",
        'li_2': "480 QSO/saat (2 daşıyıcı, 100 Hz)",
        'ft8_summary': "<strong>İstiqamətləndirilmiş Mübadilə:</strong> 105 saniyə (2 QSO üçün 7 interval) &mdash; ~69 QSO/saat. Mesaj 7-nin yeni çağırışlarla üst-üstə düşdüyü davamlı pileup-da dövr 90s (6 interval) = 80 QSO/saat olur (2 axınlı ardıcıllıqla 120 QSO/saat-a qədər; tək daşıyıcıda 60 QSO/saat). Fox &amp; Hound rejimində çoxaxınlı paralelləşdirmə gücün kəskin bölünməsinə (&minus;3.0 - &minus;7.0 dB) və gücləndiricinin gücünün azaldılmasına (back-off) səbəb olur, bu da zəif siqnalların qəbuluna mənfi təsir göstərir.",
        'lq8_summary': "<strong>İstiqamətləndirilmiş Mübadilə:</strong> 45 saniyə (2 QSO üçün 3 interval) &mdash; 160 QSO/saat. <span style=\"color: var(--emerald); font-weight: 700;\">Mesaj 3-ün yeni çağırışlarla üst-üstə düşdüyü davamlı pileup-da: hər 30 saniyədə 2 QSO = 240 QSO/saat (tək daşıyıcılı FT8-dən 4× sürətli) 0.0 dB güc itkisi ilə!</span>",
        'meta_speed': "tək daşıyıcı ilə 4× pileup sürəti (240 vs 60 QSOs/saat)"
    },
    'uz': {
        'hero_title_2': "Yagona tashuvchi bilan 4× Pileup tezligi (240 vs 60 QSOs/soat)",
        'badge': "4× Pileup tezligi",
        'stat_sub': "Yagona tashuvchi (240 vs 60/soat)",
        'desc': "Uzluksiz pileup rejimida 3-slot split chastotalardagi yangi chaqiruvchilar bilan ustma-ust tushadi va 0.0 dB quvvat yo'qotmasdan yagona tashuvchida <strong>har 30 soniyada 2 ta QSO (240 QSO/soat, yagona tashuvchili FT8 dan 4× tezroq)</strong> ni yakunlaydi (quvvat taqsimoti bo'lgan FT8 Fox &amp; Hound dagi 60–120 QSO/soatga nisbatan), ikkita tashuvchida <strong>480 QSO/soat</strong> ga yetadi!",
        'li_1': "240 QSO/soat (1 tashuvchi, 50 Hz, 0 dB)",
        'li_2': "480 QSO/soat (2 tashuvchi, 100 Hz)",
        'ft8_summary': "<strong>Yo'naltirilgan almashinuv:</strong> 105 soniya (2 QSO uchun 7 slot) &mdash; ~69 QSO/soat. 7-xabar yangi chaqiruvlar bilan ustma-ust tushadigan uzluksiz pileupda tsikl 90s (6 slot) = 80 QSO/soat (2 oqimli ketma-ketlikda 120 QSO/soatgacha; yagona tashuvchida 60 QSO/soat). Fox &amp; Hound dagi ko'p oqimli parallellashtirish jiddiy quvvat taqsimoti yo'qotishlariga (&minus;3.0 dan &minus;7.0 dB gacha) va kuchaytirgich quvvatini pasaytirishga (back-off) olib keladi, bu esa zaif signallar qabuliga salbiy ta'sir qiladi.",
        'lq8_summary': "<strong>Yo'naltirilgan almashinuv:</strong> 45 soniya (2 QSO uchun 3 slot) &mdash; 160 QSO/soat. <span style=\"color: var(--emerald); font-weight: 700;\">3-xabar yangi chaqiruvlar bilan ustma-ust tushadigan uzluksiz pileupda: har 30 soniyada 2 QSO = 240 QSO/soat (yagona tashuvchili FT8 dan 4× tezroq) 0.0 dB quvvat yo'qotishsiz!</span>",
        'meta_speed': "yagona tashuvchi bilan 4× pileup tezligi (240 vs 60 QSOs/soat)"
    },
    'ta': {
        'hero_title_2': "ஒற்றை கேரியருடன் 4× பைலப் வேகம் (240 vs 60 QSOs/h)",
        'badge': "4× பைலப் வேகம்",
        'stat_sub': "ஒற்றை கேரியர் (240 vs 60/h)",
        'desc': "தொடர்ச்சியான பைலப்பில், ஸ்லாட் 3 பிரிக்கப்பட்ட அலைவரிசைகளில் புதிய அழைப்பாளர்களுடன் மேலெழுகிறது, 0.0 dB பவர் இழப்பு இல்லாமல் ஒற்றை கேரியரில் <strong>ஒவ்வொரு 30 வினாடிக்கும் 2 QSOகளை (240 QSOs/h, ஒற்றை கேரியர் FT8 ஐ விட 4×)</strong> நிறைவு செய்கிறது (பவர் பகிர்வு கொண்ட FT8 Fox &amp; Hound இன் 60–120 QSOs/h உடன் ஒப்பிடும்போது), இரட்டை கேரியரில் <strong>480 QSOs/h</strong> வரை உயர்கிறது!",
        'li_1': "240 QSOs/h (1 கேரியர், 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 கேரியர்கள், 100 Hz)",
        'ft8_summary': "<strong>நேரடி பரிமாற்றம்:</strong> 105 வினாடிகள் (2 QSOகளுக்கு 7 ஸ்லாட்டுகள்) &mdash; ~69 QSOs/h. தொடர்ச்சியான பைலப்பில் செய்தி 7 புதிய அழைப்புகளுடன் மேலெழும்போது, சுழற்சி 90s (6 ஸ்லாட்டுகள்) = 80 QSOs/h ஆகும் (2-ஸ்ட்ரீமில் 120 QSOs/h வரை; ஒற்றை கேரியரில் 60 QSOs/h). Fox &amp; Hound இல் மல்டி-ஸ்ட்ரீம் இணைசெயல்பாடு கடுமையான பவர்-பகிர்வு இழப்புகளையும் (&minus;3.0 முதல் &minus;7.0 dB) ஆம்ப்ளிஃபையர் பேக்-ஆஃபையும் ஏற்படுத்துகிறது, இது பலவீனமான சமிக்ஞைகளின் பரவலைப் பாதிக்கிறது.",
        'lq8_summary': "<strong>நேரடி பரிமாற்றம்:</strong> 45 வினாடிகள் (2 QSOகளுக்கு 3 ஸ்லாட்டுகள்) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">தொடர்ச்சியான பைலப்பில் செய்தி 3 புதிய அழைப்புகளுடன் மேலெழும்போது: ஒவ்வொரு 30 வினாடிக்கும் 2 QSOகள் = 240 QSOs/h (ஒற்றை கேரியர் FT8 ஐ விட 4×) 0.0 dB பவர் இழப்புடன்!</span>",
        'meta_speed': "ஒற்றை கேரியருடன் 4× பைலப் வேகம் (240 vs 60 QSOs/h)"
    },
    'te': {
        'hero_title_2': "సింగిల్ క్యారియర్‌తో 4× పైలప్ వేగం (240 vs 60 QSOs/h)",
        'badge': "4× పైలప్ వేగం",
        'stat_sub': "సింగిల్ క్యారియర్ (240 vs 60/h)",
        'desc': "నిరంతర పైలప్‌లో, స్లాట్ 3 స్ప్లిట్ ఫ్రీక్వెన్సీలపై కొత్త కాలర్లతో అతివ్యాప్తి చెందుతుంది, 0.0 dB పవర్ పెనాల్టీ లేకుండా సింగిల్ క్యారియర్‌పై <strong>ప్రతి 30 సెకన్లకు 2 QSOలను (240 QSOs/h, సింగిల్-క్యారియర్ FT8 కంటే 4×)</strong> పూర్తి చేస్తుంది (పవర్ విభజనతో FT8 Fox &amp; Hound లోని 60–120 QSOs/h తో పోలిస్తే), డ్యూయల్ క్యారియర్‌తో <strong>480 QSOs/h</strong> కి పెరుగుతుంది!",
        'li_1': "240 QSOs/h (1 క్యారియర్, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 క్యారియర్లు, 100 Hz)",
        'ft8_summary': "<strong>డైరెక్టెడ్ మార్పిడి:</strong> 105 సెకన్లు (2 QSOలకు 7 స్లాట్లు) &mdash; ~69 QSOs/h. నిరంతర పైలప్‌లో సందేశం 7 కొత్త కాల్‌లతో అతివ్యాప్తి చెందినప్పుడు, చక్రం 90s (6 స్లాట్లు) = 80 QSOs/h అవుతుంది (2-స్ట్రీమ్‌లతో 120 QSOs/h వరకు; సింగిల్ క్యారియర్ 60 QSOs/h). Fox &amp; Hound లో మల్టీ-స్ట్రీమ్ సమాంతరీకరణ తీవ్రమైన పవర్-విభజన నష్టాలను (&minus;3.0 నుండి &minus;7.0 dB) మరియు యాంప్లిఫైయర్ బ్యాక్-ఆఫ్‌ను కలిగిస్తుంది, ఇది బలహీనమైన సిగ్నల్స్ పరిధిని దెబ్బతీస్తుంది.",
        'lq8_summary': "<strong>డైరెక్టెడ్ మార్పిడి:</strong> 45 సెకన్లు (2 QSOలకు 3 స్లాట్లు) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">నిరంతర పైలప్‌లో సందేశం 3 కొత్త కాల్‌లతో అతివ్యాప్తి చెందినప్పుడు: ప్రతి 30 సెకన్లకు 2 QSOలు = 240 QSOs/h (సింగిల్ క్యారియర్ FT8 కంటే 4×) 0.0 dB పవర్ పెనాల్టీతో!</span>",
        'meta_speed': "సింగిల్ క్యారియర్‌తో 4× పైలప్ వేగం (240 vs 60 QSOs/h)"
    },
    'mr': {
        'hero_title_2': "सिंगल-कॅरियरसह 4× पाइलअप गती (240 विरूद्ध 60 QSOs/h)",
        'badge': "4× पाइलअप गती",
        'stat_sub': "सिंगल-कॅरियर (240 विरूद्ध 60/h)",
        'desc': "सततच्या पाइलअपमध्ये, स्लॉट 3 स्प्लिट फ्रिक्वेन्सीवरील नवीन कॉलर्ससह ओव्हरलॅप होतो, ज्यामुळे 0.0 dB पॉवर पेनल्टीसह सिंगल कॅरियरवर <strong>दर 30 सेकंदाला 2 QSO (240 QSOs/h, सिंगल-कॅरियर FT8 पेक्षा 4×)</strong> पूर्ण होतात (पॉवर स्प्लिटसह FT8 Fox &amp; Hound मधील 60–120 QSOs/h च्या तुलनेत), जे ड्युअल-कॅरियरसह <strong>480 QSOs/h</strong> पर्यंत पोहोचते!",
        'li_1': "240 QSOs/h (1 कॅरियर, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 कॅरियर, 100 Hz)",
        'ft8_summary': "<strong>निर्देशित देवाणघेवाण:</strong> 105 सेकंद (2 QSO साठी 7 स्लॉट) &mdash; ~69 QSOs/h. सततच्या पाइलअपमध्ये जेथे संदेश 7 नवीन कॉलसह ओव्हरलॅप होतो, चक्र 90s (6 स्लॉट) = 80 QSOs/h असते (2-स्ट्रीमसह 120 QSOs/h पर्यंत; सिंगल कॅरियर 60 QSOs/h). Fox &amp; Hound मधील मल्टी-स्ट्रीम पॅरालेलिझममुळे गंभीर पॉवर-स्प्लिट नुकसान (&minus;3.0 ते &minus;7.0 dB) आणि अँप्लिफायर बॅक-ऑफ होते, ज्यामुळे कमकुवत सिग्नल पोहोचण्यावर परिणाम होतो.",
        'lq8_summary': "<strong>निर्देशित देवाणघेवाण:</strong> 45 सेकंद (2 QSO साठी 3 स्लॉट) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">सततच्या पाइलअपमध्ये जेथे संदेश 3 नवीन कॉलसह ओव्हरलॅप होतो: दर 30 सेकंदाला 2 QSO = 240 QSOs/h (सिंगल-कॅरियर FT8 पेक्षा 4×) 0.0 dB पॉवर पेनल्टीसह!</span>",
        'meta_speed': "सिंगल-कॅरियरसह 4× पाइलअप गती (240 विरूद्ध 60 QSOs/h)"
    },
    'gu': {
        'hero_title_2': "સિંગલ-કેરિયર સાથે 4× પાઇલઅપ સ્પીડ (240 વિ 60 QSOs/h)",
        'badge': "4× પાઇલઅપ સ્પીડ",
        'stat_sub': "સિંગલ-કેરિયર (240 વિ 60/h)",
        'desc': "સતત પાઇલઅપમાં, સ્લોટ 3 સ્પ્લિટ ફ્રીક્વન્સી પર નવા કૉલર્સ સાથે ઓવરલેપ થાય છે, 0.0 dB પાવર પેનલ્ટી સાથે સિંગલ કેરિયર પર <strong>દર 30 સેકન્ડે 2 QSO (240 QSOs/h, સિંગલ-કેરિયર FT8 કરતાં 4×)</strong> પૂર્ણ કરે છે (પાવર સ્પ્લિટ સાથે FT8 Fox &amp; Hound ના 60–120 QSOs/h ની તુલનામાં), જે ડ્યુઅલ-કેરિયર સાથે <strong>480 QSOs/h</strong> સુધી પહોંચે છે!",
        'li_1': "240 QSOs/h (1 કેરિયર, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 કેરિયર્સ, 100 Hz)",
        'ft8_summary': "<strong>નિર્દેશિત વિનિમય:</strong> 105 સેકન્ડ (2 QSO માટે 7 સ્લોટ) &mdash; ~69 QSOs/h. સતત પાઇલઅપમાં જ્યાં સંદેશ 7 નવા કૉલ્સ સાથે ઓવરલેપ થાય છે, ચક્ર 90s (6 સ્લોટ) = 80 QSOs/h થાય છે (2-સ્ટ્રીમ સાથે 120 QSOs/h સુધી; સિંગલ કેરિયર 60 QSOs/h). Fox &amp; Hound માં મલ્ટી-સ્ટ્રીમ સમાંતરીકરણથી ભારે પાવર-સ્પ્લિટ નુકસાન (&minus;3.0 થી &minus;7.0 dB) અને એમ્પ્લીફાયર બેક-ઓફ થાય છે, જે નબળા સિગ્નલોના ફેલાવાને અસર કરે છે.",
        'lq8_summary': "<strong>નિર્દેશિત વિનિમય:</strong> 45 સેકન્ડ (2 QSO માટે 3 સ્લોટ) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">સતત પાઇલઅપમાં જ્યાં સંદેશ 3 નવા કૉલ્સ સાથે ઓવરલેપ થાય છે: દર 30 સેકન્ડે 2 QSO = 240 QSOs/h (સિંગલ-કેરિયર FT8 કરતાં 4×) 0.0 dB પાવર પેનલ્ટી સાથે!</span>",
        'meta_speed': "સિંગલ-કેરિયર સાથે 4× પાઇલઅપ સ્પીડ (240 વિ 60 QSOs/h)"
    },
    'kn': {
        'hero_title_2': "ಸಿಂಗಲ್ ಕ್ಯಾರಿಯರ್‌ನೊಂದಿಗೆ 4× ಪೈಲಪ್ ವೇಗ (240 ವಿರುದ್ಧ 60 QSOs/h)",
        'badge': "4× ಪೈಲಪ್ ವೇಗ",
        'stat_sub': "ಸಿಂಗಲ್ ಕ್ಯಾರಿಯರ್ (240 vs 60/h)",
        'desc': "ನಿರಂತರ ಪೈಲಪ್ ಕಾರ್ಯಾಚರಣೆಯಲ್ಲಿ, ಸ್ಲಾಟ್ 3 ವಿಭಜಿತ ಆವರ್ತನಗಳಲ್ಲಿ ಹೊಸ ಕಾಲರ್‌ಗಳೊಂದಿಗೆ ಅತಿಕ್ರಮಿಸುತ್ತದೆ, 0.0 dB ಪವರ್ ಪೆನಾಲ್ಟಿ ಇಲ್ಲದೆ ಸಿಂಗಲ್ ಕ್ಯಾರಿಯರ್‌ನಲ್ಲಿ <strong>ಪ್ರತಿ 30 ಸೆಕೆಂಡಿಗೆ 2 QSOಗಳನ್ನು (240 QSOs/h, ಸಿಂಗಲ್-ಕ್ಯಾರಿಯರ್ FT8 ಗಿಂತ 4×)</strong> ಪೂರ್ಣಗೊಳಿಸುತ್ತದೆ (ಪವರ್ ವಿಭಜನೆಯೊಂದಿಗೆ FT8 Fox &amp; Hound ನ 60–120 QSOs/h ಗೆ ಹೋಲಿಸಿದರೆ), ಡ್ಯುಯಲ್ ಕ್ಯಾರಿಯರ್‌ನಲ್ಲಿ <strong>480 QSOs/h</strong> ಗೆ ವಿಸ್ತರಿಸುತ್ತದೆ!",
        'li_1': "240 QSOs/h (1 ಕ್ಯಾರಿಯರ್, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 ಕ್ಯಾರಿಯರ್‌ಗಳು, 100 Hz)",
        'ft8_summary': "<strong>ನಿರ್ದೇಶಿತ ವಿನಿಮಯ:</strong> 105 ಸೆಕೆಂಡುಗಳು (2 QSO ಗಳಿಗೆ 7 ಸ್ಲಾಟ್‌ಗಳು) &mdash; ~69 QSOs/h. ನಿರಂತರ ಪೈಲಪ್‌ನಲ್ಲಿ ಸಂದೇಶ 7 ಹೊಸ ಕರೆಗಳೊಂದಿಗೆ ಅತಿಕ್ರಮಿಸಿದಾಗ, ಸೈಕಲ್ 90s (6 ಸ್ಲಾಟ್‌ಗಳು) = 80 QSOs/h ಆಗಿರುತ್ತದೆ (2-ಸ್ಟ್ರೀಮ್‌ನೊಂದಿಗೆ 120 QSOs/h ವರೆಗೆ; ಸಿಂಗಲ್ ಕ್ಯಾರಿಯರ್ 60 QSOs/h). Fox &amp; Hound ನಲ್ಲಿನ ಮಲ್ಟಿ-ಸ್ಟ್ರೀಮ್ ಸಮಾನಾಂತರತೆಯು ತೀವ್ರ ಪವರ್-ವಿಭಜನೆ ನಷ್ಟಗಳನ್ನು (&minus;3.0 ರಿಂದ &minus;7.0 dB) ಮತ್ತು ಆಂಪ್ಲಿಫೈಯರ್ ಬ್ಯಾಕ್-ಆಫ್ ಅನ್ನು ಉಂಟುಮಾಡುತ್ತದೆ, ದುರ್ಬಲ ಸಂಕೇತಗಳ ವ್ಯಾಪ್ತಿಯನ್ನು ಕಡಿಮೆ ಮಾಡುತ್ತದೆ.",
        'lq8_summary': "<strong>ನಿರ್ದೇಶಿತ ವಿನಿಮಯ:</strong> 45 ಸೆಕೆಂಡುಗಳು (2 QSO ಗಳಿಗೆ 3 ಸ್ಲಾಟ್‌ಗಳು) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">ನಿರಂತರ ಪೈಲಪ್‌ನಲ್ಲಿ ಸಂದೇಶ 3 ಹೊಸ ಕರೆಗಳೊಂದಿಗೆ ಅತಿಕ್ರಮಿಸಿದಾಗ: ಪ್ರತಿ 30 ಸೆಕೆಂಡಿಗೆ 2 QSOಗಳು = 240 QSOs/h (ಸಿಂಗಲ್-ಕ್ಯಾರಿಯರ್ FT8 ಗಿಂತ 4×) 0.0 dB ಪವರ್ ಪೆನಾಲ್ಟಿಯೊಂದಿಗೆ!</span>",
        'meta_speed': "ಸಿಂಗಲ್ ಕ್ಯಾರಿಯರ್‌ನೊಂದಿಗೆ 4× ಪೈಲಪ್ ವೇಗ (240 ವಿರುದ್ಧ 60 QSOs/h)"
    },
    'ml': {
        'hero_title_2': "സിംഗിൾ കാരിയറിൽ 4× പൈലപ്പ് വേഗത (240 vs 60 QSOs/h)",
        'badge': "4× പൈലപ്പ് വേഗത",
        'stat_sub': "സിംഗിൾ കാരിയർ (240 vs 60/h)",
        'desc': "തുടർച്ചയായ പൈലപ്പിൽ, സ്ലോട്ട് 3 സ്പ്ലിറ്റ് ഫ്രീക്വൻസികളിൽ പുതിയ കോളർമാരുമായി ഓവർലാപ്പ് ചെയ്യുന്നു, 0.0 dB പവർ നഷ്ടമില്ലാതെ സിംഗിൾ കാരിയറിൽ <strong>ഓരോ 30 സെക്കൻഡിലും 2 QSOകൾ (240 QSOs/h, സിംഗിൾ-കാരിയർ FT8 നേക്കാൾ 4×)</strong> പൂർത്തിയാക്കുന്നു (പവർ വിഭജനമുള്ള FT8 Fox &amp; Hound ലെ 60–120 QSOs/h മായി താരതമ്യം ചെയ്യുമ്പോൾ), ഡ്യുവൽ കാരിയറിൽ <strong>480 QSOs/h</strong> വരെ ഉയരുന്നു!",
        'li_1': "240 QSOs/h (1 കാരിയർ, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 കാരിയറുകൾ, 100 Hz)",
        'ft8_summary': "<strong>ഡയറക്റ്റഡ് എക്സ്ചേഞ്ച്:</strong> 105 സെക്കൻഡ് (2 QSOകൾക്ക് 7 സ്ലോട്ടുകൾ) &mdash; ~69 QSOs/h. തുടർച്ചയായ പൈലപ്പിൽ സന്ദേശം 7 പുതിയ കോളുകളുമായി ഓവർലാപ്പ് ചെയ്യുമ്പോൾ, സൈക്കിൾ 90s (6 സ്ലോട്ടുകൾ) = 80 QSOs/h ആണ് (2-സ്ട്രീം ഇന്റർലീവിംഗിലൂടെ 120 QSOs/h വരെ; സിംഗിൾ കാരിയറിൽ 60 QSOs/h). Fox &amp; Hound ലെ മൾട്ടി-സ്ട്രീം സമാന്തരീകരണം കടുത്ത പവർ വിഭജന നഷ്ടങ്ങൾക്കും (&minus;3.0 മുതൽ &minus;7.0 dB വരെ) ആംപ്ലിഫയർ ബാക്ക്-ഓഫിനും കാരണമാകുന്നു, ഇത് ദുർബല സിഗ്നലുകളുടെ ലഭ്യതയെ ബാധിക്കുന്നു.",
        'lq8_summary': "<strong>ഡയറക്റ്റഡ് എക്സ്ചേഞ്ച്:</strong> 45 സെക്കൻഡ് (2 QSOകൾക്ക് 3 സ്ലോട്ടുകൾ) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">തുടർച്ചയായ പൈലപ്പിൽ സന്ദേശം 3 പുതിയ കോളുകളുമായി ഓവർലാപ്പ് ചെയ്യുമ്പോൾ: ഓരോ 30 സെക്കൻഡിലും 2 QSOകൾ = 240 QSOs/h (സിംഗിൾ കാരിയർ FT8 നേക്കാൾ 4×) 0.0 dB പവർ നഷ്ടത്തോടെ!</span>",
        'meta_speed': "സിംഗിൾ കാരിയറിൽ 4× പൈലപ്പ് വേഗത (240 vs 60 QSOs/h)"
    },
    'pa': {
        'hero_title_2': "ਸਿੰਗਲ ਕੈਰੀਅਰ ਨਾਲ 4× ਪਾਇਲਅੱਪ ਗਤੀ (240 ਬਨਾਮ 60 QSOs/h)",
        'badge': "4× ਪਾਇਲਅੱਪ ਗਤੀ",
        'stat_sub': "ਸਿੰਗਲ ਕੈਰੀਅਰ (240 ਬਨਾਮ 60/h)",
        'desc': "ਲਗਾਤਾਰ ਪਾਇਲਅੱਪ ਵਿੱਚ, ਸਲਾਟ 3 ਸਪਲਿਟ ਫ੍ਰੀਕੁਐਂਸੀਆਂ 'ਤੇ ਨਵੇਂ ਕਾਲਰਾਂ ਨਾਲ ਓਵਰਲੈਪ ਹੁੰਦਾ ਹੈ, 0.0 dB ਪਾਵਰ ਪੈਨਲਟੀ ਤੋਂ ਬਿਨਾਂ ਸਿੰਗਲ ਕੈਰੀਅਰ 'ਤੇ <strong>ਹਰ 30 ਸਕਿੰਟਾਂ ਵਿੱਚ 2 QSO (240 QSOs/h, ਸਿੰਗਲ-ਕੈਰੀਅਰ FT8 ਨਾਲੋਂ 4×)</strong> ਪੂਰੇ ਕਰਦਾ ਹੈ (ਪਾਵਰ ਸਪਲਿਟ ਨਾਲ FT8 Fox &amp; Hound ਦੇ 60–120 QSOs/h ਦੇ ਮੁਕਾਬਲੇ), ਜੋ ਡਿਊਲ ਕੈਰੀਅਰ ਨਾਲ <strong>480 QSOs/h</strong> ਤੱਕ ਪਹੁੰਚਦਾ ਹੈ!",
        'li_1': "240 QSOs/h (1 ਕੈਰੀਅਰ, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 ਕੈਰੀਅਰ, 100 Hz)",
        'ft8_summary': "<strong>ਨਿਰਦੇਸ਼ਿਤ ਵਟਾਂਦਰਾ:</strong> 105 ਸਕਿੰਟ (2 QSO ਲਈ 7 ਸਲਾਟ) &mdash; ~69 QSOs/h। ਲਗਾਤਾਰ ਪਾਇਲਅੱਪ ਵਿੱਚ ਜਿੱਥੇ ਸੁਨੇਹਾ 7 ਨਵੀਆਂ ਕਾਲਾਂ ਨਾਲ ਓਵਰਲੈਪ ਹੁੰਦਾ ਹੈ, ਚੱਕਰ 90s (6 ਸਲਾਟ) = 80 QSOs/h ਹੁੰਦਾ ਹੈ (2-ਸਟ੍ਰੀਮ ਨਾਲ 120 QSOs/h ਤੱਕ; ਸਿੰਗਲ ਕੈਰੀਅਰ 60 QSOs/h)। Fox &amp; Hound ਵਿੱਚ ਮਲਟੀ-ਸਟ੍ਰੀਮ ਸਮਾਨਤਾ ਨਾਲ ਭਾਰੀ ਪਾਵਰ-ਸਪਲਿਟ ਨੁਕਸਾਨ (&minus;3.0 ਤੋਂ &minus;7.0 dB) ਅਤੇ ਐਂਪਲੀਫਾਇਰ ਬੈਕ-ਆਫ ਹੁੰਦਾ ਹੈ, ਜਿਸ ਨਾਲ ਕਮਜ਼ੋਰ ਸਿਗਨਲਾਂ ਦਾ ਦਾਇਰਾ ਘਟਦਾ ਹੈ।",
        'lq8_summary': "<strong>ਨਿਰਦੇਸ਼ਿਤ ਵਟਾਂਦਰਾ:</strong> 45 ਸਕਿੰਟ (2 QSO ਲਈ 3 ਸਲਾਟ) &mdash; 160 QSOs/h। <span style=\"color: var(--emerald); font-weight: 700;\">ਲਗਾਤਾਰ ਪਾਇਲਅੱਪ ਵਿੱਚ ਜਿੱਥੇ ਸੁਨੇਹਾ 3 ਨਵੀਆਂ ਕਾਲਾਂ ਨਾਲ ਓਵਰਲੈਪ ਹੁੰਦਾ ਹੈ: ਹਰ 30 ਸਕਿੰਟਾਂ ਵਿੱਚ 2 QSO = 240 QSOs/h (ਸਿੰਗਲ-ਕੈਰੀਅਰ FT8 ਨਾਲੋਂ 4×) 0.0 dB ਪਾਵਰ ਪੈਨਲਟੀ ਨਾਲ!</span>",
        'meta_speed': "ਸਿੰਗਲ ਕੈਰੀਅਰ ਨਾਲ 4× ਪਾਇਲਅੱਪ ਗਤੀ (240 ਬਨਾਮ 60 QSOs/h)"
    },
    'or': {
        'hero_title_2': "ସିଙ୍ଗଲ୍ କ୍ୟାରିଅର୍ ସହିତ 4× ପାଇଲ୍ଅପ୍ ଗତି (240 ବନାମ 60 QSOs/h)",
        'badge': "4× ପାଇଲ୍ଅପ୍ ଗତି",
        'stat_sub': "ସିଙ୍ଗଲ୍ କ୍ୟାରିଅର୍ (240 ବନାମ 60/h)",
        'desc': "କ୍ରମାଗତ ପାଇଲ୍ଅପ୍‌ରେ, ସ୍ଲଟ୍ 3 ସ୍ପ୍ଲିଟ୍ ଫ୍ରିକ୍ୱେନ୍ସିରେ ନୂତନ କଲର୍‌ମାନଙ୍କ ସହିତ ଓଭରଲ୍ଯାପ୍ ହୁଏ, 0.0 dB ପାୱାର୍ ପେନାଲ୍ଟି ବିନା ଗୋଟିଏ କ୍ୟାରିଅର୍‌ରେ <strong>ପ୍ରତି 30 ସେକେଣ୍ଡରେ 2ଟି QSO (240 QSOs/h, ସିଙ୍ଗଲ୍ କ୍ୟାରିଅର୍ FT8 ଠାରୁ 4×)</strong> ସମ୍ପୂର୍ଣ୍ଣ କରେ (FT8 Fox &amp; Hound ର ପାୱାର୍ ସ୍ପ୍ଲିଟ୍ ସହିତ 60–120 QSOs/h ତୁଳନାରେ), ଡୁଆଲ୍ କ୍ୟାରିଅର୍‌ରେ <strong>480 QSOs/h</strong> କୁ ବୃଦ୍ଧି ପାଏ!",
        'li_1': "240 QSOs/h (1 କ୍ୟାରିଅର୍, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 କ୍ୟାରିଅର୍, 100 Hz)",
        'ft8_summary': "<strong>ନିର୍ଦ୍ଦେଶିତ ବିନିମୟ:</strong> 105 ସେକେଣ୍ଡ (2ଟି QSO ପାଇଁ 7ଟି ସ୍ଲଟ୍) &mdash; ~69 QSOs/h। କ୍ରମାଗତ ପାଇଲ୍ଅପ୍‌ରେ ଯେଉଁଠାରେ ବାର୍ତ୍ତା 7 ନୂତନ କଲ୍ ସହିତ ଓଭରଲ୍ଯାପ୍ ହୁଏ, ସାଇକଲ୍ 90s (6ଟି ସ୍ଲଟ୍) = 80 QSOs/h ହୁଏ (2-ଷ୍ଟ୍ରିମ୍ ସହିତ 120 QSOs/h ପର୍ଯ୍ୟନ୍ତ; ସିଙ୍ଗଲ୍ କ୍ୟାରିଅର୍ 60 QSOs/h)। Fox &amp; Hound ରେ ମଲ୍ଟି-ଷ୍ଟ୍ରିମ୍ ପାରାଲାଲାଇଜେସନ୍ ଯୋଗୁଁ ପ୍ରବଳ ପାୱାର୍-ସ୍ପ୍ଲିଟ୍ କ୍ଷତି (&minus;3.0 ରୁ &minus;7.0 dB) ଏବଂ ଆମ୍ପ୍ଲିଫାୟର୍ ବ୍ୟାକ୍-ଅଫ୍ ହୁଏ, ଯାହା ଦୁର୍ବଳ ସିଗନାଲ୍ ପ୍ରସାରଣକୁ ପ୍ରଭାବିତ କରେ।",
        'lq8_summary': "<strong>ନିର୍ଦ୍ଦେଶିତ ବିନିମୟ:</strong> 45 ସେକେଣ୍ଡ (2ଟି QSO ପାଇଁ 3ଟି ସ୍ଲଟ୍) &mdash; 160 QSOs/h। <span style=\"color: var(--emerald); font-weight: 700;\">କ୍ରମାଗତ ପାଇଲ୍ଅପ୍‌ରେ ଯେଉଁଠାରେ ବାର୍ତ୍ତା 3 ନୂତନ କଲ୍ ସହିତ ଓଭରଲ୍ଯାପ୍ ହୁଏ: ପ୍ରତି 30 ସେକେଣ୍ଡରେ 2ଟି QSO = 240 QSOs/h (ସିଙ୍ଗଲ୍ କ୍ୟାରିଅର୍ FT8 ଠାରୁ 4×) 0.0 dB ପାୱାର୍ ପେନାଲ୍ଟି ସହିତ!</span>",
        'meta_speed': "ସିଙ୍ଗଲ୍ କ୍ୟାରିଅର୍ ସହିତ 4× ପାଇଲ୍ଅପ୍ ଗତି (240 ବନାମ 60 QSOs/h)"
    },
    'my': {
        'hero_title_2': "single-carrier ဖြင့် 4× pileup အမြန်နှုန်း (240 vs 60 QSOs/h)",
        'badge': "4× Pileup အမြန်နှုန်း",
        'stat_sub': "Single-Carrier (240 vs 60/h)",
        'desc': "ဆက်တိုက် pileup တွင် slot 3 သည် split frequencies ပေါ်ရှိ ခေါ်ဆိုသူအသစ်များနှင့် ထပ်တူကျပြီး 0.0 dB power penalty ဖြင့် single carrier ပေါ်တွင် <strong>စက္ကန့် ၃၀ တိုင်း QSO ၂ ခု (240 QSOs/h၊ single-carrier FT8 ထက် ၄ ဆ)</strong> ပြီးမြောက်စေသည် (power split ဖြင့် FT8 Fox &amp; Hound ၏ 60–120 QSOs/h နှင့် နှိုင်းယှဉ်ပါက)၊ dual-carrier ဖြင့် <strong>480 QSOs/h</strong> အထိ တိုးမြှင့်နိုင်သည်!",
        'li_1': "240 QSOs/h (carrier ၁ ခု၊ 50 Hz၊ 0 dB)",
        'li_2': "480 QSOs/h (carrier ၂ ခု၊ 100 Hz)",
        'ft8_summary': "<strong>လမ်းညွှန်ချက် ဖလှယ်မှု:</strong> ၁၀၅ စက္ကန့် (QSO ၂ ခုအတွက် slot ၇ ခု) &mdash; ~69 QSOs/h။ မက်ဆေ့ချ် ၇ သည် ခေါ်ဆိုမှုအသစ်များနှင့် ထပ်တူကျသည့် ဆက်တိုက် pileup တွင် စက်ဝန်းသည် 90s (slot ၆ ခု) = 80 QSOs/h ဖြစ်သည် (2-stream ဖြင့် 120 QSOs/h အထိ၊ single-carrier တွင် 60 QSOs/h)။ Fox &amp; Hound ရှိ multi-stream သည် ပြင်းထန်သော power-splitting ဆုံးရှုံးမှုများ (&minus;3.0 မှ &minus;7.0 dB) နှင့် amplifier back-off ကို ဖြစ်ပေါ်စေပြီး အားနည်းသော အချက်ပြမှုများကို ထိခိုက်စေသည်။",
        'lq8_summary': "<strong>လမ်းညွှန်ချက် ဖလှယ်မှု:</strong> ၄၅ စက္ကန့် (QSO ၂ ခုအတွက် slot ၃ ခု) &mdash; 160 QSOs/h။ <span style=\"color: var(--emerald); font-weight: 700;\">မက်ဆေ့ချ် ၃ သည် ခေါ်ဆိုမှုအသစ်များနှင့် ထပ်တူကျသည့် ဆက်တိုက် pileup တွင်: စက္ကန့် ၃၀ တိုင်း QSO ၂ ခု = 240 QSOs/h (single-carrier FT8 ထက် ၄ ဆ) 0.0 dB power penalty ဖြင့်!</span>",
        'meta_speed': "single-carrier ဖြင့် 4× pileup အမြန်နှုန်း (240 vs 60 QSOs/h)"
    },
    'tl': {
        'hero_title_2': "4× Bilis ng Pileup sa Single-Carrier (240 vs 60 QSOs/h)",
        'badge': "4× Bilis ng Pileup",
        'stat_sub': "Single-Carrier (240 vs 60/h)",
        'desc': "Sa tuluy-tuloy na pileup, ang slot 3 ay sumasapaw sa mga bagong tumatawag sa split frequencies, kinukumpleto ang <strong>2 QSO bawat 30 segundo (240 QSOs/h, 4× kaysa sa single-carrier FT8)</strong> sa iisang carrier na may 0.0 dB power penalty (kumpara sa 60–120 QSOs/h sa FT8 Fox &amp; Hound na may power split), umaabot sa <strong>480 QSOs/h</strong> sa dual-carrier!",
        'li_1': "240 QSOs/h (1 carrier, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 carriers, 100 Hz)",
        'ft8_summary': "<strong>Direktang Palitan:</strong> 105 segundo (7 slot para sa 2 QSO) &mdash; ~69 QSOs/h. Sa tuluy-tuloy na pileup kung saan sumasapaw ang mensahe 7 sa mga bagong tawag, ang cycle ay 90s (6 na slot) = 80 QSOs/h (hanggang 120 QSOs/h sa 2-stream interleaving; 60 QSOs/h sa single-carrier). Ang multi-stream parallelization sa Fox &amp; Hound ay nagdudulot ng matinding pagbawas sa power splitting (&minus;3.0 hanggang &minus;7.0 dB) at amplifier back-off, na nagpapahina sa abot ng mahihinang signal.",
        'lq8_summary': "<strong>Direktang Palitan:</strong> 45 segundo (3 slot para sa 2 QSO) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">Sa tuluy-tuloy na pileup kung saan sumasapaw ang mensahe 3 sa mga bagong tawag: 2 QSO bawat 30 segundo = 240 QSOs/h (4× kaysa sa single-carrier FT8) na may 0.0 dB power penalty!</span>",
        'meta_speed': "4× bilis ng pileup sa single-carrier (240 vs 60 QSOs/h)"
    },
    'jv': {
        'hero_title_2': "Kacepetan Pileup 4× nganggo Carrier Tunggal (240 vs 60 QSOs/jam)",
        'badge': "Kacepetan Pileup 4×",
        'stat_sub': "Carrier Tunggal (240 vs 60/jam)",
        'desc': "Ing pileup terus-terusan, slot 3 tumpang tindih karo panelpon anyar ing frekuensi split, ngrampungake <strong>2 QSO saben 30 detik (240 QSO/jam, 4× tinimbang FT8 carrier tunggal)</strong> ing carrier tunggal kanthi 0.0 dB paukuman daya (vs 60–120 QSO/jam ing FT8 Fox &amp; Hound kanthi pamisahan daya), munggah nganti <strong>480 QSO/jam</strong> nganggo carrier ganda!",
        'li_1': "240 QSO/jam (1 carrier, 50 Hz, 0 dB)",
        'li_2': "480 QSO/jam (2 carrier, 100 Hz)",
        'ft8_summary': "<strong>Ijol-ijolan Katuntun:</strong> 105 detik (7 slot kanggo 2 QSO) &mdash; ~69 QSO/jam. Ing pileup terus-terusan nalika pesen 7 tumpang tindih karo telpon anyar, sikluse yaiku 90s (6 slot) = 80 QSO/jam (nganti 120 QSO/jam kanthi 2-stream; 60 QSO/jam ing carrier tunggal). Multi-stream ing Fox &amp; Hound nyebabake mundhake pamisahan daya (&minus;3.0 nganti &minus;7.0 dB) lan amplifier back-off, ngrusak jangkauan sinyal sing ringkih.",
        'lq8_summary': "<strong>Ijol-ijolan Katuntun:</strong> 45 detik (3 slot kanggo 2 QSO) &mdash; 160 QSO/jam. <span style=\"color: var(--emerald); font-weight: 700;\">Ing pileup terus-terusan nalika pesen 3 tumpang tindih karo telpon anyar: 2 QSO saben 30 detik = 240 QSO/jam (4× tinimbang FT8 carrier tunggal) kanthi 0.0 dB paukuman daya!</span>",
        'meta_speed': "kacepetan pileup 4× nganggo carrier tunggal (240 vs 60 QSOs/jam)"
    },
    'su': {
        'hero_title_2': "Laju Pileup 4× nganggo Carrier Tunggal (240 vs 60 QSOs/jam)",
        'badge': "Laju Pileup 4×",
        'stat_sub': "Carrier Tunggal (240 vs 60/jam)",
        'desc': "Dina pileup terus-terusan, slot 3 tumpang tindih sareng nu nelepon anyar dina frekuensi split, ngabéréskeun <strong>2 QSO unggal 30 detik (240 QSO/jam, 4× ti FT8 carrier tunggal)</strong> dina hiji carrier kalayan 0.0 dB pinalti daya (vs 60–120 QSO/jam dina FT8 Fox &amp; Hound kalayan pamisahan daya), naék dugi ka <strong>480 QSO/jam</strong> nganggo carrier ganda!",
        'li_1': "240 QSO/jam (1 carrier, 50 Hz, 0 dB)",
        'li_2': "480 QSO/jam (2 carrier, 100 Hz)",
        'ft8_summary': "<strong>Tukeuran Kaarah:</strong> 105 detik (7 slot pikeun 2 QSO) &mdash; ~69 QSO/jam. Dina pileup terus-terusan dimana pesen 7 tumpang tindih sareng telepon anyar, siklusna nyaéta 90s (6 slot) = 80 QSO/jam (nepi ka 120 QSO/jam kalayan 2-stream; 60 QSO/jam carrier tunggal). Multi-stream di Fox &amp; Hound ngabalukarkeun karugian pamisahan daya (&minus;3.0 dugi ka &minus;7.0 dB) sareng back-off amplifier, ngaruksak jangkauan sinyal lemah.",
        'lq8_summary': "<strong>Tukeuran Kaarah:</strong> 45 detik (3 slot pikeun 2 QSO) &mdash; 160 QSO/jam. <span style=\"color: var(--emerald); font-weight: 700;\">Dina pileup terus-terusan dimana pesen 3 tumpang tindih sareng telepon anyar: 2 QSO unggal 30 detik = 240 QSO/jam (4× ti FT8 carrier tunggal) kalayan 0.0 dB pinalti daya!</span>",
        'meta_speed': "laju pileup 4× nganggo carrier tunggal (240 vs 60 QSOs/jam)"
    },
    'bho': {
        'hero_title_2': "सिंगल-कैरियर के साथ 4× पाइलअप गति (240 बनाम 60 QSOs/h)",
        'badge': "4× पाइलअप गति",
        'stat_sub': "सिंगल-कैरियर (240 बनाम 60/h)",
        'desc': "लगातार पाइलअप में, स्लॉट 3 स्प्लिट आवृत्तियों पर नया कॉलर्स के साथ ओवरलैप होला, जेहसे 0.0 dB पावर पेनल्टी के साथे सिंगल कैरियर पर <strong>हर 30 सेकंड में 2 गो QSO (240 QSOs/h, सिंगल-कैरियर FT8 से 4×)</strong> पूरा होला (पावर स्प्लिट वाला FT8 Fox &amp; Hound के 60–120 QSOs/h के मुकाबले), जवन डुअल-कैरियर में <strong>480 QSOs/h</strong> तक पहुँच जाला!",
        'li_1': "240 QSOs/h (1 कैरियर, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 कैरियर, 100 Hz)",
        'ft8_summary': "<strong>निर्देशित आदान-प्रदान:</strong> 105 सेकंड (2 QSO खातिर 7 स्लॉट) &mdash; ~69 QSOs/h। लगातार पाइलअप में जहाँ संदेश 7 नया कॉल के साथ ओवरलैप होला, चक्र 90s (6 स्लॉट) = 80 QSOs/h होला (2-स्ट्रीम के साथ 120 QSOs/h तक; सिंगल कैरियर 60 QSOs/h)। Fox &amp; Hound में मल्टी-स्ट्रीम पैरेललाइजेशन से भारी पावर-स्प्लिट नुकसान (&minus;3.0 से &minus;7.0 dB) आ एम्पलीफायर बैक-ऑफ होला, जेहसे कमजोर सिग्नल के पहुँच प्रभावित होला।",
        'lq8_summary': "<strong>निर्देशित आदान-प्रदान:</strong> 45 सेकंड (2 QSO खातिर 3 स्लॉट) &mdash; 160 QSOs/h। <span style=\"color: var(--emerald); font-weight: 700;\">लगातार पाइलअप में जहाँ संदेश 3 नया कॉल के साथ ओवरलैप होला: हर 30 सेकंड में 2 गो QSO = 240 QSOs/h (सिंगल-कैरियर FT8 से 4×) 0.0 dB पावर पेनल्टी के साथे!</span>",
        'meta_speed': "सिंगल-कैरियर के साथ 4× पाइलअप गति (240 बनाम 60 QSOs/h)"
    },
    'pcm': {
        'hero_title_2': "4× Pileup Speed with Single-Carrier (240 vs 60 QSOs/h)",
        'badge': "4× Pileup Speed",
        'stat_sub': "Single-Carrier (240 vs 60/h)",
        'desc': "For continuous pileup, slot 3 dey overlap with new callers on split frequencies, completing <strong>2 QSOs every 30 seconds (240 QSOs/h, 4× over single-carrier FT8)</strong> on single carrier with 0.0 dB power penalty (vs 60–120 QSOs/h for FT8 Fox &amp; Hound with power split), scaling reach <strong>480 QSOs/h</strong> with dual-carrier!",
        'li_1': "240 QSOs/h (1 carrier, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (2 carriers, 100 Hz)",
        'ft8_summary': "<strong>Directed Exchange:</strong> 105 seconds (7 slots for 2 QSOs) &mdash; ~69 QSOs/h. For continuous pileup where message 7 dey overlap with new calls, the cycle na 90s (6 slots) = 80 QSOs/h (up to 120 QSOs/h with 2 streams; 60 QSOs/h for single-carrier). Multi-stream parallelization inside Fox &amp; Hound dey cause heavy power-splitting losses (&minus;3.0 to &minus;7.0 dB) and amplifier back-off, spoiling reach for weak signals.",
        'lq8_summary': "<strong>Directed Exchange:</strong> 45 seconds (3 slots for 2 QSOs) &mdash; 160 QSOs/h. <span style=\"color: var(--emerald); font-weight: 700;\">For continuous pileup where message 3 dey overlap with new calls: 2 QSOs every 30 seconds = 240 QSOs/h (4× over single-carrier FT8) with 0.0 dB power penalty!</span>",
        'meta_speed': "4× pileup speed with single-carrier (240 vs 60 QSOs/h)"
    },
    'wuu': {
        'hero_title_2': "单载波 4× 堆叠通联速度 (240 vs 60 QSOs/h)",
        'badge': "4× 堆叠速度",
        'stat_sub': "单载波 (240 vs 60/h)",
        'desc': "勒连续堆叠通联当中，第3个时隙搭分频呼叫个新电台重叠，单载波以 0.0 dB 功率损耗实现<strong>每30秒完成2个QSO (240 QSOs/h，是单载波 FT8 个4倍)</strong>（相比 FT8 Fox &amp; Hound 严重分流下底个 60–120 QSOs/h），双载波更可以扩展到 <strong>480 QSOs/h</strong>！",
        'li_1': "240 QSOs/h (单载波, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (双载波, 100 Hz)",
        'ft8_summary': "<strong>定向交换：</strong>105 秒（7 个时隙做 2 个 QSO）&mdash; 约 69 QSOs/h。勒消息 7 搭新呼叫重叠个连续堆叠里向，周期是 90 秒（6 个时隙）= 80 QSOs/h（双流交织顶多 120 QSOs/h；单载波是 60 QSOs/h）。Fox &amp; Hound 个多流并行会导致严重个功率分流损耗（&minus;3.0 到 &minus;7.0 dB）外加功率回退，严重削弱弱信号个传播。",
        'lq8_summary': "<strong>定向交换：</strong>45 秒（3 个时隙做 2 个 QSO）&mdash; 160 QSOs/h。<span style=\"color: var(--emerald); font-weight: 700;\">勒消息 3 搭新呼叫重叠个连续堆叠里向：每 30 秒完成 2 个 QSO = 240 QSOs/h（单载波 FT8 个 4 倍），0.0 dB 功率损耗！</span>",
        'meta_speed': "单载波 4× 堆叠速度 (240 vs 60 QSOs/h)"
    },
    'yue': {
        'hero_title_2': "單載波 4× 堆疊通聯速度 (240 vs 60 QSOs/h)",
        'badge': "4× 堆疊速度",
        'stat_sub': "單載波 (240 vs 60/h)",
        'desc': "喺連續堆疊通聯入面，第3個時隙同分頻呼叫嘅新電台重疊，單載波喺 0.0 dB 功率損耗下達到<strong>每30秒完成2個QSO (240 QSOs/h，係單載波 FT8 嘅4倍)</strong>（對比 FT8 Fox &amp; Hound 嚴重功率分流下嘅 60–120 QSOs/h），雙載波更可擴展至 <strong>480 QSOs/h</strong>！",
        'li_1': "240 QSOs/h (單載波, 50 Hz, 0 dB)",
        'li_2': "480 QSOs/h (雙載波, 100 Hz)",
        'ft8_summary': "<strong>定向交換：</strong>105 秒（7 個時隙完成 2 個 QSO）&mdash; 約 69 QSOs/h。喺消息 7 同新呼叫重疊嘅連續堆疊入面，週期係 90 秒（6 個時隙）= 80 QSOs/h（雙流交織最高 120 QSOs/h；單載波為 60 QSOs/h）。Fox &amp; Hound 嘅多流並行會造成嚴重嘅功率分流損耗（&minus;3.0 至 &minus;7.0 dB）同埋功放回退，大幅削弱弱信號嘅傳播。",
        'lq8_summary': "<strong>定向交換：</strong>45 秒（3 個時隙完成 2 個 QSO）&mdash; 160 QSOs/h。<span style=\"color: var(--emerald); font-weight: 700;\">喺消息 3 同新呼叫重疊嘅連續堆疊入面：每 30 秒完成 2 個 QSO = 240 QSOs/h（單載波 FT8 嘅 4 倍），具備 0.0 dB 功率損耗！</span>",
        'meta_speed': "單載波 4× 堆疊速度 (240 vs 60 QSOs/h)"
    },
    'apc': {
        'hero_title_2': "سرعة بايل أب 4× بحامل فردي (240 مقابل 60 QSO/ساعة)",
        'badge': "سرعة بايل أب 4×",
        'stat_sub': "حامل فردي (240 مقابل 60/ساعة)",
        'desc': "بالبايل أب المستمر، الفترة 3 بتتداخل مع المتصلين الجداد عالترددات المقسمة لتنجز <strong>اتصالين كل 30 ثانية (240 QSO/ساعة، 4× مقارنة بـ FT8 بحامل فردي)</strong> على حامل إشارة واحد بدون أي فقد بالقدرة (مقابل 60–120 QSO/ساعة بـ FT8 Fox &amp; Hound مع تجزئة القدرة)، وبتوصل لـ <strong>480 QSO/ساعة</strong> بحاملين!",
        'li_1': "240 QSO/ساعة (حامل واحد، 50 هرتز، 0 ديسيبل)",
        'li_2': "480 QSO/ساعة (حاملان، 100 هرتز)",
        'ft8_summary': "<strong>التبادل المباشر:</strong> 105 ثانية (7 فترات لاتصالين) &mdash; ~69 QSO/ساعة. بالبايل أب المستمر لما الرسالة 7 بتتداخل مع الاتصالات الجديدة، الدورة بتصير 90 ثانية (6 فترات) = 80 QSO/ساعة (بتوصل لـ 120 QSO/ساعة بمسارين؛ 60 بحامل فردي). التوازي بـ Fox &amp; Hound بيخسر قدرة كبيرة (&minus;3.0 لـ &minus;7.0 ديسيبل) وبيحتاج تقليل طاقة المضخم، وهاد بيضعف وصول الإشارات الضعيفة.",
        'lq8_summary': "<strong>التبادل المباشر:</strong> 45 ثانية (3 فترات لاتصالين) &mdash; 160 QSO/ساعة. <span style=\"color: var(--emerald); font-weight: 700;\">بالبايل أب المستمر لما الرسالة 3 بتتداخل مع الاتصالات الجديدة: اتصالين كل 30 ثانية = 240 QSO/ساعة (4× مقارنة بـ FT8 بحامل فردي) بدون أي فقد بالقدرة!</span>",
        'meta_speed': "سرعة بايل أب 4× بحامل فردي (240 مقابل 60 QSO/ساعة)"
    },
    'apd': {
        'hero_title_2': "سرعة بايل أب 4× بحامل فردي (240 مقابل 60 QSO/ساعة)",
        'badge': "سرعة بايل أب 4×",
        'stat_sub': "حامل فردي (240 مقابل 60/ساعة)",
        'desc': "في البايل أب المستمر، الفترة 3 بتتداخل مع المتصلين الجداد في الترددات المقسمة عشان تنجز <strong>اتصالين كل 30 ثانية (240 QSO/ساعة، 4× مقارنة بـ FT8 بحامل فردي)</strong> على حامل واحد بدون أي فقد في القدرة (مقابل 60–120 QSO/ساعة في FT8 Fox &amp; Hound مع تجزئة القدرة)، وبتصل لـ <strong>480 QSO/ساعة</strong> بحاملين!",
        'li_1': "240 QSO/ساعة (حامل واحد، 50 هرتز، 0 ديسيبل)",
        'li_2': "480 QSO/ساعة (حاملان، 100 هرتز)",
        'ft8_summary': "<strong>التبادل المباشر:</strong> 105 ثانية (7 فترات لاتصالين) &mdash; ~69 QSO/ساعة. في البايل أب المستمر لما الرسالة 7 تتداخل مع الاتصالات الجديدة، الدورة بتصبح 90 ثانية (6 فترات) = 80 QSO/ساعة (بتصل لـ 120 QSO/ساعة بمسارين؛ 60 بحامل فردي). التوازي بـ Fox &amp; Hound بيفقد قدرة كبيرة (&minus;3.0 إلى &minus;7.0 ديسيبل) وبيحتاج تقليل طاقة المضخم، وده بيأثر على انتشار الإشارات الضعيفة.",
        'lq8_summary': "<strong>التبادل المباشر:</strong> 45 ثانية (3 فترات لاتصالين) &mdash; 160 QSO/ساعة. <span style=\"color: var(--emerald); font-weight: 700;\">في البايل أب المستمر لما الرسالة 3 تتداخل مع الاتصالات الجديدة: اتصالين كل 30 ثانية = 240 QSO/ساعة (4× مقارنة بـ FT8 بحامل فردي) بدون أي فقد في القدرة!</span>",
        'meta_speed': "سرعة بايل أب 4× بحامل فردي (240 مقابل 60 QSO/ساعة)"
    },
    'arq': {
        'hero_title_2': "سرعة بايل أب 4× بحامل فردي (240 مقابل 60 QSO/ساعة)",
        'badge': "سرعة بايل أب 4×",
        'stat_sub': "حامل فردي (240 مقابل 60/ساعة)",
        'desc': "في البايل أب المستمر، الفترة 3 تتداخل مع المتصلين الجدد فالترددات المقسمة باش تكمل <strong>اتصالين كل 30 ثانية (240 QSO/ساعة، 4× مقارنة بـ FT8 بحامل فردي)</strong> على حامل واحد بلا حتى ضياع فالقدرة (مقابل 60–120 QSO/ساعة في FT8 Fox &amp; Hound مع تقسيم القدرة)، وتوصل لـ <strong>480 QSO/ساعة</strong> بحاملين!",
        'li_1': "240 QSO/ساعة (حامل واحد، 50 هرتز، 0 ديسيبل)",
        'li_2': "480 QSO/ساعة (حاملان، 100 هرتز)",
        'ft8_summary': "<strong>التبادل المباشر:</strong> 105 ثانية (7 فترات لاتصالين) &mdash; ~69 QSO/ساعة. في البايل أب المستمر كي الرسالة 7 تتداخل مع الاتصالات الجديدة، الدورة تولي 90 ثانية (6 فترات) = 80 QSO/ساعة (توصل لـ 120 QSO/ساعة بزوج مسارات؛ 60 بحامل فردي). التوازي بـ Fox &amp; Hound يضيع قدرة كبيرة (&minus;3.0 حتى &minus;7.0 ديسيبل) وينقص من قوة المضخم، وهاد الشي يضر وصول الإشارات الضعيفة.",
        'lq8_summary': "<strong>التبادل المباشر:</strong> 45 ثانية (3 فترات لاتصالين) &mdash; 160 QSO/ساعة. <span style=\"color: var(--emerald); font-weight: 700;\">في البايل أب المستمر كي الرسالة 3 تتداخل مع الاتصالات الجديدة: اتصالين كل 30 ثانية = 240 QSO/ساعة (4× مقارنة بـ FT8 بحامل فردي) بلا حتى ضياع فالقدرة!</span>",
        'meta_speed': "سرعة بايل أب 4× بحامل فردي (240 مقابل 60 QSO/ساعة)"
    },
    'ary': {
        'hero_title_2': "سرعة بايل أب 4× بحامل فردي (240 مقابل 60 QSO/ساعة)",
        'badge': "سرعة بايل أب 4×",
        'stat_sub': "حامل فردي (240 مقابل 60/ساعة)",
        'desc': "فالبايل أب المستمر، الفترة 3 كترتابط مع المتصلين الجداد فالترددات المقسمة باش تكمل <strong>اتصالين كل 30 ثانية (240 QSO/ساعة، 4× مقارنة بـ FT8 بحامل فردي)</strong> على حامل واحد بلا ما تضيع حتى طاقة (مقابل 60–120 QSO/ساعة فـ FT8 Fox &amp; Hound مع تقسيم الطاقة)، وكتوصل لـ <strong>480 QSO/ساعة</strong> بحاملين!",
        'li_1': "240 QSO/ساعة (حامل واحد، 50 هرتز، 0 ديسيبل)",
        'li_2': "480 QSO/ساعة (حاملان، 100 هرتز)",
        'ft8_summary': "<strong>التبادل المباشر:</strong> 105 ثانية (7 فترات لـ 2 QSO) &mdash; ~69 QSO/ساعة. فالبايل أب المستمر فاش الرسالة 7 كتداخل مع الاتصالات الجديدة، الدورة كتولي 90 ثانية (6 فترات) = 80 QSO/ساعة (كتوصل لـ 120 QSO/ساعة بمسارين؛ 60 بحامل فردي). التوازي فـ Fox &amp; Hound كيخسر طاقة كبيرة (&minus;3.0 حتى &minus;7.0 ديسيبل) وكيحتاج تنقيص طاقة المضخم، هادشي كيأثر على وصول الإشارات الضعيفة.",
        'lq8_summary': "<strong>التبادل المباشر:</strong> 45 ثانية (3 فترات لـ 2 QSO) &mdash; 160 QSO/ساعة. <span style=\"color: var(--emerald); font-weight: 700;\">فالبايل أب المستمر فاش الرسالة 3 كتداخل مع الاتصالات الجديدة: اتصالين كل 30 ثانية = 240 QSO/ساعة (4× مقارنة بـ FT8 بحامل فردي) بلا حتى ضياع فالطاقة!</span>",
        'meta_speed': "سرعة بايل أب 4× بحامل فردي (240 مقابل 60 QSO/ساعة)"
    },
    'arz': {
        'hero_title_2': "سرعة بايل أب 4× بحامل فردي (240 مقابل 60 QSO/ساعة)",
        'badge': "سرعة بايل أب 4×",
        'stat_sub': "حامل فردي (240 مقابل 60/ساعة)",
        'desc': "في البايل أب المستمر، الفترة 3 بتتداخل مع المتصلين الجداد في الترددات المقسمة عشان تخلص <strong>اتصالين كل 30 ثانية (240 QSO/ساعة، 4× مقارنة بـ FT8 بحامل فردي)</strong> على حامل واحد من غير أي فقد في الباور (مقابل 60–120 QSO/ساعة في FT8 Fox &amp; Hound مع تجزئة الباور)، وبتوصل لـ <strong>480 QSO/ساعة</strong> بحاملين!",
        'li_1': "240 QSO/ساعة (حامل واحد، 50 هرتز، 0 ديسيبل)",
        'li_2': "480 QSO/ساعة (حاملان، 100 هرتز)",
        'ft8_summary': "<strong>التبادل المباشر:</strong> 105 ثانية (7 فترات لاتصالين) &mdash; ~69 QSO/ساعة. في البايل أب المستمر لما الرسالة 7 تتداخل مع الاتصالات الجديدة، الدورة بتبقى 90 ثانية (6 فترات) = 80 QSO/ساعة (بتوصل لـ 120 QSO/ساعة بمسارين؛ 60 بحامل فردي). التوازي في Fox &amp; Hound بيخسر باور كبيرة (&minus;3.0 إلى &minus;7.0 ديسيبل) وبيحتاج تخفيض باور الأمبليفاير، وده بيأثر على انتشار الإشارات الضعيفة.",
        'lq8_summary': "<strong>التبادل المباشر:</strong> 45 ثانية (3 فترات لاتصالين) &mdash; 160 QSO/ساعة. <span style=\"color: var(--emerald); font-weight: 700;\">في البايل أب المستمر لما الرسالة 3 تتداخل مع الاتصالات الجديدة: اتصالين كل 30 ثانية = 240 QSO/ساعة (4× مقارنة بـ FT8 بحامل فردي) من غير أي فقد في الباور!</span>",
        'meta_speed': "سرعة بايل أب 4× بحامل فردي (240 مقابل 60 QSO/ساعة)"
    }
}


def update_language_file(lang, t):
    file_path = os.path.join(LANG_DIR, f"lang{lang}.js")
    if not os.path.exists(file_path):
        print(f"Warning: File not found: {file_path}")
        return False

    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # 1. Update HERO_TITLE_2
    h2_val = json.dumps(t['hero_title_2'], ensure_ascii=False)
    content = re.sub(r'([ \t]*"HERO_TITLE_2":)[^\n]*', lambda m: f'{m.group(1)} {h2_val},', content)

    # 2. Update PITCH_CARD_2_BADGE
    badge_val = json.dumps(t['badge'], ensure_ascii=False)
    content = re.sub(r'([ \t]*"PITCH_CARD_2_BADGE":)[^\n]*', lambda m: f'{m.group(1)} {badge_val},', content)

    # 3. Update PITCH_CARD_2_STAT_MAIN
    content = re.sub(r'([ \t]*"PITCH_CARD_2_STAT_MAIN":)[^\n]*', lambda m: f'{m.group(1)} "4.0×",', content)

    # 4. Update PITCH_CARD_2_STAT_SUB
    sub_val = json.dumps(t['stat_sub'], ensure_ascii=False)
    content = re.sub(r'([ \t]*"PITCH_CARD_2_STAT_SUB":)[^\n]*', lambda m: f'{m.group(1)} {sub_val},', content)

    # 5. Update PITCH_CARD_2_DESC
    desc_val = json.dumps(t['desc'], ensure_ascii=False)
    content = re.sub(r'([ \t]*"PITCH_CARD_2_DESC":)[^\n]*', lambda m: f'{m.group(1)} {desc_val},', content)

    # 6. Update PITCH_CARD_2_LI_1
    li1_val = json.dumps(t['li_1'], ensure_ascii=False)
    content = re.sub(r'([ \t]*"PITCH_CARD_2_LI_1":)[^\n]*', lambda m: f'{m.group(1)} {li1_val},', content)

    # 7. Update PITCH_CARD_2_LI_2
    li2_val = json.dumps(t['li_2'], ensure_ascii=False)
    content = re.sub(r'([ \t]*"PITCH_CARD_2_LI_2":)[^\n]*', lambda m: f'{m.group(1)} {li2_val},', content)

    # 8. Update SPEEDUP_PL_FT8_SUMMARY
    ft8_val = json.dumps(t['ft8_summary'], ensure_ascii=False)
    content = re.sub(r'([ \t]*"SPEEDUP_PL_FT8_SUMMARY":)[^\n]*', lambda m: f'{m.group(1)} {ft8_val},', content)

    # 9. Update SPEEDUP_PL_LQ8_SUMMARY
    lq8_val = json.dumps(t['lq8_summary'], ensure_ascii=False)
    content = re.sub(r'([ \t]*"SPEEDUP_PL_LQ8_SUMMARY":)[^\n]*', lambda m: f'{m.group(1)} {lq8_val},', content)

    # 10. Update meta descriptions: replace >2.3× or similar pileup text with meta_speed
    meta_speed = t['meta_speed']
    for meta_key in ['META_DESC', 'OG_DESC', 'TWITTER_DESC']:
        m = re.search(r'"' + meta_key + r'":\s*"([^"]*)"', content)
        if m:
            val = m.group(1)
            new_val = re.sub(r'(?:>|&gt;)?\s*2\.3[0-9]*\s*[×xX][^,()]*\([0-9]+\s*[^)]*69[^)]*\)', meta_speed, val)
            new_val = re.sub(r'(?:>|&gt;)?\s*2[.,]3[0-9]*\s*[×xX][^,()]*\([0-9]+\s*[^)]*\)', meta_speed, new_val)
            val_json = json.dumps(new_val, ensure_ascii=False)
            content = re.sub(r'([ \t]*"' + meta_key + r'":)[^\n]*', lambda m: f'{m.group(1)} {val_json},', content)

    # 11. Update HERO_SUBTITLE: replace 320 QSOs/h with 480 QSOs/h and 160 with 240
    m_sub = re.search(r'"HERO_SUBTITLE":\s*"([^"]*)"', content)
    if m_sub:
        val = m_sub.group(1)
        new_val = val.replace('320', '480')
        new_val = re.sub(r'160\s*QSOs?/h[^)]*\)', f'240 QSOs/h ({t["hero_title_2"]})', new_val)
        val_json = json.dumps(new_val, ensure_ascii=False)
        content = re.sub(r'([ \t]*"HERO_SUBTITLE":)[^\n]*', lambda m: f'{m.group(1)} {val_json},', content)

    with open(file_path, "w", encoding="utf-8") as f:
        f.write(content)

    return True


def main():
    print("================================================================================")
    print("  LQ Digital Mode — Updating Pileup Speed Translations (63 Languages)")
    print("================================================================================")
    updated_count = 0

    for lang, t in TRANSLATIONS.items():
        if update_language_file(lang, t):
            updated_count += 1
            print(f"  [{lang:>3}] Updated pileup keys in lang{lang}.js")

    print(f"\n✓ Successfully updated {updated_count} language dictionary files.")


if __name__ == "__main__":
    main()
