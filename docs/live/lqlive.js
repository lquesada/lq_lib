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

  var THEME = {
    ocean: '#1E477A', land: '#3B82B6', coast: '#CBD5E1', grid: 'rgba(255,255,255,0.22)',
    border: 'rgba(226,232,240,0.7)', pin: '#0F172A', pinStroke: '#F8FAFC', dx: '#D81B78', dxStroke: '#0F172A'
  };
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
      for (var i = 0; i < panels.length; i++) {
        var id = panels[i].getAttribute('data-hint');
        panels[i].hidden = closed.indexOf(id) >= 0 || (id === 'dx' && !dxListed);
      }
      if (show) show.hidden = !closed.length;
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
    if (show) show.addEventListener('click', function () { closed = []; writeCookieList(HINTS_COOKIE, closed); apply(); });
    reapplyHints = apply;
    apply();
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
  // The screen box of a pin at (x, y) with the stack of tags drawn above it by drawPin (same sizes), in map pixels.
  function tagWidth(it) { return 10 + Math.max((it.mode + ' ' + it.band).length * 6.6, it.call.length * 7.6); }
  function stackBox(x, y, list, max, sc) {
    var shown = Math.min(list.length, max), w = 12;
    for (var i = 0; i < shown; i++) w = Math.max(w, tagWidth(list[i]));
    var top = -22 - 31 * (shown - 1) - 29 - (list.length > max ? 16 : 0);
    return { l: x - (w / 2 + 3) * sc, r: x + (w / 2 + 3) * sc, t: y + (top - 3) * sc, b: y + 2 * sc };
  }
  function overlaps(a, b) { return a.l < b.r && b.l < a.r && a.t < b.b && b.t < a.b; }
  function union(a, b, c) {
    return { l: Math.min(a.l, b.l, c.l), r: Math.max(a.r, b.r, c.r), t: Math.min(a.t, b.t, c.t), b: Math.max(a.b, b.b, c.b) };
  }
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
    this.build();
  }

  LiveMap.prototype.build = function () {
    var self = this;
    this.root.classList.add('lqmap');
    this.svg = el('svg', { 'class': 'lqmap-svg', role: 'img', 'aria-label': str('LIVE_TITLE', 'LQ8 live map') }, this.root);
    el('rect', { x: 0, y: 0, width: '100%', height: '100%', fill: THEME.ocean }, this.svg);
    var defs = el('defs', {}, this.svg);
    var tile = el('g', { id: 'lqtile' + Math.random().toString(36).slice(2) }, defs);
    // Maidenhead fields (20 x 10 degrees)
    var gd = '';
    for (var x = 0; x <= 360; x += 20) gd += 'M' + x + ',0V180';
    for (var y = 0; y <= 180; y += 10) gd += 'M0,' + y + 'H360';
    el('path', { d: window.LQ8_WORLD_LAND || '', fill: THEME.land, stroke: THEME.coast, 'stroke-width': 1.35,
      'vector-effect': 'non-scaling-stroke', 'stroke-linejoin': 'round' }, tile);
    // country borders (countries.js), thinner and fainter than the coast
    if (window.LQ8_COUNTRY_BORDERS) el('path', { d: window.LQ8_COUNTRY_BORDERS, fill: 'none', stroke: THEME.border,
      'stroke-width': 1.0, 'vector-effect': 'non-scaling-stroke', 'stroke-linejoin': 'round' }, tile);
    el('path', { d: gd, fill: 'none', stroke: THEME.grid, 'stroke-width': 0.6, 'vector-effect': 'non-scaling-stroke' }, tile);
    this.world = el('g', {}, this.svg);
    for (var k = -2; k <= 2; k++) el('use', { href: '#' + tile.id, x: k * 360, y: 0 }, this.world);
    this.names = el('g', { 'class': 'lqmap-names' }, this.svg);
    this.marks = el('g', {}, this.svg);
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

  LiveMap.prototype.render = function () {
    var r = this.root.getBoundingClientRect();
    this.W = Math.max(1, r.width); this.H = Math.max(1, r.height);
    this.clamp();
    var s = this.scale();
    var tx = this.W / 2 + this.st.offsetX - 180 * s, ty = this.H / 2 + this.st.offsetY - 90 * s;
    this.world.setAttribute('transform', 'translate(' + tx + ',' + ty + ') scale(' + s + ')');
    this.drawNames();
    this.drawMarks();
    if (this.popKey) this.placePop();
  };

  // Every copy of a point that is on screen (the map repeats sideways).
  LiveMap.prototype.copies = function (lon, lat) {
    var p = this.toScreen(lon, lat), w = 360 * this.scale(), out = [];
    for (var k = -2; k <= 2; k++) {
      var x = p.x + k * w;
      if (x > -60 && x < this.W + 60) out.push({ x: x, y: p.y });
    }
    return out;
  };

  // Country names on the full map once zoomed in (the same zoom 1.8 as the app's world map).
  var NAMES_MIN_ZOOM = 1.8;
  LiveMap.prototype.drawNames = function () {
    var g = this.names;
    while (g.firstChild) g.removeChild(g.firstChild);
    var list = window.LQ8_COUNTRY_NAMES;
    if (this.mini || !list || this.st.zoom < NAMES_MIN_ZOOM) return;
    for (var i = 0; i < list.length; i++) {
      var cs = this.copies(list[i][1], list[i][2]);
      for (var c = 0; c < cs.length; c++) {
        if (cs[c].y < -20 || cs[c].y > this.H + 20) continue;
        var t = el('text', { x: cs[c].x.toFixed(1), y: cs[c].y.toFixed(1), 'text-anchor': 'middle', 'class': 'lqmap-country' }, g);
        t.textContent = list[i][0];
      }
    }
  };

  LiveMap.prototype.drawMarks = function () {
    var g = this.marks;
    while (g.firstChild) g.removeChild(g.firstChild);
    this.dxClocks = [];
    var sc = this.mini ? 1.2 : 1.45;
    // DXpeditions first (underneath the stations)
    for (var d = 0; d < this.dx.length; d++) {
      var dx = this.dx[d];
      var cs = this.copies(dx.lon, dx.lat);
      for (var c = 0; c < cs.length; c++) this.drawDx(g, cs[c], dx, d, sc);
    }
    // stations: each at its own point (the one the qFT8 app draws it at: inside its grid square, on land). A station whose
    // pin + tag would touch another stack of tags joins that stack (newest first, above the stack's first pin; it keeps its
    // own pin), so tags never cover each other; zooming in pulls the stacks apart. Old feeds without lat/lon: grid centres.
    var vis = this.visibleItems(), s = this.scale(), max = this.mini ? 2 : 4, clusters = [];
    for (var i = 0; i < vis.length; i++) {
      var it = vis[i], x = it.lon * s, y = -it.lat * s, cl = null;   // map pixels (the shared screen offset left out)
      var own = stackBox(x, y, [it], max, sc);
      for (var k = 0; k < clusters.length && !cl; k++) if (overlaps(clusters[k].box, own)) cl = clusters[k];
      if (cl) {
        cl.list.push(it);
        cl.box = union(stackBox(cl.x, cl.y, cl.list, max, sc), cl.box, own);
      } else clusters.push({ x: x, y: y, list: [it], box: own });
    }
    for (var c = 0; c < clusters.length; c++) {
      var list = clusters[c].list, ps = this.copies(list[0].lon, list[0].lat);
      for (var p = 0; p < ps.length; p++) {
        for (var m = list.length - 1; m >= 1; m--) {   // the other pins of the stack, at their own points
          this.drawPinOnly(g, { x: ps[p].x + (list[m].lon - list[0].lon) * s, y: ps[p].y - (list[m].lat - list[0].lat) * s }, list[m], sc);
        }
        this.drawPin(g, ps[p], list, sc);
      }
    }
  };

  // A pin with its tip on the station's point; hovering or tapping it opens that station's popup, as its tag does.
  LiveMap.prototype.drawPinOnly = function (g, pt, it, sc) {
    var grp = el('g', { transform: 'translate(' + pt.x.toFixed(1) + ',' + pt.y.toFixed(1) + ') scale(' + sc + ')' }, g);
    var pin = el('g', { 'class': 'lqmap-mark', 'data-kind': 'st', 'data-idx': this.items.indexOf(it), tabindex: -1 }, grp);
    el('path', { d: 'M0,0 C-1.5,-5 -6,-8 -6,-13 A6,6 0 1 1 6,-13 C6,-8 1.5,-5 0,0 Z', fill: bandColor(it.band),
      stroke: THEME.pin, 'stroke-width': 1.5 }, pin);
    el('circle', { cx: 0, cy: -13, r: 2.3, fill: THEME.pin }, pin);
    return grp;
  };

  LiveMap.prototype.drawPin = function (g, pt, list, sc) {
    var grp = this.drawPinOnly(g, pt, list[0], sc);
    var max = this.mini ? 2 : 4, y = -22;
    for (var i = 0; i < list.length && i < max; i++) {
      // each tag: the callsign on top, "LQ8 20m" under it
      var it = list[i], label = it.mode + ' ' + it.band;
      var w = 10 + Math.max(label.length * 6.6, it.call.length * 7.6), col = bandColor(it.band);
      var lg = el('g', { 'class': 'lqmap-mark', 'data-kind': 'st', 'data-idx': this.items.indexOf(it), tabindex: this.mini ? -1 : 0 }, grp);
      el('rect', { x: -w / 2, y: y - 29, width: w, height: 29, rx: 4, fill: col, stroke: THEME.pin, 'stroke-width': 1 }, lg);
      var tc = el('text', { x: 0, y: y - 17.5, 'text-anchor': 'middle', fill: textOn(col), 'class': 'lqmap-label' }, lg);
      tc.textContent = it.call;
      var t = el('text', { x: 0, y: y - 4.5, 'text-anchor': 'middle', fill: textOn(col), 'class': 'lqmap-label lqmap-sub' }, lg);
      t.textContent = label;
      y -= 31;
    }
    if (list.length > max) {
      var more = el('text', { x: 0, y: y - 4, 'text-anchor': 'middle', 'class': 'lqmap-more' }, grp);
      more.textContent = '+' + (list.length - max);
    }
  };

  LiveMap.prototype.drawDx = function (g, pt, dx, idx, sc) {
    var grp = el('g', { transform: 'translate(' + pt.x.toFixed(1) + ',' + pt.y.toFixed(1) + ') scale(' + sc + ')',
      'class': 'lqmap-mark', 'data-kind': 'dx', 'data-idx': idx, tabindex: this.mini ? -1 : 0 }, g);
    // a diamond on the entity, its call in a magenta tag beside it
    el('path', { d: 'M0,-9 L9,0 L0,9 L-9,0 Z', fill: THEME.dx, stroke: '#FFFFFF', 'stroke-width': 2 }, grp);
    el('circle', { cx: 0, cy: 0, r: 2.5, fill: '#FFFFFF' }, grp);
    var w = 12 + dx.call.length * 6.6;
    el('rect', { x: 12, y: -8, width: w, height: 16, rx: 4, fill: THEME.dx, stroke: THEME.dxStroke, 'stroke-width': 1 }, grp);
    var t = el('text', { x: 12 + w / 2, y: 3.5, 'text-anchor': 'middle', fill: '#FFFFFF', 'class': 'lqmap-label' }, grp);
    t.textContent = dx.call;
    var txt = dxClock(dx, Date.now()), cw = Math.max(w, 10 + txt.length * 5.8);
    el('rect', { x: 12, y: 10, width: cw, height: 14, rx: 4, fill: '#0F172A', stroke: THEME.dx, 'stroke-width': 1, opacity: 0.92 }, grp);
    var ck = el('text', { x: 12 + cw / 2, y: 20.5, 'text-anchor': 'middle', fill: '#F9A8D4', 'class': 'lqmap-label lqmap-sub lqmap-dxclock' }, grp);
    ck.textContent = txt;
    (this.dxClocks = this.dxClocks || []).push({ node: ck, dx: dx });
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
      if (d.name) h('div', 'lqmap-pop-row', d.name, p);
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

  LiveMap.prototype.setData = function (items, dx) {
    this.items = items;
    this.dx = dx;
    var key = this.popKey;
    this.hidePop();
    this.render();
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
    this.map.setData(items, dx);
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
      dxListed = dx.length > 0;
      this.dxBox.hidden = !dx.length || hintClosed('dx');
      if (this.dxList) {
        this.dxList.textContent = '';
        dx.forEach(function (d, idx) {
          var li = h('li', d.st.state === 'on' ? 'on' : '', null, self.dxList);
          var b = h('button', 'lqlive-dx-item', null, li);
          b.type = 'button';
          h('span', 'call', d.call, b);
          if (d.name) h('span', 'name', d.name, b);
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

  function start() {
    var roots = document.querySelectorAll('.lqlive');
    for (var i = 0; i < roots.length; i++) new Page(roots[i]);
    setupHints();
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', start); else start();
})();
