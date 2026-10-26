// LQ live map (lq8.org/live and the small map on the main page).
// Shows the stations active in the LQ modes (LQ8, LQ4, LQ2, LQ16) in the last 10 minutes, as reported by qFT8 users,
// and the upcoming LQ DXpeditions listed in dxpeditions.json. No frameworks, no tiles, no trackers: one SVG world map
// (land outlines from world.js), equirectangular, pan + zoom. The projection, pan/zoom handling and the map colours
// follow the qFT8 web remote's world map (its default "Bright Future" theme).
(function () {
  'use strict';

  var FEED_DEFAULT = 'https://data.qft8.com/lqlive.json', FEED = FEED_DEFAULT;
  var POLL_MS = 30000;
  var MAX_AGE_MS = 10 * 60 * 1000;
  var SCRIPT_SRC = (document.currentScript && document.currentScript.src) || '';
  // Local testing only: ?feed=<url> is honoured on localhost, never on the real site.
  try {
    var qp = new URLSearchParams(location.search).get('feed');
    if (qp && (location.hostname === 'localhost' || location.hostname === '127.0.0.1')) FEED = qp;
  } catch (e) {}

  // The map themes: the qFT8 app's world map palettes, same names, same order (LQ8-SITE-THEMES-1). Land, sea, coast,
  // borders and grid are the app's; names, pin / tag outlines and lines are chosen per theme for contrast.
  function T(name, ocean, land, coast, border, grid, name1, halo, outline, extra) {
    var t = { name: name, ocean: ocean, land: land, coast: coast, border: border, grid: grid, nameFill: name1, nameHalo: halo,
      pinStroke: outline, tagStroke: outline, leader: name1, leaderHalo: halo, dx: '#D81B78', dxStroke: outline,
      dxBadge: '#0F172A', dxBadgeText: '#F8FAFC', dxClock: '#F9A8D4' };
    for (var k in extra) t[k] = extra[k];
    return t;
  }
  var THEMES = [
    T('BRIGHT_FUTURE', '#1E477A', '#3B82B6', '#CBD5E1', 'rgba(226,232,240,0.7)', 'rgba(255,255,255,0.22)', '#E2E8F0', '#0F172A', '#0F172A',
      { leader: '#F8FAFC', pinHalo: 'rgba(248,250,252,0.85)' }),
    T('CONSOLE', '#050B05', '#1B3D22', '#22C55E', 'rgba(22,163,74,0.85)', 'rgba(34,197,94,0.3)', '#86EFAC', '#050B05', '#86EFAC'),
    T('EARTH', '#1E3A8A', '#78350F', '#FDE68A', 'rgba(254,243,199,0.6)', 'rgba(254,243,199,0.25)', '#FEF3C7', '#1E293B', '#0F172A',
      { leader: '#FEF3C7', pinHalo: 'rgba(254,243,199,0.85)' }),
    T('MIDNIGHT', '#090D16', '#223454', '#0EA5E9', 'rgba(100,116,139,0.9)', 'rgba(14,165,233,0.25)', '#E2E8F0', '#090D16', '#E2E8F0'),
    T('AMBER', '#0B0702', '#3D2A0E', '#F59E0B', 'rgba(217,119,6,0.85)', 'rgba(245,158,11,0.25)', '#FCD34D', '#0B0702', '#FCD34D'),
    T('NIGHT_RED', '#050000', '#581010', '#B91C1C', 'rgba(153,27,27,0.9)', 'rgba(185,28,28,0.25)', '#F87171', '#050000', '#FCA5A5'),
    T('DAYLIGHT', '#93C5FD', '#F5F0E1', '#64748B', 'rgba(148,163,184,0.9)', 'rgba(71,85,105,0.25)', '#1E293B', '#F8FAFC', '#1E293B',
      { dxBadge: '#1E293B' }),
    T('MATRIX_GREEN', '#000000', '#0A3D18', '#00FF41', 'rgba(0,168,43,0.85)', 'rgba(0,168,43,0.25)', '#00FF41', '#000000', '#CCFFD8')
  ];
  var THEME_COOKIE = 'lq8live_map_theme';
  function readTheme() {
    var v = readCookieList(THEME_COOKIE)[0], i = parseInt(v, 10);
    return i >= 0 && i < THEMES.length ? i : 0;
  }
  var THEME = THEMES[0];
  // One colour per band (text colour chosen for contrast); unknown bands get the last one.
  // the band colours hams know from the usual reception-report maps
  var BAND_COLORS = {
    '160m': '#7CFC00', '80m': '#E550E5', '60m': '#00008B', '40m': '#5959FF', '30m': '#62D962',
    '20m': '#F2C40C', '17m': '#F2F261', '15m': '#CCA166', '12m': '#B22222', '10m': '#FF69B4',
    '6m': '#FF0000', '4m': '#CC0044', '2m': '#FF1493', '70cm': '#999900', '23cm': '#5AE5E5', '13cm': '#94A3B8'
  };
  var BAND_ORDER = ['160m', '80m', '60m', '40m', '30m', '20m', '17m', '15m', '12m', '10m', '6m', '2m', '70cm', '23cm', '13cm'];   // the bands the service accepts
  var MODE_ORDER = ['LQ8', 'LQ4', 'LQ2', 'LQ16'];
  var SVGNS = 'http://www.w3.org/2000/svg';

  // The bands the viewer chose to hide on the full map, kept in a cookie for a year (only the list of hidden bands).
  var BANDS_COOKIE = 'lq8live_hidden_bands';
  function readHiddenBands() {
    var out = {};
    try {
      var m = document.cookie.match(new RegExp('(?:^|; )' + BANDS_COOKIE + '=([^;]*)'));
      if (m) decodeURIComponent(m[1]).split(',').forEach(function (b) { if (BAND_ORDER.indexOf(b) >= 0) out[b] = 1; });
    } catch (e) {}
    return out;
  }
  function saveHiddenBands(hidden) {
    try {
      document.cookie = BANDS_COOKIE + '=' + encodeURIComponent(Object.keys(hidden).join(',')) +
        '; max-age=31536000; path=/; SameSite=Lax' + (location.protocol === 'https:' ? '; Secure' : '');
    } catch (e) {}
  }
  // Hint panels the viewer closed with their X (full map), kept in a cookie for a year; "Show hints" brings them back.
  var HINTS_COOKIE = 'lq8live_hidden_hints';
  function readCookieList(name) {
    try {
      var m = document.cookie.match(new RegExp('(?:^|; )' + name + '=([^;]*)'));
      return m ? decodeURIComponent(m[1]).split(',').filter(function (x) { return x; }) : [];
    } catch (e) { return []; }
  }
  function writeCookieList(name, list) {
    try {
      document.cookie = name + '=' + encodeURIComponent(list.join(',')) +
        '; max-age=31536000; path=/; SameSite=Lax' + (location.protocol === 'https:' ? '; Secure' : '');
    } catch (e) {}
  }
  // the DXpeditions panel is a hint too, but it is also hidden while there is no DXpedition to list
  var hintsClosed = [], dxListed = false, reapplyHints = null;
  function hintClosed(id) { return hintsClosed.indexOf(id) >= 0; }
  function setupHints() {
    var panels = document.querySelectorAll('[data-hint]');
    var show = document.querySelector('.lqlive-hints-show');
    if (!panels.length) return;
    var closed = hintsClosed = readCookieList(HINTS_COOKIE);
    function apply() {
      hintsClosed = closed;
      var anyShown = false;
      for (var i = 0; i < panels.length; i++) {
        var id = panels[i].getAttribute('data-hint');
        panels[i].hidden = closed.indexOf(id) >= 0 || (id === 'dx' && !dxListed);
        if (!panels[i].hidden) anyShown = true;
      }
      // one button for all of them: "Hide hints" while any is shown, "Show hints" when all are closed
      if (show) {
        show.hidden = false;
        show.textContent = anyShown ? str('LIVE_HIDE_HINTS', 'Hide hints') : str('LIVE_SHOW_HINTS', 'Show hints');
      }
      try { window.dispatchEvent(new Event('resize')); } catch (e) {}   // the station tags keep off the panels shown
      var pre = document.getElementById('lqlive-hints-pre');   // the first-paint rule of the page's head
      if (pre) pre.parentNode.removeChild(pre);
    }
    Array.prototype.forEach.call(panels, function (p) {
      var x = h('button', 'lqlive-x', null, p);
      x.type = 'button';
      x.setAttribute('aria-label', str('LIVE_HIDE', 'Hide'));
      x.title = str('LIVE_HIDE', 'Hide');
      x.innerHTML = '<svg viewBox="0 0 12 12" width="12" height="12" aria-hidden="true"><path d="M2 2L10 10M10 2L2 10" ' +
        'stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/></svg>';
      x.addEventListener('click', function () {
        var id = p.getAttribute('data-hint');
        if (closed.indexOf(id) < 0) closed.push(id);
        writeCookieList(HINTS_COOKIE, closed);
        apply();
      });
    });
    if (show) show.addEventListener('click', function () {
      var anyShown = false;
      for (var i = 0; i < panels.length; i++) if (!panels[i].hidden) anyShown = true;
      closed = anyShown ? Array.prototype.map.call(panels, function (p) { return p.getAttribute('data-hint'); }) : [];
      writeCookieList(HINTS_COOKIE, closed);
      apply();
    });
    reapplyHints = apply;
    apply();
  }
  // Full screen (top bar, rightmost): the button, Esc or the browser's Back leave it again
  function setupFullscreen() {
    var b = document.querySelector('.lqlive-fs'), de = document.documentElement;
    if (!b) return;
    var req = de.requestFullscreen || de.webkitRequestFullscreen, can = document.fullscreenEnabled || document.webkitFullscreenEnabled;
    if (!req || !can) return;   // e.g. Safari on iPhone: no page full screen, so no button
    function fsEl() { return document.fullscreenElement || document.webkitFullscreenElement; }
    function exit() { var x = document.exitFullscreen || document.webkitExitFullscreen; if (x && fsEl()) x.call(document); }
    function label() {
      var on = !!fsEl(), t = on ? str('LIVE_EXIT_FULLSCREEN', 'Exit full screen') : str('LIVE_FULLSCREEN', 'Full screen');
      b.classList.toggle('on', on); b.title = t; b.setAttribute('aria-label', t); b.setAttribute('aria-pressed', on ? 'true' : 'false');
    }
    b.hidden = false; label();
    b.addEventListener('click', function () {
      if (fsEl()) { exit(); return; }
      try { var r = req.call(de); if (r && r.catch) r.catch(function () {}); } catch (e) {}
    });
    function changed() {
      if (fsEl()) {
        if (!(history.state && history.state.lqFs)) { try { history.pushState({ lqFs: 1 }, ''); } catch (e) {} }   // Back leaves full screen
      } else if (history.state && history.state.lqFs) {
        try { history.back(); } catch (e) {}   // left by the button or Esc: drop the entry Back would have used
      }
      label();
    }
    document.addEventListener('fullscreenchange', changed);
    document.addEventListener('webkitfullscreenchange', changed);
    window.addEventListener('popstate', function () { if (fsEl()) exit(); });
  }
  function bandColor(b) { return BAND_COLORS[b] || '#64748B'; }
  function textOn(hex) {
    var n = parseInt(hex.slice(1), 16), r = n >> 16, g = (n >> 8) & 255, b = n & 255;
    return (0.299 * r + 0.587 * g + 0.114 * b) > 150 ? '#0F172A' : '#FFFFFF';
  }
  function el(name, attrs, parent) {
    var e = document.createElementNS(SVGNS, name);
    for (var k in attrs) e.setAttribute(k, attrs[k]);
    if (parent) parent.appendChild(e);
    return e;
  }
  function h(tag, cls, text, parent) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (text !== undefined && text !== null) e.textContent = text;
    if (parent) parent.appendChild(e);
    return e;
  }
  // Translated strings: hidden <span data-i18n="KEY"> elements in the page (pre-rendered per language).
  function str(key, fallback) {
    var e = document.querySelector('#lqlive-strings [data-i18n="' + key + '"]');
    return e ? e.textContent : (fallback || key);
  }
  function gridCenter(g) {
    var lon = (g.charCodeAt(0) - 65) * 20 - 180 + (g.charCodeAt(2) - 48) * 2 + 1;
    var lat = (g.charCodeAt(1) - 65) * 10 - 90 + (g.charCodeAt(3) - 48) + 0.5;
    return { lon: lon, lat: lat };
  }
  // The feed's point for a station (lat/lon, LQ8-LIVE-POSITION-1) is used when it lies in the station's grid square.
  function inSquare(c, gc) {
    return typeof c.lat === 'number' && typeof c.lon === 'number' && isFinite(c.lat) && isFinite(c.lon) &&
      Math.abs(c.lon - gc.lon) <= 1.0001 && Math.abs(c.lat - gc.lat) <= 0.5001;
  }
  // Tag sizes (local units; the map multiplies them by its marker scale): a tag is the callsign over "LQ8 20m".
  var TAG_H = 29, TAG_STEP = 31, MORE_H = 16;
  function tagWidth(it) { return 10 + Math.max((it.mode + ' ' + it.band).length * 6.6, it.call.length * 7.6); }
  function box(l, t, w, hh) { return { l: l, t: t, r: l + w, b: t + hh }; }
  function overlaps(a, b, pad) { pad = pad || 0; return a.l < b.r + pad && b.l < a.r + pad && a.t < b.b + pad && b.t < a.b + pad; }
  function overlapArea(a, b) {
    var w = Math.min(a.r, b.r) - Math.max(a.l, b.l), hh = Math.min(a.b, b.b) - Math.max(a.t, b.t);
    return w > 0 && hh > 0 ? w * hh : 0;
  }
  function itemKey(it) { return it.call + '|' + it.band + '|' + it.mode; }
  function minutesAgo(ms, now) {
    var m = Math.floor((now - ms) / 60000);
    return m < 1 ? str('LIVE_JUST_NOW', 'just now') : str('LIVE_MIN_AGO', '{n} min ago').replace('{n}', String(m));
  }
  function freqText(hz) { return (hz / 1e6).toFixed(3) + ' MHz'; }
  var GRID_OK = /^[A-R]{2}[0-9]{2}$/, CALL_OK = /^[A-Z0-9/]{3,13}$/;

  // ---- DXpeditions -------------------------------------------------------------------------------------------------
  var DAY = 86400000;
  function utcDay(s) { var m = /^(\d{4})-(\d{2})-(\d{2})$/.exec(s || ''); return m ? Date.UTC(+m[1], +m[2] - 1, +m[3]) : NaN; }
  // optional "time": "HH:MM" (UTC) on the start day -> the countdown is exact to the minute
  function utcTime(s) { var m = /^([01]\d|2[0-3]):([0-5]\d)$/.exec(s || ''); return m ? (+m[1] * 60 + +m[2]) * 60000 : 0; }
  // -> null (ended / invalid) or {state:'on'|'soon', days, ms}
  function dxStatus(d, now) {
    var a = utcDay(d.start), b = utcDay(d.end);
    if (isNaN(a) || isNaN(b)) return null;
    if (now >= b + DAY) return null;
    a += utcTime(d.time);
    if (now >= a) return { state: 'on', days: 0, ms: 0 };
    var today = Math.floor(now / DAY) * DAY;   // calendar days, as before: 'tomorrow' = the next UTC day
    return { state: 'soon', days: Math.round((utcDay(d.start) - today) / DAY), ms: a - now, timed: !!utcTime(d.time) };
  }
  function two(n) { return (n < 10 ? '0' : '') + n; }
  // the live countdown under the marker's call (ticks every second): hours can pass 24, no words to translate
  function dxClock(d, now) {
    var a = utcDay(d.start) + utcTime(d.time);
    if (isNaN(a) || now >= a) return str('LIVE_DX_ON_AIR', 'on the air now');
    var sec = Math.ceil((a - now) / 1000), d = Math.floor(sec / 86400), hh = Math.floor(sec / 3600) % 24,
        mm = Math.floor(sec / 60) % 60, ss = sec % 60;
    var t = d > 0 ? d + 'd ' + hh + 'h' : hh > 0 ? hh + 'h ' + mm + 'm' : mm + 'm ' + ss + 's';   // 2d 5h / 14h 2m / 15m 9s
    return str('LIVE_DX_IN', 'in {t}').replace('{t}', t);
  }
  // the start as the viewer's own clock shows it, in the page's language (no strings of ours)
  function dxLocal(d) {
    var a = utcDay(d.start) + utcTime(d.time);
    if (isNaN(a)) return '';
    try {
      var o = { weekday: 'short', year: 'numeric', month: 'short', day: 'numeric', timeZoneName: 'short' };
      if (d.time) { o.hour = '2-digit'; o.minute = '2-digit'; }
      return new Date(a).toLocaleString(document.documentElement.lang || undefined, o);
    } catch (e) { return ''; }
  }
  function dxCounter(st) {
    if (st.state === 'on') return str('LIVE_DX_ON_AIR', 'on the air now');
    if (st.timed && st.ms < DAY) {   // a start time given, the last 24 h: hours and minutes, "T−05:12" (no words to translate)
      var min = Math.ceil(st.ms / 60000);
      return 'T\u2212' + two(Math.floor(min / 60)) + ':' + two(min % 60);
    }
    if (st.days <= 1) return str('LIVE_DX_TOMORROW', 'tomorrow');
    return str('LIVE_DX_DAYS', 'in {n} days').replace('{n}', String(st.days));
  }
  function validDx(d) {
    return d && typeof d.call === 'string' && CALL_OK.test(d.call) && typeof d.lat === 'number' && typeof d.lon === 'number' &&
      Math.abs(d.lat) <= 90 && Math.abs(d.lon) <= 180;
  }

  // ---- DXCC entity of a station (dxcc.js; its tables, dxcc_data.js, are loaded on the first popup) ------------------
  var dxccState = 0;   // 0 = not loaded, 1 = loading, 2 = ready
  function dxccOf(call, map, kind, idx) {
    var api = window.LqDxcc;
    if (!api) return null;
    if (dxccState === 2) return api.resolve(call);
    if (dxccState === 0) {
      dxccState = 1;
      var sc = document.createElement('script');
      try { sc.src = new URL('dxcc_data.js', SCRIPT_SRC || location.href).href; } catch (e) { dxccState = 0; return null; }
      sc.onload = function () {
        if (!window.LQ_DXCC_DATA) { dxccState = 0; return; }
        api.load(window.LQ_DXCC_DATA);
        dxccState = 2;
        // the popup still open for this station gets its entity rows now
        if (map.popKey === kind + idx && !map.pop.hidden) map.showPop(kind, idx, map.pinned);
      };
      sc.onerror = function () { dxccState = 0; };
      document.head.appendChild(sc);
    }
    return null;
  }

  // ---- the map -----------------------------------------------------------------------------------------------------
  function LiveMap(root, opts) {
    this.root = root;
    this.mini = !!opts.mini;
    this.href = opts.href || '';
    this.items = [];
    this.dx = [];
    this.hiddenBands = {};
    this.hiddenModes = {};
    this.st = { zoom: 1, minZoom: 1, maxZoom: 40, offsetX: 0, offsetY: 0 };
    this.pointers = {};
    this.theme = THEMES[readTheme()];
    this.build();
  }

  LiveMap.prototype.build = function () {
    var self = this;
    this.root.classList.add('lqmap');
    this.svg = el('svg', { 'class': 'lqmap-svg', role: 'img', 'aria-label': str('LIVE_TITLE', 'LQ8 live map') }, this.root);
    this.oceanRect = el('rect', { x: 0, y: 0, width: '100%', height: '100%' }, this.svg);
    var defs = el('defs', {}, this.svg);
    var tile = el('g', { id: 'lqtile' + Math.random().toString(36).slice(2) }, defs);
    // Maidenhead fields (20 x 10 degrees)
    var gd = '';
    for (var x = 0; x <= 360; x += 20) gd += 'M' + x + ',0V180';
    for (var y = 0; y <= 180; y += 10) gd += 'M0,' + y + 'H360';
    // land, coast and borders (world.js: one Natural Earth source, so the borders meet the coast); the finer levels
    // in map/ (updateGeo) replace them once the full map is zoomed in
    var md = window.LQ8_MAP || {};
    this.allGeo = []; this.cellEls = {};
    this.geo = el('g', { transform: 'scale(' + (1 / (md.q || 1)) + ')' }, tile);
    this.geoPaths(this.geo, md);
    this.levelG = LEVELS.map(function () { return el('g', { display: 'none' }, tile); });
    this.pGrid = el('path', { d: gd, fill: 'none', 'stroke-width': 0.6, 'vector-effect': 'non-scaling-stroke' }, tile);
    this.applyTheme(this.theme);
    this.world = el('g', {}, this.svg);
    for (var k = -2; k <= 2; k++) el('use', { href: '#' + tile.id, x: k * 360, y: 0 }, this.world);
    this.names = el('g', { 'class': 'lqmap-names' }, this.svg);
    this.marks = el('g', {}, this.svg);
    this.gDx = el('g', {}, this.marks);
    this.gLead = el('g', {}, this.marks);
    this.gPin = el('g', {}, this.marks);
    this.gTag = el('g', {}, this.marks);
    this.live = {};
    this.pop = h('div', 'lqmap-pop', null, this.root);
    this.pop.hidden = true;
    window.addEventListener('resize', function () { self.render(); });
    if (!this.mini) this.bindPanZoom();
    else if (this.href) {
      // the small map on the main page: a click anywhere opens the full live map
      this.svg.classList.add('lqmap-link');
      this.svg.addEventListener('click', function () { location.href = self.href; });
    }
    this.render();
  };

  LiveMap.prototype.applyTheme = function (t) {
    this.theme = t;
    this.root.style.background = t.ocean;
    this.root.style.setProperty('--lq-name', t.nameFill);
    this.root.style.setProperty('--lq-halo', t.nameHalo);
    this.oceanRect.setAttribute('fill', t.ocean);
    this.allGeo.forEach(function (ps) {
      ps.fill.setAttribute('fill', t.land); ps.land.setAttribute('fill', t.land); ps.land.setAttribute('stroke', t.coast);
      ps.coast.setAttribute('stroke', t.coast); ps.borders.setAttribute('stroke', t.border);
    });
    this.pGrid.setAttribute('stroke', t.grid);
    if (this.W) this.render();
  };
  // Finer map levels (map/<level>/<col>_<row>.json, built from the same Natural Earth source): once the full map is zoomed
  // in, only the cells on screen (and the ring around them) are fetched; a level is shown when every cell on screen has
  // arrived, the coarser one until then. Cells that are all sea have no file (404 = empty).
  var LEVELS = [{ z: 2.5, dir: 'm1', cell: 30 }, { z: 8, dir: 'm2', cell: 10 }];
  var cellCache = {};   // 'm1/3_2' -> null (loading) | false (empty / failed) | data (shared by every map on the page)
  LiveMap.prototype.cellRange = function (lv, margin) {
    var s = this.scale(), cell = lv.cell, out = [];
    var lonA = (0 - this.W / 2 - this.st.offsetX) / s, lonB = (this.W - this.W / 2 - this.st.offsetX) / s;
    var latA = (this.H / 2 + this.st.offsetY) / s, latB = (this.H / 2 + this.st.offsetY - this.H) / s;
    var c0 = Math.floor((lonA + 180) / cell) - margin, c1 = Math.floor((lonB + 180) / cell) + margin;
    var r0 = Math.max(0, Math.floor((90 - latA) / cell) - margin), r1 = Math.min(180 / cell - 1, Math.floor((90 - latB) / cell) + margin);
    var nc = 360 / cell;
    for (var c = c0; c <= c1 && c - c0 < nc; c++)
      for (var r = r0; r <= r1; r++) out.push(lv.dir + '/' + (((c % nc) + nc) % nc) + '_' + r);
    return out;
  };
  LiveMap.prototype.updateGeo = function () {
    if (this.mini) return;
    var self = this, want = -1;
    for (var i = 0; i < LEVELS.length; i++) if (this.st.zoom >= LEVELS[i].z) want = i;
    var show = -1;
    if (want >= 0) {
      this.cellRange(LEVELS[want], 1).forEach(function (id) { self.loadCell(id); });
      for (var l = want; l >= 0 && show < 0; l--) {
        var ok = this.cellRange(LEVELS[l], 0).every(function (id) { return cellCache[id] !== undefined && cellCache[id] !== null; });
        if (ok) show = l;
      }
    }
    for (var k = 0; k < LEVELS.length; k++) {
      var g = this.levelG[k];
      if (k === show) {
        this.cellRange(LEVELS[k], 1).forEach(function (id) { self.drawCell(g, id); });
        g.removeAttribute('display');
      } else g.setAttribute('display', 'none');
    }
    if (show >= 0) this.geo.setAttribute('display', 'none'); else this.geo.removeAttribute('display');
  };
  function cellExists(id) {
    var md = window.LQ8_MAP, lv = id.split('/');
    if (!md || !md.cells || typeof md.cells[lv[0]] !== 'string') return true;
    return (' ' + md.cells[lv[0]] + ' ').indexOf(' ' + lv[1] + ' ') >= 0;
  }
  LiveMap.prototype.loadCell = function (id) {
    if (cellCache[id] !== undefined) return;
    if (!cellExists(id)) { cellCache[id] = false; return; }   // all sea: nothing to fetch
    cellCache[id] = null;
    var self = this, url;
    try { url = new URL('map/' + id + '.json', SCRIPT_SRC || location.href).href; } catch (e) { cellCache[id] = false; return; }
    fetch(url).then(function (r) { return r.ok ? r.json() : false; }).then(function (j) {
      cellCache[id] = j && j.q ? j : false;
    }).catch(function () { cellCache[id] = false; }).then(function () {
      if (!self.geoPending) {   // one redraw for a burst of cells
        self.geoPending = true;
        requestAnimationFrame(function () { self.geoPending = false; self.updateGeo(); });
      }
    });
  };
  LiveMap.prototype.drawCell = function (g, id) {
    var d = cellCache[id];
    if (!d || this.cellEls[id]) return;
    var cg = el('g', { transform: 'scale(' + (1 / d.q) + ')' }, g);
    this.cellEls[id] = cg;
    this.geoPaths(cg, d);
  };
  // the four paths of a map piece (world.js or one cell), coloured by the theme
  LiveMap.prototype.geoPaths = function (g, d) {
    var t = this.theme, ps = {
      fill: el('path', { d: d.fill || '', fill: t.land, stroke: 'none' }, g),
      land: el('path', { d: d.land || '', fill: t.land, stroke: t.coast, 'stroke-width': 1.35, 'vector-effect': 'non-scaling-stroke', 'stroke-linejoin': 'round' }, g),
      coast: el('path', { d: d.coast || '', fill: 'none', stroke: t.coast, 'stroke-width': 1.35, 'vector-effect': 'non-scaling-stroke', 'stroke-linejoin': 'round' }, g),
      borders: el('path', { d: d.borders || '', fill: 'none', stroke: t.border, 'stroke-width': 1.0, 'vector-effect': 'non-scaling-stroke', 'stroke-linejoin': 'round' }, g)
    };
    this.allGeo.push(ps);
    return ps;
  };

  // qFT8 web remote: getMapScale / geoToScreen / clampOffsets. Zoom 1 fills the window; a tall phone screen pans sideways.
  LiveMap.prototype.scale = function () {
    // the full map always fills the height (it never shows space above the pole or below Antarctica); sideways it wraps
    var base = this.mini ? this.W / 360.0 : Math.max(this.H / 180.0, this.W / 360.0);
    return base * this.st.zoom;
  };
  LiveMap.prototype.toScreen = function (lon, lat) {
    var s = this.scale();
    return { x: this.W / 2 + this.st.offsetX + lon * s, y: this.H / 2 + this.st.offsetY - lat * s };
  };
  LiveMap.prototype.clamp = function () {
    var st = this.st;
    st.zoom = Math.max(st.minZoom, Math.min(st.maxZoom, st.zoom));
    var s = this.scale();
    var maxY = Math.max(0, 90 * s - this.H / 2);
    st.offsetY = Math.max(-maxY, Math.min(maxY, st.offsetY));
    var worldW = 360 * s;
    if (worldW > 0) st.offsetX = ((st.offsetX % worldW) + worldW * 1.5) % worldW - worldW * 0.5;
  };
  LiveMap.prototype.zoomAt = function (sx, sy, factor) {
    var st = this.st, old = this.scale();
    st.zoom = Math.max(st.minZoom, Math.min(st.maxZoom, st.zoom * factor));
    var f = this.scale() / old;
    st.offsetX = (sx - this.W / 2) * (1 - f) + st.offsetX * f;
    st.offsetY = (sy - this.H / 2) * (1 - f) + st.offsetY * f;
    this.clamp();
    this.render();
  };
  LiveMap.prototype.reset = function () {
    this.st.zoom = 1; this.st.offsetX = 0; this.st.offsetY = 0;
    this.hidePop();
    this.render();
  };

  LiveMap.prototype.bindPanZoom = function () {
    var self = this, svg = this.svg, drag = null, pinch = 0;
    svg.style.touchAction = 'none';
    svg.addEventListener('wheel', function (e) {
      e.preventDefault();
      var r = svg.getBoundingClientRect();
      self.zoomAt(e.clientX - r.left, e.clientY - r.top, e.deltaY < 0 ? 1.25 : 0.8);
    }, { passive: false });
    svg.addEventListener('pointerdown', function (e) {
      self.pointers[e.pointerId] = { x: e.clientX, y: e.clientY };
      var ids = Object.keys(self.pointers);
      if (ids.length === 1) drag = { x0: e.clientX, y0: e.clientY, x: e.clientX, y: e.clientY, target: e.target };
      else if (ids.length === 2) {
        drag = null;
        var a = self.pointers[ids[0]], b = self.pointers[ids[1]];
        pinch = Math.hypot(a.x - b.x, a.y - b.y);
      }
      try { svg.setPointerCapture(e.pointerId); } catch (x) {}
    });
    svg.addEventListener('pointermove', function (e) {
      if (!self.pointers[e.pointerId]) return;
      self.pointers[e.pointerId] = { x: e.clientX, y: e.clientY };
      var ids = Object.keys(self.pointers);
      if (ids.length === 2 && pinch > 0) {
        var a = self.pointers[ids[0]], b = self.pointers[ids[1]];
        var d = Math.hypot(a.x - b.x, a.y - b.y), r = svg.getBoundingClientRect();
        self.zoomAt((a.x + b.x) / 2 - r.left, (a.y + b.y) / 2 - r.top, d / pinch);
        pinch = d;
      } else if (drag) {
        self.st.offsetX += e.clientX - drag.x;
        self.st.offsetY += e.clientY - drag.y;
        drag.x = e.clientX; drag.y = e.clientY;
        if (Math.hypot(e.clientX - drag.x0, e.clientY - drag.y0) >= 6) { svg.classList.add('dragging'); self.hidePop(); }
        self.clamp();
        self.render();
      }
    });
    function up(e) {
      if (!self.pointers[e.pointerId]) return;
      delete self.pointers[e.pointerId];
      if (Object.keys(self.pointers).length < 2) pinch = 0;
      svg.classList.remove('dragging');
      if (drag && Math.hypot(e.clientX - drag.x0, e.clientY - drag.y0) < 6) {
        var m = drag.target && drag.target.closest ? drag.target.closest('[data-idx]') : null;
        if (m) self.showPop(m.getAttribute('data-kind'), +m.getAttribute('data-idx'), true);
        else self.hidePop();
      }
      drag = null;
    }
    svg.addEventListener('pointerup', up);
    svg.addEventListener('pointercancel', up);
    svg.addEventListener('mouseover', function (e) {
      var m = e.target.closest ? e.target.closest('[data-idx]') : null;
      if (m && !self.pinned && e.buttons === 0) self.showPop(m.getAttribute('data-kind'), +m.getAttribute('data-idx'), false);
    });
    svg.addEventListener('mouseout', function (e) {
      var m = e.target.closest ? e.target.closest('[data-idx]') : null;
      if (m && !self.pinned) self.hidePop();
    });
    document.addEventListener('keydown', function (e) { if (e.key === 'Escape') self.hidePop(); });
  };

  LiveMap.prototype.visibleItems = function () {
    var self = this;
    return this.items.filter(function (it) { return !self.hiddenBands[it.band] && !self.hiddenModes[it.mode]; });
  };

  LiveMap.prototype.render = function (fade) {
    this.fadeNew = !!fade;
    var r = this.root.getBoundingClientRect();
    this.W = Math.max(1, r.width); this.H = Math.max(1, r.height);
    this.clamp();
    var s = this.scale();
    var tx = this.W / 2 + this.st.offsetX - 180 * s, ty = this.H / 2 + this.st.offsetY - 90 * s;
    this.world.setAttribute('transform', 'translate(' + tx + ',' + ty + ') scale(' + s + ')');
    this.updateGeo();
    this.drawMarks();
    this.drawNames();   // after the markers: a name keeps off their tags (it is still drawn underneath them)
    if (this.popKey) this.placePop();
  };

  // Every copy of a point that is on screen (the map repeats sideways).
  LiveMap.prototype.copies = function (lon, lat) {
    var p = this.toScreen(lon, lat), w = 360 * this.scale(), out = [];
    for (var k = -2; k <= 2; k++) {
      var x = p.x + k * w;
      if (x > -60 && x < this.W + 60) out.push({ x: x, y: p.y, k: k });
    }
    return out;
  };

  // Country names on the full map once zoomed in (the same zoom 1.8 as the app's world map), in the page's language:
  // countries.js gives each its ISO 3166 code, the browser names it (Intl.DisplayNames); English where it cannot
  // (LQ8-SITE-COUNTRY-NAMES-1). Bigger countries first; a name that would cover another is left out until zoomed in.
  var NAMES_MIN_ZOOM = 1.8, regionNames = null;
  function countryName(row) {
    if (regionNames === null) {
      regionNames = false;
      var lang = (document.documentElement.lang || 'en').toLowerCase();
      try {
        if (lang !== 'en' && Intl.DisplayNames && Intl.DisplayNames.supportedLocalesOf([lang]).length)
          regionNames = new Intl.DisplayNames([lang], { type: 'region', fallback: 'none' });
      } catch (e) { regionNames = false; }
      countryName.lang = lang;
    }
    if (countryName.lang === 'en') return row[1];
    if (!row[0]) return null;   // a region with no code of its own (e.g. Corsica): only on the English page
    if (!regionNames) return row[1];
    var n = null;
    try { n = regionNames.of(row[0]); } catch (e) {}
    return n || row[1];
  }
  // about how wide a text is at 14 px (wide scripts count double)
  function textWidth(t, per) {
    var w = 0;
    for (var i = 0; i < t.length; i++) w += t.charCodeAt(i) >= 0x2E80 ? per * 1.9 : per;
    return w;
  }
  LiveMap.prototype.drawNames = function () {
    var g = this.names;
    while (g.firstChild) g.removeChild(g.firstChild);
    var list = window.LQ8_COUNTRY_NAMES;
    if (this.mini || !list || this.st.zoom < NAMES_MIN_ZOOM) return;
    // rank: Natural Earth's label zoom; our zoom 1 shows the world, about their zoom 2
    var maxRank = 2.6 + Math.log(this.st.zoom) / Math.LN2, taken = (this.markBoxes || []).slice();
    for (var i = 0; i < list.length; i++) {
      if (list[i][4] > maxRank) continue;
      var cs = this.copies(list[i][2], list[i][3]);
      for (var c = 0; c < cs.length; c++) {
        if (cs[c].y < -20 || cs[c].y > this.H + 20 || cs[c].x < -150 || cs[c].x > this.W + 150) continue;
        var name = list[i]._n !== undefined ? list[i]._n : (list[i]._n = countryName(list[i]));
        if (!name) continue;
        var w = textWidth(name, 7.6) + 6, b = { l: cs[c].x - w / 2, r: cs[c].x + w / 2, t: cs[c].y - 10, b: cs[c].y + 10 }, hit = false;
        for (var k = 0; k < taken.length && !hit; k++) hit = overlaps(b, taken[k]);
        if (hit) continue;
        taken.push(b);
        var t = el('text', { x: cs[c].x.toFixed(1), y: cs[c].y.toFixed(1), 'text-anchor': 'middle', 'class': 'lqmap-country' }, g);
        t.textContent = name;
      }
    }
  };

  // ---- markers: stations (pin + tag) and DXpeditions -------------------------------------------------------------
  // Every marker is a keyed group that lives across redraws: pan and zoom only move it; a station or DXpedition that
  // arrives or leaves with new data fades in / out over a second (LQ8-SITE-FADE-1).
  var FADE_MS = 1000;
  function reducedMotion() {
    try { return window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches; } catch (e) { return false; }
  }
  LiveMap.prototype.keyed = function (layer, key, attrs) {
    var e = this.live[key];
    if (!e || e._leaving || e.parentNode !== layer) {
      if (e && e.parentNode && !e._leaving) e.parentNode.removeChild(e);
      e = el('g', { 'data-k': key }, layer);
      this.live[key] = e;
      if (this.fadeNew && !reducedMotion()) {
        e.setAttribute('class', 'lqf lqf-hide');
        (this.fadeQueue = this.fadeQueue || []).push(e);
      }
    } else while (e.firstChild) e.removeChild(e.firstChild);
    for (var k in attrs) e.setAttribute(k, attrs[k]);
    e._gen = this.gen;
    return e;
  };
  // after a draw: the groups not drawn this time leave (fading out when the data changed, at once on pan / zoom)
  LiveMap.prototype.sweep = function () {
    var self = this, fade = this.fadeNew && !reducedMotion();
    Object.keys(this.live).forEach(function (key) {
      var e = self.live[key];
      if (e._gen === self.gen) return;
      delete self.live[key];
      if (fade && !e._leaving && e.parentNode) {
        e._leaving = true;
        e.setAttribute('class', 'lqf lqf-hide');
        e.style.pointerEvents = 'none';
        setTimeout(function () { if (e.parentNode) e.parentNode.removeChild(e); }, FADE_MS + 50);
      } else if (!e._leaving && e.parentNode) e.parentNode.removeChild(e);
    });
    var q = this.fadeQueue;
    this.fadeQueue = null;
    if (q && q.length) {
      q[0].getBoundingClientRect();   // the hidden state is painted first, so the change below is a transition
      requestAnimationFrame(function () { q.forEach(function (e) { if (!e._leaving) e.setAttribute('class', 'lqf'); }); });
    }
  };

  LiveMap.prototype.drawMarks = function () {
    this.gen = (this.gen || 0) + 1;
    this.dxClocks = [];
    var sc = this.mini ? 1.2 : 1.45, obstacles = [];
    // DXpeditions first (underneath the stations); their diamond and tags are obstacles for the station tags
    for (var d = 0; d < this.dx.length; d++) {
      var dx = this.dx[d], cs = this.copies(dx.lon, dx.lat);
      for (var c = 0; c < cs.length; c++) obstacles.push(this.drawDx(cs[c], dx, d, sc));
    }
    // stations: each pin at its own point (the one the qFT8 app draws it at), its tag placed by placeTags
    var vis = this.visibleItems(), pins = [];
    for (var i = 0; i < vis.length; i++) {
      var ps = this.copies(vis[i].lon, vis[i].lat);
      for (var p = 0; p < ps.length; p++) {
        var x = ps[p].x, y = ps[p].y;
        pins.push({ it: vis[i], key: itemKey(vis[i]) + '#' + ps[p].k, order: i, x: x, y: y,
          box: box(x - 7 * sc, y - 20 * sc, 14 * sc, 21 * sc), head: { x: x, y: y - 13 * sc } });
      }
    }
    var groups = this.placeTags(pins, obstacles, sc);
    this.markBoxes = obstacles.concat(groups.map(function (g0) { return g0.box; }), pins.map(function (p0) { return p0.box; }));
    for (var j = 0; j < pins.length; j++) this.drawPinOnly(pins[j], sc);
    for (var gi = 0; gi < groups.length; gi++) this.drawTags(groups[gi], sc);
    this.sweep();
  };

  // LQ8-SITE-LABELS-1: where each station's tag goes. Every tag first tries the spots around its own pin (above, below,
  // the sides, the corners, slightly nudged), keeping the spot it had on the last draw when that still fits, so zoom and
  // pan do not make tags jump. A spot fits when it stays on the visible map and covers no pin, DXpedition tag or other
  // tag. Only stations whose tag fits nowhere are stacked, together with the stations around them: the stack is centred
  // among its pins (above, below or beside them), and a short line runs from each tag to its pin.
  LiveMap.prototype.placeTags = function (pins, obstacles, sc) {
    var self = this, W = this.W, H = this.H, M = 3, PAD = 2 * sc, prev = this.prevSpot || {}, next = {};
    var max = this.mini ? 2 : 4, placed = [], panels = this.panelBoxes();
    function inView(b) { return b.l >= M && b.t >= M && b.r <= W - M && b.b <= H - M; }
    // strict: also keeps off the page's panels over the map; a tag that cannot keep off them may go under one
    function clear(b, except, strict) {
      if (!inView(b)) return false;
      if (strict) for (var q = 0; q < panels.length; q++) if (overlaps(b, panels[q], PAD)) return false;
      for (var i = 0; i < pins.length; i++) if (except.indexOf(pins[i]) < 0 && overlaps(b, pins[i].box, PAD)) return false;
      for (var o = 0; o < obstacles.length; o++) if (overlaps(b, obstacles[o], PAD)) return false;
      for (var t = 0; t < placed.length; t++) if (placed[t] && overlaps(b, placed[t].box, PAD)) return false;
      return true;
    }
    function singleSpots(pn, w, hh) {
      var x = pn.x, y = pn.y, s = sc, hy = y - 13 * s, out = [];
      out.push(box(x - w / 2, y - 22 * s - hh, w, hh));          // above (the usual place)
      out.push(box(x + 9 * s, hy - hh / 2, w, hh));              // right
      out.push(box(x - 9 * s - w, hy - hh / 2, w, hh));          // left
      out.push(box(x - w / 2, y + 4 * s, w, hh));                // below
      out.push(box(x + 5 * s, y - 20 * s - hh, w, hh));          // above right
      out.push(box(x - 5 * s - w, y - 20 * s - hh, w, hh));      // above left
      out.push(box(x + 5 * s, y + 2 * s, w, hh));                // below right
      out.push(box(x - 5 * s - w, y + 2 * s, w, hh));            // below left
      [-0.5, 0.5].forEach(function (f) {                          // nudged
        out.push(box(x - w / 2 + f * w, y - 22 * s - hh, w, hh));
        out.push(box(x - w / 2 + f * w, y + 4 * s, w, hh));
        out.push(box(x + 9 * s, hy - hh / 2 + f * hh, w, hh));
        out.push(box(x - 9 * s - w, hy - hh / 2 + f * hh, w, hh));
      });
      return out;
    }
    // 1. single tags, newest station first
    var failed = [];
    pins.forEach(function (pn) { pn.w = tagWidth(pn.it) * sc; });
    // a pin off the visible map has no tag (its tag would only be pushed back onto the map, away from it)
    var tagged = pins.filter(function (pn) { return pn.x >= 0 && pn.x <= W && pn.y - 10 * sc >= 0 && pn.y <= H; });
    tagged.slice().sort(function (a, b) { return a.order - b.order; }).forEach(function (pn) {
      var spots = singleSpots(pn, pn.w, TAG_H * sc), order = [], got = -1;
      if (prev[pn.key] && prev[pn.key].s >= 0) order.push(prev[pn.key].s);
      for (var i = 0; i < spots.length; i++) if (order.indexOf(i) < 0) order.push(i);
      for (var k = 0; k < order.length && got < 0; k++) if (clear(spots[order[k]], [pn], true)) got = order[k];
      for (k = 0; k < order.length && got < 0; k++) if (clear(spots[order[k]], [pn], false)) got = order[k];
      if (got >= 0) { pn.single = { box: spots[got], members: [pn], spot: got }; placed.push(pn.single); next[pn.key] = { s: got }; }
      else failed.push(pn);
    });
    // 2. stacks: a station without a spot is stacked with the stations whose tags would collide with its own
    var comp = {}, inStack = {};
    function find(a) { while (comp[a] !== a) a = comp[a] = comp[comp[a]]; return a; }
    tagged.forEach(function (pn, i) { comp[i] = i; pn.idx = i; });
    function near(a, b) { return Math.abs(a.x - b.x) < (a.w + b.w) / 2 + 4 * sc && Math.abs(a.y - b.y) < (TAG_H + 24) * sc; }
    var queue = failed.slice();
    failed.forEach(function (f) { inStack[f.idx] = 1; });
    while (queue.length) {
      var f = queue.shift();
      tagged.forEach(function (o) {
        if (o === f || !near(o, f)) return;
        comp[find(o.idx)] = find(f.idx);
        if (!inStack[o.idx]) { inStack[o.idx] = 1; queue.push(o); }
      });
    }
    var byRoot = {};
    tagged.forEach(function (pn) {
      if (!inStack[pn.idx]) return;
      if (pn.single) { placed[placed.indexOf(pn.single)] = null; pn.single = null; delete next[pn.key]; }
      var r = find(pn.idx);
      (byRoot[r] = byRoot[r] || []).push(pn);
    });
    placed = placed.filter(function (x) { return x; });
    var stacks = Object.keys(byRoot).map(function (r) { return byRoot[r].sort(function (a, b) { return a.order - b.order; }); });
    stacks.sort(function (a, b) { return b.length - a.length; });
    stacks.forEach(function (members) {
      var shown = members.slice(0, max), w = 0, P = null, cx = 0, cy = 0;
      shown.forEach(function (pn) { w = Math.max(w, pn.w); });
      members.forEach(function (pn) {
        cx += pn.head.x; cy += pn.head.y;
        P = P ? { l: Math.min(P.l, pn.box.l), t: Math.min(P.t, pn.box.t), r: Math.max(P.r, pn.box.r), b: Math.max(P.b, pn.box.b) } : pn.box;
      });
      cx /= members.length; cy /= members.length;
      var hh = (shown.length * TAG_STEP - (TAG_STEP - TAG_H) + (members.length > max ? MORE_H : 0)) * sc, g = 8 * sc, spots = [];
      [0, 12, 30, 55, 90].forEach(function (d0) {
        var d = d0 * sc;
        [-0.5, -0.25, 0, 0.25, 0.5].forEach(function (f) {
          spots.push(box(cx - w / 2 + f * w, P.t - g - d - hh, w, hh));          // above, centred among the pins
          spots.push(box(cx - w / 2 + f * w, P.b + g + d, w, hh));               // below
          spots.push(box(P.r + g + d, cy - hh / 2 + f * hh, w, hh));             // right
          spots.push(box(P.l - g - d - w, cy - hh / 2 + f * hh, w, hh));         // left
        });
      });
      // the free spot with the shortest lines to its pins (the last draw's spot wins a tie, so tags do not jump)
      function lineLen(b0) {
        var sum = 0;
        shown.forEach(function (pn) {
          var ex = Math.max(b0.l, Math.min(b0.r, pn.head.x)), ey = Math.max(b0.t, Math.min(b0.b, pn.head.y));
          sum += Math.hypot(ex - pn.head.x, ey - pn.head.y);
        });
        return sum;
      }
      var skey = 'S' + members[0].key, got = -1, bestLen = Infinity, pv = prev[skey] ? prev[skey].s : -1;
      [true, false].forEach(function (strict) {
        if (got >= 0) return;
        for (var i = 0; i < spots.length; i++) {
          if (!clear(spots[i], members, strict)) continue;
          // beside the pins first: there every tag's line leaves from its own side without crossing the other tags
          var len = lineLen(spots[i]) - (i === pv ? 12 * sc : 0) + (i % 4 < 2 ? 30 * sc * shown.length : 0);
          if (len < bestLen) { bestLen = len; got = i; }
        }
      });
      var b;
      if (got >= 0) b = spots[got];
      else {
        // nothing is free: the spot that covers the least, moved fully onto the visible map
        var best = Infinity;
        spots.forEach(function (s0) {
          var s1 = self.clampBox(s0, M), cost = lineLen(s1) * 0.01;
          pins.forEach(function (o) { if (members.indexOf(o) < 0) cost += overlapArea(s1, o.box); });
          placed.forEach(function (t) { cost += overlapArea(s1, t.box); });
          obstacles.forEach(function (o) { cost += overlapArea(s1, o); });
          members.forEach(function (o) { cost += overlapArea(s1, o.box) * 2; });
          if (cost < best) { best = cost; b = s1; }
        });
      }
      next[skey] = { s: got };
      // tags in the order of their pins from top to bottom, so the lines do not cross
      shown.sort(function (p1, p2) { return p1.head.y - p2.head.y; });
      var st = { box: b, members: members, shown: shown, stack: true };
      placed.push(st);
    });
    this.prevSpot = next;
    return placed;
  };
  // the page's panels over the map (hint panels, band filter), in map pixels
  LiveMap.prototype.panelBoxes = function () {
    var out = [], ov = this.root.parentNode && this.root.parentNode.querySelectorAll('.overlay .panel');
    if (!ov || !ov.length) return out;
    var r0 = this.root.getBoundingClientRect();
    for (var i = 0; i < ov.length; i++) {
      if (ov[i].hidden) continue;
      var r = ov[i].getBoundingClientRect();
      if (r.width > 0 && r.height > 0) out.push({ l: r.left - r0.left, t: r.top - r0.top, r: r.right - r0.left, b: r.bottom - r0.top });
    }
    return out;
  };
  LiveMap.prototype.clampBox = function (b, M) {
    var w = b.r - b.l, hh = b.b - b.t, l = Math.max(M, Math.min(this.W - M - w, b.l)), t = Math.max(M, Math.min(this.H - M - hh, b.t));
    return box(l, t, w, hh);
  };

  // A pin with its tip on the station's point; hovering or tapping it opens that station's popup, as its tag does.
  LiveMap.prototype.drawPinOnly = function (pn, sc) {
    var g = this.keyed(this.gPin, 'p' + pn.key, { transform: 'translate(' + pn.x.toFixed(1) + ',' + pn.y.toFixed(1) + ') scale(' + sc + ')' });
    var pin = el('g', { 'class': 'lqmap-mark', 'data-kind': 'st', 'data-idx': this.items.indexOf(pn.it), tabindex: -1 }, g);
    // a light rim where the outline is dark on a mid-blue sea, so a dark band colour (60m) still shows
    if (this.theme.pinHalo) el('path', { d: 'M0,0 C-1.5,-5 -6,-8 -6,-13 A6,6 0 1 1 6,-13 C6,-8 1.5,-5 0,0 Z', fill: 'none',
      stroke: this.theme.pinHalo, 'stroke-width': 4, 'stroke-linejoin': 'round' }, pin);
    el('path', { d: 'M0,0 C-1.5,-5 -6,-8 -6,-13 A6,6 0 1 1 6,-13 C6,-8 1.5,-5 0,0 Z', fill: bandColor(pn.it.band),
      stroke: this.theme.pinStroke, 'stroke-width': 1.5, 'class': 'lqmap-pin' }, pin);
    el('circle', { cx: 0, cy: -13, r: 2.3, fill: this.theme.pinStroke }, pin);
  };

  // One tag or a stack of tags; a stack gets a short line from each tag to its own pin.
  LiveMap.prototype.drawTags = function (grp, sc) {
    var list = grp.stack ? grp.shown : grp.members, b = grp.box, mini = this.mini, w = (b.r - b.l) / sc;
    for (var i = 0; i < list.length; i++) {
      var pn = list[i], it = pn.it, col = bandColor(it.band), tw = grp.stack ? w : pn.w / sc;
      var top = b.t + i * TAG_STEP * sc;
      if (grp.stack) {   // the leader: from the tag's nearest edge to the top of its pin
        var ty = top + TAG_H * sc / 2, hx = pn.head.x, hy = pn.y - 19 * sc;
        var ex = hx < (b.l + b.r) / 2 ? b.l : b.r, ey = ty;   // from the side of the tag (lines run under the tags)
        var lg = this.keyed(this.gLead, 'l' + pn.key, {});
        el('line', { x1: ex.toFixed(1), y1: ey.toFixed(1), x2: hx.toFixed(1), y2: hy.toFixed(1), 'class': 'lqmap-leader',
          stroke: this.theme.leaderHalo, 'stroke-width': 3.5 }, lg);
        el('line', { x1: ex.toFixed(1), y1: ey.toFixed(1), x2: hx.toFixed(1), y2: hy.toFixed(1), 'class': 'lqmap-leader',
          stroke: this.theme.leader, 'stroke-width': 1.5 }, lg);
      }
      var g = this.keyed(this.gTag, 't' + pn.key, { transform: 'translate(' + (b.l + (grp.stack ? 0 : 0)).toFixed(1) + ',' + top.toFixed(1) + ') scale(' + sc + ')' });
      var t = el('g', { 'class': 'lqmap-mark', 'data-kind': 'st', 'data-idx': this.items.indexOf(it), tabindex: mini ? -1 : 0 }, g);
      el('rect', { x: 0, y: 0, width: tw, height: TAG_H, rx: 4, fill: col, stroke: this.theme.tagStroke, 'stroke-width': 1 }, t);
      var tc = el('text', { x: tw / 2, y: 11.5, 'text-anchor': 'middle', fill: textOn(col), 'class': 'lqmap-label', direction: 'ltr' }, t);
      tc.textContent = it.call;
      var s2 = el('text', { x: tw / 2, y: 24.5, 'text-anchor': 'middle', fill: textOn(col), 'class': 'lqmap-label lqmap-sub', direction: 'ltr' }, t);
      s2.textContent = it.mode + ' ' + it.band;
    }
    if (grp.stack && grp.members.length > list.length) {
      var mg = this.keyed(this.gTag, 'm' + grp.members[0].key, {});
      var more = el('text', { x: ((b.l + b.r) / 2).toFixed(1), y: (b.b - 3 * sc).toFixed(1), 'text-anchor': 'middle', 'class': 'lqmap-more', direction: 'ltr' }, mg);
      more.textContent = '+' + (grp.members.length - list.length);
    }
  };

  // A diamond on the entity, its call in a magenta tag beside it, the place above and the countdown below.
  // Returns the screen box it covers (station tags keep out of it).
  LiveMap.prototype.drawDx = function (pt, dx, idx, sc) {
    var g = this.keyed(this.gDx, 'd' + dx.call + '#' + pt.k, { transform: 'translate(' + pt.x.toFixed(1) + ',' + pt.y.toFixed(1) + ') scale(' + sc + ')' });
    var grp = el('g', { 'class': 'lqmap-mark', 'data-kind': 'dx', 'data-idx': idx, tabindex: this.mini ? -1 : 0 }, g);
    var T = this.theme, txt = dxClock(dx, Date.now());
    el('path', { d: 'M0,-9 L9,0 L0,9 L-9,0 Z', fill: T.dx, stroke: '#FFFFFF', 'stroke-width': 2 }, grp);
    el('circle', { cx: 0, cy: 0, r: 2.5, fill: '#FFFFFF' }, grp);
    var w = 12 + dx.call.length * 6.6, pw = dx.place ? Math.max(w, 10 + String(dx.place).length * 5.8) : 0;
    var cw = Math.max(w, 10 + txt.length * 5.8), right = 12 + Math.max(w, pw, cw), top = dx.place ? -24 : -10;
    // the tags go to the right of the diamond, or to its left where the right edge of the map would cut them
    var flip = pt.x + (right + 2) * sc > this.W - 3 && pt.x - (right + 2) * sc >= 3;
    function X(x0, ww) { return flip ? -x0 - ww : x0; }
    if (dx.place) {   // the exact place, above the call (e.g. "Corfu Island")
      el('rect', { x: X(12, pw), y: -24, width: pw, height: 14, rx: 4, fill: T.dxBadge, stroke: T.dx, 'stroke-width': 1, opacity: 0.92 }, grp);
      var pl = el('text', { x: X(12, pw) + pw / 2, y: -13.5, 'text-anchor': 'middle', fill: T.dxBadgeText, 'class': 'lqmap-label lqmap-sub' }, grp);
      pl.textContent = dx.place;
    }
    el('rect', { x: X(12, w), y: -8, width: w, height: 16, rx: 4, fill: T.dx, stroke: T.dxStroke, 'stroke-width': 1 }, grp);
    var t = el('text', { x: X(12, w) + w / 2, y: 3.5, 'text-anchor': 'middle', fill: '#FFFFFF', 'class': 'lqmap-label', direction: 'ltr' }, grp);
    t.textContent = dx.call;
    el('rect', { x: X(12, cw), y: 10, width: cw, height: 14, rx: 4, fill: T.dxBadge, stroke: T.dx, 'stroke-width': 1, opacity: 0.92 }, grp);
    var ck = el('text', { x: X(12, cw) + cw / 2, y: 20.5, 'text-anchor': 'middle', fill: T.dxClock, 'class': 'lqmap-label lqmap-sub lqmap-dxclock' }, grp);
    ck.textContent = txt;
    this.dxClocks.push({ node: ck, dx: dx });
    return flip ? { l: pt.x - (right + 2) * sc, t: pt.y + top * sc, r: pt.x + 10 * sc, b: pt.y + 25 * sc }
      : { l: pt.x - 10 * sc, t: pt.y + top * sc, r: pt.x + (right + 2) * sc, b: pt.y + 25 * sc };
  };
  // every second: the markers' clocks and the open popup's countdown, nothing else redrawn
  LiveMap.prototype.tickDx = function () {
    var now = Date.now(), list = this.dxClocks || [];
    for (var i = 0; i < list.length; i++) if (list[i].node.isConnected) list[i].node.textContent = dxClock(list[i].dx, now);
    var pc = this.pop && !this.pop.hidden && this.pop.querySelector('.lqmap-pop-clock');
    if (pc && pc._dx) pc.textContent = dxClock(pc._dx, now);
  };

  LiveMap.prototype.showPop = function (kind, idx, pin) {
    var now = Date.now(), p = this.pop;
    p.textContent = '';
    if (kind === 'st') {
      var it = this.items[idx];
      if (!it) return;
      h('div', 'lqmap-pop-call', it.call, p);
      var rows = [];
      var ent = dxccOf(it.call, this, kind, idx);
      if (ent && ent.name) {
        rows.push([str('LIVE_DXCC_ENTITY', 'DXCC entity'), ent.name], [str('LIVE_DXCC_PREFIX', 'Prefix'), ent.prefix]);
        if (ent.dxcc > 0) rows.push([str('LIVE_DXCC_NUMBER', 'DXCC number'), String(ent.dxcc)]);
      }
      rows.push([str('LIVE_BAND', 'Band'), it.mode + ' ' + it.band], [str('LIVE_FREQ', 'Frequency'), freqText(it.dial_hz)],
        [str('LIVE_GRID', 'Grid'), it.grid], [str('LIVE_HEARD', 'Last active'), minutesAgo(it.tx_ms, now)]);
      for (var i = 0; i < rows.length; i++) {
        var r = h('div', 'lqmap-pop-row', null, p);
        h('span', 'k', rows[i][0], r); h('span', 'v', rows[i][1], r);
      }
      this.popGeo = { lon: it.lon, lat: it.lat };
    } else {
      var d = this.dx[idx];
      if (!d) return;
      h('div', 'lqmap-pop-call dx', d.call, p);
      h('div', 'lqmap-pop-row lqmap-pop-planned', str('LIVE_DX_PLANNED', 'Planned LQ8 DXpedition'), p);
      if (d.place || d.name) h('div', 'lqmap-pop-row', (d.place ? d.place + (d.name ? ', ' : '') : '') + (d.name || ''), p);
      h('div', 'lqmap-pop-row', d.start + (d.time ? ' ' + d.time + ' UTC' : '') + (d.end !== d.start ? ' – ' + d.end : ''), p);
      var loc = dxLocal(d);
      if (loc) h('div', 'lqmap-pop-row lqmap-pop-local', loc, p);
      if (d.bands) h('div', 'lqmap-pop-row', d.bands, p);
      var pc = h('div', 'lqmap-pop-count lqmap-pop-clock', dxClock(d, now), p);   // the live countdown (no second "in N days")
      pc._dx = d;
      this.popGeo = { lon: d.lon, lat: d.lat };
    }
    this.popKey = kind + idx;
    this.pinned = !!pin;
    p.hidden = false;
    this.placePop();
  };
  LiveMap.prototype.placePop = function () {
    if (!this.popGeo) return;
    var cs = this.copies(this.popGeo.lon, this.popGeo.lat);
    if (!cs.length) { this.hidePop(); return; }
    var pt = cs[0], best = Infinity;
    for (var i = 0; i < cs.length; i++) { var dd = Math.abs(cs[i].x - this.W / 2); if (dd < best) { best = dd; pt = cs[i]; } }
    var p = this.pop, pw = p.offsetWidth, ph = p.offsetHeight;
    var x = Math.min(Math.max(8, pt.x - pw / 2), this.W - pw - 8);
    var y = pt.y + 14;
    if (y + ph > this.H - 8) y = Math.max(8, pt.y - ph - 40);
    p.style.left = x + 'px'; p.style.top = y + 'px';
  };
  LiveMap.prototype.hidePop = function () { this.pop.hidden = true; this.popKey = null; this.popGeo = null; this.pinned = false; };

  LiveMap.prototype.setData = function (items, dx, fade) {
    this.items = items;
    this.dx = dx;
    var key = this.popKey;
    this.hidePop();
    this.render(!!fade);   // fade only for changes AFTER the first data was shown: the page opens with everything in place
    return key;
  };

  // ---- data --------------------------------------------------------------------------------------------------------
  function cleanItems(j, now) {
    var out = [];
    var list = j && Array.isArray(j.cqs) ? j.cqs : [];
    for (var i = 0; i < list.length && out.length < 2000; i++) {
      var c = list[i];
      if (!c || typeof c.call !== 'string' || !CALL_OK.test(c.call) || typeof c.grid !== 'string' || !GRID_OK.test(c.grid)) continue;
      if (typeof c.band !== 'string' || typeof c.mode !== 'string' || MODE_ORDER.indexOf(c.mode) < 0) continue;
      if (typeof c.dial_hz !== 'number' || typeof c.tx_ms !== 'number' || now - c.tx_ms > MAX_AGE_MS) continue;
      var gc = gridCenter(c.grid), pt = inSquare(c, gc) ? c : gc;
      out.push({ call: c.call, grid: c.grid, band: c.band, mode: c.mode, dial_hz: c.dial_hz, tx_ms: c.tx_ms, lon: pt.lon, lat: pt.lat });
    }
    out.sort(function (a, b) { return b.tx_ms - a.tx_ms; });
    return out;
  }
  function sorted(keys, order) {
    return keys.sort(function (a, b) {
      var ia = order.indexOf(a), ib = order.indexOf(b);
      return (ia < 0 ? 99 : ia) - (ib < 0 ? 99 : ib) || (a < b ? -1 : 1);
    });
  }

  // ---- the page around the map -------------------------------------------------------------------------------------
  function Page(root) {
    var self = this;
    this.root = root;
    this.mini = root.getAttribute('data-mini') === '1';
    this.map = new LiveMap(root.querySelector('.lqmap-host'), { mini: this.mini, href: root.getAttribute('data-href') });
    this.status = root.querySelector('.lqlive-status');
    this.legend = root.querySelector('.lqlive-legend');
    this.dxBox = root.querySelector('.lqlive-dx');
    this.dxList = root.querySelector('.lqlive-dx-list');
    this.raw = null; this.dxRaw = []; this.failed = false;
    // the DXpeditions written into the page by the site build: their markers are there on the first paint
    try {
      var emb = document.getElementById('lqlive-dx-embed'), ej = emb && JSON.parse(emb.textContent || 'null');
      if (ej && Array.isArray(ej.dxpeditions)) this.dxRaw = ej.dxpeditions.filter(validDx);
    } catch (e) {}
    // the band filter (full map only): every band the service accepts, all shown unless the viewer hid some
    this.bandBox = this.mini ? null : root.querySelector('.lqlive-bands');
    if (this.bandBox) {
      this.map.hiddenBands = readHiddenBands();
      this.bandBox.setAttribute('aria-label', str('LIVE_BANDS_FILTER', 'Bands to show'));
      var all = this.bandBox.querySelector('.lqlive-bands-all');
      if (all) all.addEventListener('click', function () {
        Object.keys(self.map.hiddenBands).forEach(function (b) { delete self.map.hiddenBands[b]; });
        saveHiddenBands(self.map.hiddenBands);
        self.update();
      });
      this.drawBandFilter();   // the list is fixed, so it is drawn at once, not when the first data arrives
    }
    var reset = root.querySelector('.lqlive-reset');
    if (reset) reset.addEventListener('click', function () { self.map.reset(); });
    var how = root.querySelector('.lqlive-how');
    if (how) {
      var path = str('LIVE_PATH_SETTINGS', 'Settings') + ' › ' + str('LIVE_PATH_REPORTING', 'Reporting') + ' › “' +
        str('LIVE_PATH_LABEL', 'Announce LQ8 activity to the qFT8 service') + '”';
      how.textContent = str('LIVE_HOW', 'To appear on the map, in qFT8 go to {path}, then transmit in LQ8 through your radio.').replace('{path}', path);
    }
    this.setStatus(str('LIVE_LOADING', 'Loading live data…'));
    if (this.dxRaw.length) this.update();   // the embedded DXpeditions: drawn at once, before any request returns
    this.loadDx();
    this.poll();
    setInterval(function () { self.poll(); }, POLL_MS);
    setInterval(function () { if (!document.hidden && self.dxRaw && self.dxRaw.length) self.update(); }, 60000);   // the DX list's day / hour wording
    setInterval(function () { if (!document.hidden && self.map && self.map.tickDx) self.map.tickDx(); }, 1000);   // the live DX clocks
    document.addEventListener('visibilitychange', function () { if (!document.hidden) self.poll(); });
  }
  Page.prototype.setStatus = function (t, cls) {
    if (!this.status) return;
    this.status.textContent = t;
    this.status.className = 'lqlive-status' + (cls ? ' ' + cls : '');
  };
  Page.prototype.poll = function () {
    var self = this;
    if (document.hidden && this.raw) return;
    // the first poll uses the request the page's head already started (window.__lqFeedEarly), so the stations
    // arrive while the map scripts are still loading
    var early = !this.raw && FEED === FEED_DEFAULT && window.__lqFeedEarly;
    window.__lqFeedEarly = null;
    (early || fetch(FEED, { credentials: 'omit', cache: 'no-cache' }).then(function (r) {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      return r.json();
    })).then(function (j) {
      if (!j) throw new Error('no data');
      self.raw = j; self.failed = false; self.update();
    }).catch(function () {
      self.failed = true; self.update();
    });
  };
  Page.prototype.loadDx = function () {
    var self = this, url;
    try { url = new URL('dxpeditions.json', SCRIPT_SRC || location.href).href; } catch (e) { return; }
    fetch(url, { cache: 'no-cache' }).then(function (r) { return r.ok ? r.json() : null; }).then(function (j) {
      self.dxRaw = j && Array.isArray(j.dxpeditions) ? j.dxpeditions.filter(validDx) : [];
      self.update();
    }).catch(function () {});
  };
  Page.prototype.update = function () {
    var now = Date.now(), self = this;
    var items = cleanItems(this.raw, now);
    var dx = [];
    for (var i = 0; i < this.dxRaw.length; i++) {
      var st = dxStatus(this.dxRaw[i], now);
      if (st) dx.push(Object.assign({}, this.dxRaw[i], { st: st }));
    }
    dx.sort(function (a, b) { return utcDay(a.start) - utcDay(b.start); });
    // the first live data (and the DXpeditions written into the page) appear at once; later arrivals / departures fade
    this.map.setData(items, dx, !!this.shownLive);
    if (this.raw) this.shownLive = true;
    // status line
    if (this.failed && !this.raw) this.setStatus(str('LIVE_ERROR', 'The live data cannot be reached right now. Retrying…'), 'err');
    else if (!this.raw) this.setStatus(str('LIVE_LOADING', 'Loading live data…'));
    else if (!items.length) this.setStatus(str('LIVE_EMPTY', 'No LQ8 activity right now.'), 'empty');
    else {
      var calls = {};
      items.forEach(function (it) { calls[it.call] = 1; });
      this.setStatus(str('LIVE_COUNT', 'Active stations: {n}').replace('{n}', String(Object.keys(calls).length)), 'ok');
    }
    // legend: the bands (and, when more than one, the modes) on the air; a tap hides / shows them
    if (this.legend) {
      this.legend.textContent = '';
      var bands = {}, modes = {};
      items.forEach(function (it) { bands[it.band] = 1; modes[it.mode] = 1; });
      var ml = sorted(Object.keys(modes), MODE_ORDER);
      if (ml.length > 1) ml.forEach(function (m) { self.chip(m, null, self.map.hiddenModes); });
      sorted(Object.keys(bands), BAND_ORDER).forEach(function (b) { self.chip(b, bandColor(b), self.map.hiddenBands); });
      this.legend.hidden = !items.length;
    }
    this.drawBandFilter();
    // DXpeditions list: hidden while there are none
    if (this.dxBox) {
      if (dxListed !== (dx.length > 0)) { dxListed = dx.length > 0; if (reapplyHints) reapplyHints(); }
      this.dxBox.hidden = !dx.length || hintClosed('dx');
      if (this.dxList) {
        this.dxList.textContent = '';
        dx.forEach(function (d, idx) {
          var li = h('li', d.st.state === 'on' ? 'on' : '', null, self.dxList);
          var b = h('button', 'lqlive-dx-item', null, li);
          b.type = 'button';
          h('span', 'call', d.call, b);
          if (d.place || d.name) h('span', 'name', d.place || d.name, b);
          h('span', 'count', dxCounter(d.st), b);
          b.addEventListener('click', function () {
            if (self.mini) return;
            self.map.showPop('dx', idx, true);
          });
        });
      }
    }
  };
  Page.prototype.drawBandFilter = function () {
    var list = this.bandBox && this.bandBox.querySelector('.lqlive-band-list');
    if (!list) return;
    var self = this;
    list.textContent = '';
    BAND_ORDER.forEach(function (b) { self.chip(b, bandColor(b), self.map.hiddenBands, list); });
    var all = this.bandBox.querySelector('.lqlive-bands-all');
    if (all) all.disabled = !Object.keys(this.map.hiddenBands).length;
  };
  Page.prototype.chip = function (label, color, hidden, parent) {
    var self = this, b = h('button', 'lqlive-chip' + (hidden[label] ? ' off' : ''), null, parent || this.legend);
    b.setAttribute('aria-pressed', hidden[label] ? 'false' : 'true');
    b.type = 'button';
    if (color) { var sw = h('span', 'sw', null, b); sw.style.background = color; }
    h('span', null, label, b);
    if (this.mini) { b.disabled = true; return; }
    b.addEventListener('click', function () {
      if (hidden[label]) delete hidden[label]; else hidden[label] = 1;
      if (hidden === self.map.hiddenBands && self.bandBox) saveHiddenBands(hidden);
      self.update();
    });
  };

  // The map theme (top bar of the full map), kept in a cookie for a year; the small map on the main page follows it.
  function setupTheme(pages) {
    var sel = document.querySelector('.lqlive-theme');
    if (!sel) return;
    var t = str('LIVE_THEME', 'Map theme');
    sel.setAttribute('aria-label', t); sel.title = t;
    sel.value = String(readTheme());
    sel.addEventListener('change', function () {
      var i = parseInt(sel.value, 10) || 0;
      writeCookieList(THEME_COOKIE, [String(i)]);
      pages.forEach(function (p) { p.map.applyTheme(THEMES[i]); });
    });
  }
  function start() {
    var roots = document.querySelectorAll('.lqlive');
    var pages = [];
    for (var i = 0; i < roots.length; i++) pages.push(new Page(roots[i]));
    // local testing only: the maps, to point the camera from a test (never on the real site)
    if (location.hostname === 'localhost' || location.hostname === '127.0.0.1') window.__lqTest = {
      maps: pages.map(function (p) { return p.map; }),
      view: function (lon, lat, zoom, i) {
        var m = this.maps[i || 0]; m.st.zoom = zoom; var s = m.scale();
        m.st.offsetX = -lon * s; m.st.offsetY = lat * s; m.clamp(); m.render();
      },
      theme: function (t, i) { this.maps[i || 0].applyTheme(THEMES[t]); }
    };
    setupHints();
    setupFullscreen();
    setupTheme(pages);
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', start); else start();
})();
