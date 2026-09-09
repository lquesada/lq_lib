// =============================================================================
// The LQ Digital Mode Family — Reference Implementation (lq_lib)
//
// Author:  Luis Quesada (HB9IPH)
// Web:     https://luisquesada.com
// Portal:  https://lquesada.github.io/lq_lib/
// GitHub:  https://github.com/lquesada/lq_lib
// App:     qFT8 — Portable Amateur Radio for Android (https://qft8.com)
//
// License: MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)
//
// Copyright (c) 2026 Luis Quesada (HB9IPH)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// =============================================================================

// List of all supported language codes for the LQ Web Portal
const supportedLanguages = [
  'am', 'apc', 'apd', 'ar', 'arq', 'ary', 'arz', 'az', 'bho', 'bn',
  'ca', 'cs', 'da', 'de', 'el', 'en', 'eo', 'es', 'eu', 'fa',
  'fi', 'fr', 'gl', 'gu', 'ha', 'he', 'hi', 'hu', 'id', 'it',
  'ja', 'jv', 'kn', 'ko', 'ml', 'mr', 'my', 'nl', 'no', 'or',
  'pa', 'pcm', 'pl', 'pt', 'ro', 'ru', 'sk', 'su', 'sv', 'sw',
  'ta', 'te', 'th', 'tl', 'tr', 'uk', 'ur', 'uz', 'vi', 'wuu',
  'yo', 'yue', 'zh'
];

function getLanguage() {
  // Check for query parameter override (e.g. ?lang=es)
  const urlParams = new URLSearchParams(window.location.search);
  const langParam = urlParams.get('lang');
  if (langParam && supportedLanguages.includes(langParam.toLowerCase())) {
    return langParam.toLowerCase();
  }

  // Auto-determine from browser settings
  const browserLang = (navigator.language || navigator.userLanguage || 'en').split('-')[0].toLowerCase();
  if (supportedLanguages.includes(browserLang)) {
    return browserLang;
  }

  // Default to English
  return 'en';
}

function loadLanguage(lang) {
  if (lang === 'en') {
    // English is the default in HTML, apply if langen.js exists or just reveal
    const script = document.createElement('script');
    script.src = `languages/langen.js`;
    script.onload = () => applyTranslations(lang);
    script.onerror = () => {
      document.body.style.visibility = 'visible';
    };
    document.head.appendChild(script);
    return;
  }

  const script = document.createElement('script');
  script.src = `languages/lang${lang}.js`;
  script.onload = () => applyTranslations(lang);
  script.onerror = () => {
    document.body.style.visibility = 'visible';
  };
  document.head.appendChild(script);
}

function applyTranslations(lang) {
  if (typeof langData !== 'undefined') {
    // Update Title
    const metaTitle = document.querySelector('title');
    if (metaTitle && langData['META_TITLE'] !== undefined) {
      metaTitle.innerHTML = langData['META_TITLE'];
    }

    // Update Meta Description & OpenGraph tags
    const metaDesc = document.querySelector('meta[name="description"]');
    if (metaDesc && langData['META_DESC'] !== undefined) {
      metaDesc.content = langData['META_DESC'];
    }
    const ogTitle = document.querySelector('meta[property="og:title"]');
    if (ogTitle && langData['META_TITLE'] !== undefined) {
      ogTitle.content = langData['META_TITLE'];
    }
    const ogDesc = document.querySelector('meta[property="og:description"]');
    if (ogDesc && langData['META_DESC'] !== undefined) {
      ogDesc.content = langData['META_DESC'];
    }

    // Replace content in elements marked with data-i18n
    document.querySelectorAll('[data-i18n]').forEach(el => {
      const key = el.getAttribute('data-i18n');
      if (langData[key] !== undefined) {
        const content = langData[key];
        if (el.tagName === 'IMG') {
          el.alt = content;
        } else {
          el.innerHTML = content;
        }
      }
    });

    // Update document lang attribute
    document.documentElement.lang = lang;
  }

  // Make body visible after translations are applied
  document.body.style.visibility = 'visible';
}

document.addEventListener('DOMContentLoaded', () => {
  const lang = getLanguage();
  if (lang !== 'en') {
    document.body.style.visibility = 'hidden';
  }
  loadLanguage(lang);

  // Update internal links to propagate ?lang= parameter if explicitly set
  const urlParams = new URLSearchParams(window.location.search);
  if (urlParams.has('lang')) {
    const forceLang = urlParams.get('lang');
    document.querySelectorAll('a').forEach(a => {
      const href = a.getAttribute('href');
      if (href && !href.startsWith('http') && !href.startsWith('//') && !href.startsWith('#')) {
        try {
          const url = new URL(a.href, window.location.href);
          url.searchParams.set('lang', forceLang);
          a.href = url.pathname + url.search + url.hash;
        } catch (e) {}
      }
    });
  }
});
