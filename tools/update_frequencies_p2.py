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
update_frequencies_p2.py

Updates FREQUENCIES_BOX_P2 across all 63 language files in docs/languages/lang*.js
to accurately describe 30m as the only band with no voice phone operation allowed,
and 17m/12m as contest-free WARC spectrum.
"""

import os
import re

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DOCS_DIR = os.path.join(BASE_DIR, "docs")
LANG_DIR = os.path.join(DOCS_DIR, "languages")

P2_TRANSLATIONS = {
    'en': (
        "These selected bands provide optimal, noise-free propagation environments: "
        "<strong>30m</strong> is a narrow-band WARC allocation with no voice phone operation allowed by international law; "
        "<strong>17m and 12m</strong> offer contest-free WARC spectrum, while "
        "<strong>40m, 20m, 15m, and 10m</strong> provide expansive, quiet digital allocations well separated from their respective voice phone segments."
    ),
    'es': (
        "Estas bandas seleccionadas proporcionan entornos de propagación óptimos y libres de ruido: "
        "<strong>30m</strong> es una asignación WARC de banda estrecha sin operación de fonía permitida por la legislación internacional; "
        "<strong>17m y 12m</strong> ofrecen espectro WARC libre de concursos, mientras que "
        "<strong>40m, 20m, 15m y 10m</strong> proporcionan amplios y tranquilos segmentos digitales bien separados de sus respectivas secciones de fonía."
    ),
    'pt': (
        "Essas bandas selecionadas oferecem ambientes de propagação ideais e silenciosos: "
        "<strong>30m</strong> é uma alocação WARC de banda estreita sem operação de fonia permitida pela legislação internacional; "
        "<strong>17m e 12m</strong> oferecem espectro WARC livre de concursos, enquanto "
        "<strong>40m, 20m, 15m e 10m</strong> fornecem faixas digitais amplas e silenciosas, bem separadas de seus respectivos segmentos de fonia."
    ),
    'fr': (
        "Ces bandes sélectionnées offrent des conditions de propagation optimales et silencieuses : "
        "le <strong>30m</strong> est une allocation WARC à bande étroite sans téléphonie vocale autorisée par la réglementation internationale ; "
        "le <strong>17m et 12m</strong> offrent un spectre WARC sans concours, tandis que "
        "le <strong>40m, 20m, 15m et 10m</strong> procurent de larges segments numériques très calmes, bien séparés de leurs plages de téléphonie respectives."
    ),
    'it': (
        "Queste bande selezionate offrono ambienti di propagazione ottimali e privi di rumore: "
        "i <strong>30m</strong> sono un'allocazione WARC a banda stretta in cui la fonia vocale è vietata dalle norme internazionali; "
        "i <strong>17m e 12m</strong> offrono spettro WARC privo di contest, mentre "
        "i <strong>40m, 20m, 15m e 10m</strong> mettono a disposizione ampi e silenziosi segmenti digitali ben separati dalle rispettive sezioni di fonia."
    ),
    'de': (
        "Diese ausgewählten Bänder bieten optimale, störungsfreie Ausbreitungsbedingungen: "
        "<strong>30m</strong> ist ein Schmalband-WARC-Bereich, auf dem Sprechfunk nach internationalem Recht verboten ist; "
        "<strong>17m und 12m</strong> bieten contestfreies WARC-Spektrum, während "
        "<strong>40m, 20m, 15m und 10m</strong> großzügige, ruhige Digitalbereiche weit abgesetzt von ihren jeweiligen Sprachfunksegmenten bereitstellen."
    ),
    'ru': (
        "Выбранные диапазоны обеспечивают оптимальные условия прохождения без помех: "
        "<strong>30м</strong> — это узкополосный диапазон WARC, где телефонная связь запрещена международными регламентами; "
        "<strong>17м и 12м</strong> предлагают спектр WARC, свободный от контестов, а "
        "<strong>40м, 20м, 15м и 10м</strong> предоставляют широкие и спокойные цифровые участки, надежно отделенные от голосовых сегментов."
    ),
    'zh': (
        "这些选定频段提供了优质、无噪的传播环境："
        "<strong>30m</strong> 是窄带 WARC 分配频段，国际法规严格禁止话音通信；"
        "<strong>17m 和 12m</strong> 提供无竞赛干扰的 WARC 频谱，而 "
        "<strong>40m、20m、15m 和 10m</strong> 则拥有宽广、宁静且与各自话音区段明确分离的数字模式分配区间。"
    ),
    'yue': (
        "呢啲選定頻段提供咗優質、無噪嘅傳播環境："
        "<strong>30m</strong> 係窄頻 WARC 分配頻段，國際法規嚴格禁止話音通聯；"
        "<strong>17m 同 12m</strong> 提供無競賽干擾嘅 WARC 頻譜，而 "
        "<strong>40m、20m、15m 同 10m</strong> 擁有寬廣、寧靜且同各自話音區段明確分開嘅數字模式分配區間。"
    ),
    'wuu': (
        "箇星选定频段传播条件好、噪声低："
        "<strong>30m</strong> 是窄带 WARC 分配频段，国际法规严禁语音通话；"
        "<strong>17m 跟 12m</strong> 提供无竞赛干扰个 WARC 频谱，而 "
        "<strong>40m、20m、15m 跟 10m</strong> 数字段宽广安静，与各自话音段明确分开。"
    ),
    'ja': (
        "選定されたバンドはノイズが極めて少なく最適な電波伝搬環境を提供します："
        "<strong>30m</strong> は国際規則により音声通話が禁止された狭帯域専用WARCバンドです。"
        "<strong>17m、12m</strong> はコンテストのない静穏なWARCスペクトラムを提供し、"
        "<strong>40m、20m、15m、10m</strong> は音声通信セグメントから明確に分離された広大で静寂なデジタル通信帯域を確保しています。"
    ),
    'ko': (
        "선정된 밴드들은 최적의 잡음 없는 전파 환경을 제공합니다: "
        "<strong>30m</strong>는 국제 규정에 의해 음성 통신이 금지된 협대역 전용 WARC 밴드이며, "
        "<strong>17m, 12m</strong>는 콘테스트 없는 쾌적한 WARC 스펙트럼을 제공하고, "
        "<strong>40m, 20m, 15m, 10m</strong>는 음성 세그먼트와 명확히 분리된 넓고 조용한 디지털 전용 영역을 제공합니다."
    ),
    'ar': (
        "توفر هذه النطاقات المختارة بيئات انتشار مثالية وخالية من الضوضاء: "
        "يعتبر نطاق <strong>30 متراً</strong> نطاق WARC ضيق النطاق مخصصاً يُحظر فيه تشغيل الصوت الهاتفي بموجب القانون الدولي؛ "
        "ويوفر نطاقا <strong>17 متراً و12 متراً</strong> طيف WARC خالياً من المسابقات، بينما يوفر نطاق "
        "<strong>40 متراً و20 متراً و15 متراً و10 أمتار</strong> تخصيصات رقمية هادئة وواسعة مفصولة تماماً عن قطاعات الصوت الهاتفي المقابلة لها."
    ),
    'apc': (
        "توفر هذه النطاقات المختارة بيئات انتشار مثالية وخالية من الضوضاء: "
        "يعتبر نطاق <strong>30 متر</strong> نطاق WARC ضيق يُحظر فيه الصوت الهاتفي بموجب القانون الدولي؛ "
        "ويوفر نطاق <strong>17 و12 متر</strong> طيف WARC خالي من المسابقات، بينما يوفر نطاق "
        "<strong>40 و20 و15 و10 متر</strong> تخصيصات رقمية هادئة وواسعة مفصولة تماماً عن قطاعات الصوت الهاتفي المقابلة لها."
    ),
    'apd': (
        "توفر هذه النطاقات المختارة بيئات انتشار مثالية وخالية من الضوضاء: "
        "يعتبر نطاق <strong>30 متراً</strong> نطاق WARC ضيقاً مخصصاً يُحظر فيه تشغيل الصوت الهاتفي بموجب القانون الدولي؛ "
        "ويوفر نطاقا <strong>17 متراً و12 متراً</strong> طيف WARC خالياً من المسابقات، بينما يوفر نطاق "
        "<strong>40 متراً و20 متراً و15 متراً و10 أمتار</strong> تخصيصات رقمية هادئة وواسعة مفصولة تماماً عن قطاعات الصوت الهاتفي المقابلة لها."
    ),
    'arq': (
        "توفر هذه النطاقات المختارة بيئات انتشار مثالية وخالية من الضوضاء: "
        "يعتبر نطاق <strong>30 متراً</strong> نطاق WARC ضيق النطاق مخصصاً يُحظر فيه تشغيل الصوت الهاتفي بموجب القانون الدولي؛ "
        "ويوفر نطاقا <strong>17 متراً و12 متراً</strong> طيف WARC خالياً من المسابقات، بينما يوفر نطاق "
        "<strong>40 متراً و20 متراً و15 متراً و10 أمتار</strong> تخصيصات رقمية هادئة وواسعة مفصولة تماماً عن قطاعات الصوت الهاتفي المقابلة لها."
    ),
    'ary': (
        "توفر هذه النطاقات المختارة بيئات انتشار مثالية وخالية من الضوضاء: "
        "يعتبر نطاق <strong>30 متراً</strong> نطاق WARC ضيق النطاق مخصصاً يُحظر فيه تشغيل الصوت الهاتفي بموجب القانون الدولي؛ "
        "ويوفر نطاقا <strong>17 متراً و12 متراً</strong> طيف WARC خالياً من المسابقات، بينما يوفر نطاق "
        "<strong>40 متراً و20 متراً و15 متراً و10 أمتار</strong> تخصيصات رقمية هادئة وواسعة مفصولة تماماً عن قطاعات الصوت الهاتفي المقابلة لها."
    ),
    'arz': (
        "توفر هذه النطاقات المختارة بيئات انتشار مثالية وخالية من الضوضاء: "
        "يعتبر نطاق <strong>30 متراً</strong> نطاق WARC ضيق النطاق مخصصاً يُحظر فيه تشغيل الصوت الهاتفي بموجب القانون الدولي؛ "
        "ويوفر نطاقا <strong>17 متراً و12 متراً</strong> طيف WARC خالياً من المسابقات، بينما يوفر نطاق "
        "<strong>40 متراً و20 متراً و15 متراً و10 أمتار</strong> تخصيصات رقمية هادئة وواسعة مفصولة تماماً عن قطاعات الصوت الهاتفي المقابلة لها."
    ),
    'fa': (
        "این باندهای منتخب شرایط انتشار بهینه و بدون نویزی را فراهم می‌کنند: "
        "باند <strong>30 متر</strong> یک تخصیص باریک‌باند WARC است که ارتباطات صوتی در آن طبق قوانین بین‌المللی ممنوع است؛ "
        "باندهای <strong>17 و 12 متر</strong> طیف WARC بدون مسابقه را فراهم می‌کنند، در حالی که باندهای "
        "<strong>40، 20، 15 و 10 متر</strong> بخش‌های دیجیتال آرام و وسیعی را کاملاً جدا از بخش‌های صوتی مربوطه ارائه می‌دهند."
    ),
    'he': (
        "פסים נבחרים אלה מספקים תנאי התפשטות אידיאליים ונקיים מרעש: "
        "פס <strong>30 מטר</strong> הוא פס WARC צר-סרט שבו שידור דיבור קולי אסור לפי החוק הבינלאומי; "
        "פסי <strong>17 מטר ו-12 מטר</strong> מציעים ספקטרום WARC ללא תחרויות, בעוד פסי "
        "<strong>40 מטר, 20 מטר, 15 מטר ו-10 מטר</strong> מציעים מקטעים דיגיטליים שקטים ונרחבים המופרדים היטב ממקטעי הדיבור."
    ),
    'hi': (
        "ये चयनित बैंड इष्टतम, शोर-मुक्त प्रसार वातावरण प्रदान करते हैं: "
        "<strong>30m</strong> एक संकीर्ण-बैंड WARC आवंटन है जहाँ अंतरराष्ट्रीय कानून द्वारा वॉयस फोन संचालन की अनुमति नहीं है; "
        "<strong>17m और 12m</strong> प्रतियोगिता-मुक्त WARC स्पेक्ट्रम प्रदान करते हैं, जबकि "
        "<strong>40m, 20m, 15m और 10m</strong> अपने संबंधित वॉयस फोन सेगमेंट से अच्छी तरह अलग शांत और विस्तृत डिजिटल स्पेक्ट्रम प्रदान करते हैं।"
    ),
    'bn': (
        "এই নির্বাচিত ব্যান্ডগুলি সর্বোত্তম ও কোলাহলমুক্ত প্রচারের পরিবেশ প্রদান করে: "
        "<strong>30m</strong> একটি সংকীর্ণ-ব্যান্ড WARC বরাদ্দ যেখানে আন্তর্জাতিক আইন অনুযায়ী ভয়েস ফোন অপারেশন নিষিদ্ধ; "
        "<strong>17m এবং 12m</strong> প্রতিযোগিতা-মুক্ত WARC স্পেকট্রাম অফার করে, যখন "
        "<strong>40m, 20m, 15m এবং 10m</strong> তাদের নিজ নিজ ভয়েস ফোন অংশ থেকে স্পষ্টভাবে পৃথক শান্ত ও বিস্তৃত ডিজিটাল এলাকা প্রদান করে।"
    ),
    'ur': (
        "یہ منتخب کردہ بینڈز بہترین اور شور سے پاک لہروں کی ترسیل کا ماحول فراہم کرتے ہیں: "
        "<strong>30m</strong> ایک تنگ بینڈ WARC بینڈ ہے جہاں بین الاقوامی قانون کے تحت صوتی فون رابطے کی ممانعت ہے؛ "
        "<strong>17m اور 12m</strong> مقابلوں سے پاک WARC سپیکٹرم پیش کرتے ہیں، جبکہ "
        "<strong>40m، 20m، 15m اور 10m</strong> اپنے وائس فون طبقات سے الگ تھلگ وسیع اور پرسکون ڈیجیٹل حصے فراہم کرتے ہیں۔"
    ),
    'tr': (
        "Bu seçilen bantlar gürültüsüz ve optimum yayılım ortamı sunar: "
        "<strong>30m</strong>, uluslararası düzenlemelerle sesli telefon iletişiminin yasaklandığı dar bant bir WARC tahsisidir; "
        "<strong>17m ve 12m</strong> yarışmasız WARC spektrumu sunarken, "
        "<strong>40m, 20m, 15m ve 10m</strong> ilgili ses segmentlerinden iyi ayrılmış geniş ve sessiz dijital alanlar sağlar."
    ),
    'vi': (
        "Các băng tần được chọn này cung cấp môi trường truyền sóng tối ưu và không có tiếng ồn: "
        "<strong>30m</strong> là phân bổ WARC băng hẹp không cho phép thoại theo luật quốc tế; "
        "<strong>17m và 12m</strong> cung cấp phổ WARC không có thi đấu (contest), trong khi "
        "<strong>40m, 20m, 15m và 10m</strong> cung cấp các phân đoạn kỹ thuật số rộng rãi, yên tĩnh được tách biệt rõ ràng khỏi các phân đoạn thoại tương ứng."
    ),
    'id': (
        "Pita frekuensi yang dipilih ini menyediakan lingkungan propagasi yang optimal dan bebas derau: "
        "<strong>30m</strong> adalah alokasi pita-sempit WARC tanpa operasi telepon suara berdasarkan hukum internasional; "
        "<strong>17m dan 12m</strong> menawarkan spektrum WARC bebas kontes, sementara "
        "<strong>40m, 20m, 15m, dan 10m</strong> menawarkan alokasi digital yang luas dan tenang yang terpisah baik dari segmen suara masing-masing."
    ),
    'nl': (
        "Deze geselecteerde banden bieden optimale, storingsvrije propagatieomstandigheden: "
        "<strong>30m</strong> is een smalbandige WARC-band waar spraaktelefonie volgens internationale wetgeving verboden is; "
        "<strong>17m en 12m</strong> bieden contestvrij WARC-spectrum, terwijl "
        "<strong>40m, 20m, 15m en 10m</strong> ruime, rustige digitale segmenten bieden die goed gescheiden zijn van hun respectieve spraaksegmenten."
    ),
    'pl': (
        "Wybrane pasma zapewniają optymalne, ciche warunki propagacyjne: "
        "pasmo <strong>30m</strong> to wąskopasmowy segment WARC z całkowitym międzynarodowym zakazem łączności głosowej; "
        "pasma <strong>17m i 12m</strong> oferują wolne od zawodów pasma WARC, natomiast "
        "pasma <strong>40m, 20m, 15m i 10m</strong> zapewniają szerokie i spokojne segmenty cyfrowe, dobrze odseparowane od pasm fonicznych."
    ),
    'sv': (
        "Dessa utvalda band erbjuder optimala och brusfria vågutbredningsförhållanden: "
        "<strong>30m</strong> är ett smalbandigt WARC-band där telefonitrafik är förbjuden enligt internationell lag; "
        "<strong>17m och 12m</strong> erbjuder tävlingsfritt WARC-spektrum, medan "
        "<strong>40m, 20m, 15m och 10m</strong> erbjuder rymliga, tysta digitala segment väl separerade från respektive telefoniområden."
    ),
    'fi': (
        "Valitut bandit tarjoavat optimaaliset ja kohinattomat etenemisolosuhteet: "
        "<strong>30m</strong> on kapeakaistainen WARC-alue, jolla puheliikenne on kansainvälisesti kielletty; "
        "<strong>17m ja 12m</strong> tarjoavat kilpailuvapaata WARC-spektriä, kun taas "
        "<strong>40m, 20m, 15m ja 10m</strong> tarjoavat laajat ja hiljaiset digitaalialueet selvästi puhealueista erillään."
    ),
    'da': (
        "Disse udvalgte bånd giver optimale og støjfri udbredelsesforhold: "
        "<strong>30m</strong> er et smalbåndet WARC-bånd uden telefoni i henhold til internationale regler; "
        "<strong>17m og 12m</strong> tilbyder konkurrencefrit WARC-spektrum, mens "
        "<strong>40m, 20m, 15m og 10m</strong> tilbyder rummelige, rolige digitale segmenter veladskilt fra deres respektive telefonisegmenter."
    ),
    'no': (
        "Disse utvalgte båndene gir optimale og støyfrie utbredelsesforhold: "
        "<strong>30m</strong> er et smalbåndet WARC-bånd hvor taletelefoni er forbudt etter internasjonale regler; "
        "<strong>17m og 12m</strong> tilbyr konkurransefritt WARC-spektrum, mens "
        "<strong>40m, 20m, 15m og 10m</strong> tilbyr romslige, rolige digitale segmenter godt atskilt fra sine respektive telefoniområder."
    ),
    'cs': (
        "Tato vybraná pásma poskytují optimální šíření bez rušení: "
        "<strong>30m</strong> je úzkopásmové pásmo WARC, kde je provoz fonií mezinárodně zakázán; "
        "<strong>17m a 12m</strong> nabízejí spektrum WARC bez radioamatérských závodů, zatímco "
        "<strong>40m, 20m, 15m a 10m</strong> nabízejí prostorné a tiché digitální úseky dobře oddělené od fonických segmentů."
    ),
    'sk': (
        "Vybrané pásma ponúkajú optimálne a bezšumové podmienky šírenia: "
        "<strong>30m</strong> je úzkopásmové pásmo WARC, kde je hlasová prevádzka medzinárodne zakázaná; "
        "<strong>17m a 12m</strong> ponúkajú spektrum WARC bez pretekov, zatiaľ čo "
        "<strong>40m, 20m, 15m a 10m</strong> poskytujú široké a pokojné digitálne úseky dobre oddelené od hlasových segmentov."
    ),
    'hu': (
        "A kiválasztott sávok optimális és zajmentes terjedési környezetet biztosítanak: "
        "a <strong>30m</strong> egy keskenysávú WARC sáv, ahol a hangátvitel nemzetközi tiltás alá esik; "
        "a <strong>17m és 12m</strong> versenymentes WARC spektrumot kínál, míg "
        "a <strong>40m, 20m, 15m és 10m</strong> sávok tágas, csendes digitális szakaszokat biztosítanak a hangalapú szegmensektől jól elkülönítve."
    ),
    'ro': (
        "Aceste benzi selectate oferă condiții optime de propagare fără zgomot: "
        "<strong>30m</strong> este o alocare WARC cu bandă îngustă în care telefonia vocală este interzisă prin lege internațională; "
        "<strong>17m și 12m</strong> oferă spectru WARC fără concursuri, în timp ce "
        "<strong>40m, 20m, 15m și 10m</strong> oferă segmente digitale liniștite și spațioase, bine separate de segmentele vocale corespunzătoare."
    ),
    'el': (
        "Αυτές οι επιλεγμένες ζώνες παρέχουν ιδανικές συνθήκες διάδοσης χωρίς θόρυβο: "
        "τα <strong>30m</strong> είναι μια κατανομή WARC στενής ζώνης χωρίς φωνητική τηλεφωνία βάσει διεθνών κανονισμών· "
        "τα <strong>17m και 12m</strong> προσφέρουν φάσμα WARC χωρίς διαγωνισμούς (contests), ενώ "
        "τα <strong>40m, 20m, 15m και 10m</strong> προσφέρουν ευρείες, ήσυχες ψηφιακές περιοχές σαφώς διαχωρισμένες από τα αντίστοιχα τμήματα φωνής."
    ),
    'th': (
        "ย่านความถี่ที่เลือกเหล่านี้มีสภาพการแพร่กระจายคลื่นที่เหมาะสมและปราศจากสัญญาณรบกวน: "
        "<strong>30m</strong> เป็นย่าน WARC แถบความถี่แคบที่กฎหมายระหว่างประเทศไม่อนุญาตให้ใช้เสียงสนทนา; "
        "<strong>17m และ 12m</strong> มอบย่าน WARC ที่ปราศจากการแข่งขัน ในขณะที่ "
        "<strong>40m, 20m, 15m และ 10m</strong> ให้การจัดสรรดิจิทัลที่กว้างขวางและเงียบสงบซึ่งแยกออกจากส่วนเสียงสนทนาอย่างชัดเจน"
    ),
    'ca': (
        "Aquestes bandes seleccionades proporcionen entorns de propagació òptims i silenciosos: "
        "<strong>30m</strong> és una assignació WARC de banda estreta sense operació de fonia permesa per la legislació internacional; "
        "<strong>17m i 12m</strong> ofereixen espectre WARC lliure de concursos, mentre que "
        "<strong>40m, 20m, 15m i 10m</strong> proporcionen amplis i tranquils segments digitals ben separats de les seves respectives seccions de fonia."
    ),
    'eu': (
        "Hautatutako banda hauek hedapen-ingurune optimoa eta zaratarik gabea eskaintzen dute: "
        "<strong>30m</strong> banda estuko WARC esleipena da, non nazioarteko araudiek debekatu egiten duten ahots-telefonia; "
        "<strong>17m eta 12m</strong> lehiaketarik gabeko WARC espektroa eskaintzen dute, eta "
        "<strong>40m, 20m, 15m eta 10m</strong> bandek eremu digital zabal eta lasaiak eskaintzen dituzte dagokien ahots-segmentuetatik ondo bereizita."
    ),
    'gl': (
        "Estas bandas seleccionadas proporcionan contornas de propagación óptimas e silenciosas: "
        "<strong>30m</strong> é unha asignación WARC de banda estreita sen operación de fonía permitida pola lexislación internacional; "
        "<strong>17m e 12m</strong> ofrecen espectro WARC libre de concursos, mentres que "
        "<strong>40m, 20m, 15m e 10m</strong> proporcionan amplos e tranquilos segmentos dixitais ben separados das súas respectivas seccións de fonía."
    ),
    'eo': (
        "Ĉi tiuj elektitaj bendoj provizas optimumajn kaj sensonorajn disvastiĝajn kondiĉojn: "
        "<strong>30m</strong> estas mallarĝbenda asigno de WARC sen voĉfonia operacio permesita de internacia juro; "
        "<strong>17m kaj 12m</strong> ofertas senkonkursan WARC-spektron, dum "
        "<strong>40m, 20m, 15m kaj 10m</strong> provizas vastajn, kvietajn ciferecajn segmentojn bone apartigitajn de siaj respektivaj voĉaj sekcioj."
    ),
    'uk': (
        "Обрані діапазони забезпечують оптимальні умови проходження без шумів: "
        "<strong>30м</strong> — це вузькосмуговий діапазон WARC, де телефонний зв'язок заборонений міжнародним регламентом; "
        "<strong>17м та 12м</strong> пропонують спектр WARC, вільний від контестів, а "
        "<strong>40м, 20м, 15м та 10м</strong> надають широкі та спокійні цифрові ділянки, надійно відокремлені від голосових сегментів."
    ),
    'sw': (
        "Bendi hizi zilizochaguliwa hutoa mazingira tulivu na bora ya usafirishaji wa mawimbi: "
        "<strong>30m</strong> ni bendi maalum ya WARC ya masafa membamba ambapo mawasiliano ya sauti hayaruhusiwi kisheria; "
        "<strong>17m na 12m</strong> hutoa wigo wa WARC usio na mashindano, huku "
        "<strong>40m, 20m, 15m na 10m</strong> zikitoa sehemu tulivu na pana za kidijitali zilizotengwa vizuri kutoka maeneo ya sauti."
    ),
    'ha': (
        "Waɗannan zaɓaɓɓun zangon suna ba da yanayin watsawa mai kyau ba tare da surutu ba: "
        "<strong>30m</strong> zangon WARC ne mai kunkuntar zango wanda dokokin ƙasa da ƙasa suka haramta magana ta murya; "
        "<strong>17m da 12m</strong> suna ba da zangon WARC ba tare da gasa ba, yayin da "
        "<strong>40m, 20m, 15m da 10m</strong> ke ba da faffadan sassan dijital masu natsuwa nesa da sassan murya."
    ),
    'yo': (
        "Awọn ẹgbẹ ti a yan wọnyi pese agbegbe to dara fun gbigbe ifihan agbara laisi ariwo: "
        "<strong>30m</strong> jẹ ipin WARC ti o kere nibiti ofin agbaye ko gba laaye ipe ohun; "
        "<strong>17m ati 12m</strong> n pese igbohunsafẹfẹ WARC laisi idije, lakoko ti "
        "<strong>40m, 20m, 15m ati 10m</strong> n pese awọn agbegbe oni-nọmba to dakẹ ti o yapa daradara kuro ninu awọn apakan ohun."
    ),
    'am': (
        "እነዚህ የተመረጡ ባንዶች የተረጋጋ እና ድምፅ-አልባ የሞገድ ሥርጭት ሁኔታዎችን ይሰጣሉ: "
        "<strong>30m</strong> በዓለም አቀፍ ሕግ የድምፅ ግንኙነት የተከለከለበት ጠባብ የWARC ባንድ ነው፤ "
        "<strong>17m እና 12m</strong> ውድድር-አልባ የWARC ባንድ የሚያቀርቡ ሲሆን "
        "<strong>40m፣ 20m፣ 15m እና 10m</strong> ከድምፅ ክፍሎች በደንብ የተለዩ ሰፊ እና ጸጥ ያሉ የዲጂታል ክፍሎችን ያቀርባሉ።"
    ),
    'az': (
        "Seçilmiş bu diapazonlar optimal və küysüz yayım mühiti təmin edir: "
        "<strong>30m</strong> beynəlxalq qaydalarla səsli telefon rabitəsinin qadağan edildiyi darzolaqlı WARC diapazonudur; "
        "<strong>17m və 12m</strong> yarışsız WARC spektri təklif edir, "
        "<strong>40m, 20m, 15m və 10m</strong> isə səs seqmentlərindən aydın şəkildə ayrılmış geniş və sakit rəqəmsal sahələr təmin edir."
    ),
    'uz': (
        "Ushbu tanlangan diapazonlar optimal va shovqinsiz tarqalish sharoitlarini ta'minlaydi: "
        "<strong>30m</strong> xalqaro qonunchilikka binoan ovozli aloqa taqiqlangan tor polosali WARC diapazonidir; "
        "<strong>17m va 12m</strong> musobaqalarsiz WARC spektrini taklif etadi, "
        "<strong>40m, 20m, 15m va 10m</strong> esa ovozli segmentlardan yaxshi ajratilgan keng va tinch raqamli hududlarni taqdim etadi."
    ),
    'ta': (
        "இந்த தேர்ந்தெடுக்கப்பட்ட அலைவரிசைகள் சத்தமில்லாத உகந்த பரவல் சூழலை வழங்குகின்றன: "
        "<strong>30m</strong> என்பது சர்வதேச சட்டத்தால் குரல் தொடர்பு தடைசெய்யப்பட்ட குறுகிய-அலைவரிசை WARC பிரிவாகும்; "
        "<strong>17m மற்றும் 12m</strong> போட்டிகள் இல்லாத WARC அலைவரிசையை வழங்குகின்றன, அதே நேரத்தில் "
        "<strong>40m, 20m, 15m மற்றும் 10m</strong> குரல் பிரிவுகளிலிருந்து நன்கு பிரிக்கப்பட்ட அமைதியான மற்றும் விரிவான டிஜிட்டல் இடங்களை வழங்குகின்றன."
    ),
    'te': (
        "ఈ ఎంచుకున్న బ్యాండ్‌లు నాయిస్ లేని సరైన వాతావరణాన్ని అందిస్తాయి: "
        "<strong>30m</strong> అనేది అంతర్జాతీయ చట్టం ప్రకారం వాయిస్ ఫోన్ అనుమతించబడని ఇరుకైన బ్యాండ్‌విడ్త్ WARC కేటాయింపు; "
        "<strong>17m మరియు 12m</strong> పోటీలు లేని WARC స్పెక్ట్రమ్‌ను అందిస్తాయి, అయితే "
        "<strong>40m, 20m, 15m మరియు 10m</strong> వాయిస్ విభాగాల నుండి వేరు చేయబడిన విశాలమైన, ప్రశాంతమైన డిజిటల్ విభాగాలను అందిస్తాయి."
    ),
    'mr': (
        "हे निवडलेले बँड उत्कृष्ट आणि आवाजमुक्त प्रसार वातावरण प्रदान करतात: "
        "<strong>30m</strong> हे एक अरुंद-बँड WARC वाटप आहे जिथे आंतरराष्ट्रीय कायद्यानुसार व्हॉईस फोन ऑपरेशनला परवानगी नाही; "
        "<strong>17m आणि 12m</strong> स्पर्धा-मुक्त WARC स्पेक्ट्रम देतात, तर "
        "<strong>40m, 20m, 15m आणि 10m</strong> त्यांच्या व्हॉईस विभागांपासून वेगळे केलेले शांत आणि विस्तृत डिजिटल क्षेत्र प्रदान करतात."
    ),
    'gu': (
        "આ પસંદ કરેલા બેન્ડ શ્રેષ્ઠ પ્રસાર વાતાવરણ પૂરું પાડે છે: "
        "<strong>30m</strong> એ નેરો-બેન્ડ WARC ફાળવણી છે જ્યાં આંતરરાષ્ટ્રીય કાયદા દ્વારા વૉઇસ ફોનની મંજૂરી નથી; "
        "<strong>17m અને 12m</strong> સ્પર્ધા-મુક્ત WARC સ્પેક્ટ્રમ પ્રદાન કરે છે, જ્યારે "
        "<strong>40m, 20m, 15m અને 10m</strong> તેમના વૉઇસ સેગમેન્ટ્સથી અલગ શાંત અને વિશાળ ડિજિટલ સ્પેક્ટ્રમ પ્રદાન કરે છે."
    ),
    'kn': (
        "ಆಯ್ಕೆಮಾಡಿದ ಈ ಬ್ಯಾಂಡ್‌ಗಳು ಶಬ್ದರಹಿತ ಪ್ರಸರಣ ವಾತಾವರಣವನ್ನು ಒದಗಿಸುತ್ತವೆ: "
        "<strong>30m</strong> ಅಂತರರಾಷ್ಟ್ರೀಯ ನಿಯಮಗಳ ಪ್ರಕಾರ ಧ್ವನಿ ಸಂವಹನವನ್ನು ನಿಷೇಧಿಸಲಾದ ಕಿರಿದಾದ-ಬ್ಯಾಂಡ್ WARC ಹಂಚಿಕೆಯಾಗಿದೆ; "
        "<strong>17m ಮತ್ತು 12m</strong> ಸ್ಪರ್ಧಾ-ರಹಿತ WARC ತರಂಗಾಂತರವನ್ನು ಒದಗಿಸುತ್ತವೆ, ಆದರೆ "
        "<strong>40m, 20m, 15m ಮತ್ತು 10m</strong> ಧ್ವನಿ ವಿಭಾಗಗಳಿಂದ ಪ್ರತ್ಯೇಕಿಸಲಾದ ವಿಶಾಲವಾದ ಶಾಂತ ಡಿಜಿಟಲ್ ವಿಭಾಗಗಳನ್ನು ಒದಗಿಸುತ್ತವೆ."
    ),
    'ml': (
        "തിരഞ്ഞെടുത്ത ഈ ബാൻഡുകൾ ശബ്ദരഹിതമായ മികച്ച പ്രസരണ അന്തരീക്ഷം നൽകുന്നു: "
        "അന്താരാഷ്ട്ര നിയമപ്രകാരം വോയ്‌സ് ഫോൺ അനുവദിക്കാത്ത ഇടുങ്ങിയ ബാൻഡ്‌വിഡ്ത്ത് WARC ബാൻഡാണ് <strong>30m</strong>; "
        "മത്സരങ്ങളില്ലാത്ത WARC സ്പെക്ട്രം <strong>17m, 12m</strong> എന്നിവ നൽകുന്നു, അതേസമയം "
        "<strong>40m, 20m, 15m, 10m</strong> വോയ്‌സ് വിഭാഗങ്ങളിൽ നിന്ന് വേർതിരിച്ച ശാന്തവും വിശാലവുമായ ഡിജിറ്റൽ മേഖലകൾ ഉറപ്പാക്കുന്നു."
    ),
    'pa': (
        "ਇਹ ਚੁਣੇ ਹੋਏ ਬੈਂਡ ਅਨੁਕੂਲ, ਸ਼ੋਰ-ਰਹਿਤ ਪ੍ਰਸਾਰ ਵਾਤਾਵਰਣ ਪ੍ਰਦਾਨ ਕਰਦੇ ਹਨ: "
        "<strong>30m</strong> ਇੱਕ ਤੰਗ-ਬੈਂਡ WARC ਅਲਾਟਮੈਂਟ ਹੈ ਜਿਸ ਵਿੱਚ ਅੰਤਰਰਾਸ਼ਟਰੀ ਕਾਨੂੰਨ ਦੁਆਰਾ ਕੋਈ ਵੌਇਸ ਫ਼ੋਨ ਸੰਚਾਲਨ ਦੀ ਇਜਾਜ਼ਤ ਨਹੀਂ ਹੈ; "
        "<strong>17m ਅਤੇ 12m</strong> ਮੁਕਾਬਲੇ-ਮੁਕਤ WARC ਸਪੈਕਟ੍ਰਮ ਪੇਸ਼ ਕਰਦੇ ਹਨ, ਜਦੋਂ ਕਿ "
        "<strong>40m, 20m, 15m ਅਤੇ 10m</strong> ਉਹਨਾਂ ਦੇ ਵੌਇਸ ਫ਼ੋਨ ਖੰਡਾਂ ਤੋਂ ਵੱਖਰੇ ਵਿਸਤ੍ਰਿਤ, ਸ਼ਾਂਤ ਡਿਜੀਟਲ ਸਪੇਸ ਪ੍ਰਦਾਨ ਕਰਦੇ ਹਨ।"
    ),
    'or': (
        "ଏହି ମନୋନୀତ ବ୍ୟାଣ୍ଡଗୁଡ଼ିକ ସର୍ବୋତ୍କୃଷ୍ଟ, ଶବ୍ଦମୁକ୍ତ ପ୍ରସାରଣ ପରିବେଶ ପ୍ରଦାନ କରେ: "
        "<strong>30m</strong> ହେଉଛି ଏକ ସଂକୀର୍ଣ୍ଣ-ବ୍ୟାଣ୍ଡ WARC ଆବଣ୍ଟନ ଯେଉଁଥିରେ ଅନ୍ତର୍ଜାତୀୟ ନିୟମ ଦ୍ୱାରା କୌଣସି ଭଏସ୍ ଫୋନ୍ କାର୍ଯ୍ୟ ଅନୁମତିପ୍ରାପ୍ତ ନୁହେଁ; "
        "<strong>17m ଏବଂ 12m</strong> ପ୍ରତିଯୋଗିତା-ମୁକ୍ତ WARC ସ୍ପେକ୍ଟ୍ରମ୍ ପ୍ରଦାନ କରେ, ଯେତେବେଳେ କି "
        "<strong>40m, 20m, 15m ଏବଂ 10m</strong> ସେମାନଙ୍କର ଭଏସ୍ ଫୋନ୍ ସେଗମେଣ୍ଟରୁ ଭଲ ଭାବରେ ପୃଥକ ଶାନ୍ତ ଡିଜିଟାଲ୍ ସ୍ଥାନ ପ୍ରଦାନ କରେ।"
    ),
    'my': (
        "ဤရွေးချယ်ထားသော လှိုင်းခွင်များသည် ဆူညံသံကင်းစင်သော အကောင်းဆုံး လှိုင်းဖြန့်ကျက်မှု ပတ်ဝန်းကျင်ကို ပံ့ပိုးပေးသည်- "
        "<strong>30m</strong> သည် နိုင်ငံတကာဥပဒေအရ အသံဖုန်းဆက်သွယ်မှုခွင့်မပြုသော လှိုင်းကျဉ်း WARC သတ်မှတ်ချက်ဖြစ်သည်፤ "
        "<strong>17m နှင့် 12m</strong> သည် ပြိုင်ပွဲကင်းစင်သော WARC လှိုင်းစဉ်ကို ပေးစွမ်းပြီး "
        "<strong>40m, 20m, 15m နှင့် 10m</strong> သည် သက်ဆိုင်ရာအသံပိုင်းများမှ သီးခြားခွဲထုတ်ထားသော ကျယ်ဝန်းပြီး တိတ်ဆိတ်သည့် ဒစ်ဂျစ်တယ်နေရာများကို ပံ့ပိုးပေးပါသည်။"
    ),
    'tl': (
        "Ang mga napiling bandang ito ay nagbibigay ng maayos at walang ingay na kapaligiran sa pagpapalaganap: "
        "ang <strong>30m</strong> ay isang makitid na WARC alokasyon kung saan ipinagbabawal ang boses ng internasyonal na batas; "
        "nag-aalok ang <strong>17m at 12m</strong> ng WARC spectrum na walang paligsahan, habang "
        "ang <strong>40m, 20m, 15m, at 10m</strong> ay nag-aalok ng malalawak at tahimik na digital segment na nakahiwalay sa kani-kanilang voice phone segment."
    ),
    'jv': (
        "Pita sing dipilih iki nyedhiyakake lingkungan panyebaran sing optimal lan tanpa swara: "
        "<strong>30m</strong> minangka alokasi pita-ciut WARC sing ora ngidini komunikasi swara miturut hukum internasional; "
        "<strong>17m lan 12m</strong> nawakake spektrum WARC bebas kontes, dene "
        "<strong>40m, 20m, 15m, lan 10m</strong> nawakake wilayah digital sing jembar lan tenang sing kapisah saka bagean swara."
    ),
    'su': (
        "Pita anu dipilih ieu nyadiakeun lingkungan rambatan anu optimal sareng henteu gandeng: "
        "<strong>30m</strong> mangrupikeun alokasi pita-heureut WARC anu teu kénging aya komunikasi sora dumasar kana hukum internasional; "
        "<strong>17m sareng 12m</strong> nawiskeun spéktrum WARC bébas kontés, sedengkeun "
        "<strong>40m, 20m, 15m, sareng 10m</strong> nawiskeun daérah digital anu lega tur tenang anu papisah tina bagian sora."
    ),
    'bho': (
        "ई चुनल गइल बैंड बिना शोर वाला बेहतरीन माहौल देवेला: "
        "<strong>30m</strong> एगो संकीर्ण बैंड WARC आवंटन हवे जहाँ अंतर्राष्ट्रीय कानूनन आवाज वाला फोन मना बा; "
        "<strong>17m आ 12m</strong> प्रतियोगिता-मुक्त WARC स्पेक्ट्रम देवेला, जबकि "
        "<strong>40m, 20m, 15m आ 10m</strong> आवाज वाला हिस्सा से अलग शांत आ बड़हन डिजिटल जगह देवेला।"
    ),
    'pcm': (
        "Dis selected bands dey give clean, noise-free propagation environment: "
        "<strong>30m</strong> na narrow-band WARC allocation wey international law no allow voice phone; "
        "<strong>17m and 12m</strong> dey give contest-free WARC spectrum, while "
        "<strong>40m, 20m, 15m, and 10m</strong> get plenty quiet digital space well separated from voice segments."
    )
}


def update_language_file(lang, text):
    file_path = os.path.join(LANG_DIR, f"lang{lang}.js")
    if not os.path.exists(file_path):
        print(f"File not found: {file_path}")
        return False

    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Escape quotes and backslashes for JS string literal
    escaped_text = text.replace('\\', '\\\\').replace('"', '\\"')

    # Replace FREQUENCIES_BOX_P2
    pattern = r'("FREQUENCIES_BOX_P2":\s*")[^"]*(")'
    new_content, count = re.subn(pattern, rf'\g<1>{escaped_text}\g<2>', content)

    if count == 0:
        print(f"Pattern not found in lang{lang}.js")
        return False

    with open(file_path, "w", encoding="utf-8") as f:
        f.write(new_content)

    return True


def main():
    print(f"Updating FREQUENCIES_BOX_P2 across {len(P2_TRANSLATIONS)} languages...")
    success = 0
    for lang, text in P2_TRANSLATIONS.items():
        if update_language_file(lang, text):
            success += 1
            print(f"  ✓ Updated lang{lang}.js")
        else:
            print(f"  ✗ Failed lang{lang}.js")

    print(f"\nSuccessfully updated {success}/{len(P2_TRANSLATIONS)} language files.")


if __name__ == "__main__":
    main()
