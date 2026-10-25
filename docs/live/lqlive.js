// LQ live map (lq8.org/live and the small map on the main page).
// Shows the stations active in the LQ modes (LQ8, LQ4, LQ2, LQ16) in the last 10 minutes, as reported by qFT8 users,
// and the upcoming LQ DXpeditions listed in dxpeditions.json. No frameworks, no tiles, no trackers: one SVG world map
// (land outlines from world.js), equirectangular, pan + zoom. The projection, pan/zoom handling and the map colours
// follow the qFT8 web remote's world map (its default "Bright Future" theme).
(function () {
  'use strict';

  var FEED = 'https://data.qft8.com/lqlive.json';
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
    pin: '#0F172A', pinStroke: '#F8FAFC', dx: '#D81B78', dxStroke: '#0F172A'
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
  function setupHints() {
    var panels = document.querySelectorAll('[data-hint]');
    var show = document.querySelector('.lqlive-hints-show');
    if (!panels.length) return;
    var closed = readCookieList(HINTS_COOKIE);
    function apply() {
      for (var i = 0; i < panels.length; i++) panels[i].hidden = closed.indexOf(panels[i].getAttribute('data-hint')) >= 0;
      if (show) show.hidden = !closed.length;
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
  function minutesAgo(ms, now) {
    var m = Math.floor((now - ms) / 60000);
    return m < 1 ? str('LIVE_JUST_NOW', 'just now') : str('LIVE_MIN_AGO', '{n} min ago').replace('{n}', String(m));
  }
  function freqText(hz) { return (hz / 1e6).toFixed(3) + ' MHz'; }
  var GRID_OK = /^[A-R]{2}[0-9]{2}$/, CALL_OK = /^[A-Z0-9/]{3,13}$/;

  // ---- DXpeditions -------------------------------------------------------------------------------------------------
  var DAY = 86400000;
  function utcDay(s) { var m = /^(\d{4})-(\d{2})-(\d{2})$/.exec(s || ''); return m ? Date.UTC(+m[1], +m[2] - 1, +m[3]) : NaN; }
  // -> null (ended / invalid) or {state:'on'|'soon', days}
  function dxStatus(d, now) {
    var a = utcDay(d.start), b = utcDay(d.end);
    if (isNaN(a) || isNaN(b)) return null;
    if (now >= b + DAY) return null;
    if (now >= a) return { state: 'on', days: 0 };
    return { state: 'soon', days: Math.ceil((a - now) / DAY) };
  }
  function dxCounter(st) {
    if (st.state === 'on') return str('LIVE_DX_ON_AIR', 'on the air now');
    if (st.days <= 1) return str('LIVE_DX_TOMORROW', 'tomorrow');
    return str('LIVE_DX_DAYS', 'in {n} days').replace('{n}', String(st.days));
  }
  function validDx(d) {
    return d && typeof d.call === 'string' && CALL_OK.test(d.call) && typeof d.lat === 'number' && typeof d.lon === 'number' &&
      Math.abs(d.lat) <= 90 && Math.abs(d.lon) <= 180;
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
    el('path', { d: window.LQ8_WORLD_LAND || '', fill: THEME.land, stroke: THEME.coast, 'stroke-width': 0.8,
      'vector-effect': 'non-scaling-stroke', 'stroke-linejoin': 'round' }, tile);
    el('path', { d: gd, fill: 'none', stroke: THEME.grid, 'stroke-width': 0.6, 'vector-effect': 'non-scaling-stroke' }, tile);
    this.world = el('g', {}, this.svg);
    for (var k = -2; k <= 2; k++) el('use', { href: '#' + tile.id, x: k * 360, y: 0 }, this.world);
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

  LiveMap.prototype.drawMarks = function () {
    var g = this.marks;
    while (g.firstChild) g.removeChild(g.firstChild);
    var sc = this.mini ? 1.2 : 1.45;
    // DXpeditions first (underneath the stations)
    for (var d = 0; d < this.dx.length; d++) {
      var dx = this.dx[d];
      var cs = this.copies(dx.lon, dx.lat);
      for (var c = 0; c < cs.length; c++) this.drawDx(g, cs[c], dx, d, sc);
    }
    // stations, grouped by grid square: one pin, labels stacked above it
    var groups = {}, order = [], vis = this.visibleItems();
    for (var i = 0; i < vis.length; i++) {
      var it = vis[i];
      if (!groups[it.grid]) { groups[it.grid] = []; order.push(it.grid); }
      groups[it.grid].push(it);
    }
    for (var o = 0; o < order.length; o++) {
      var list = groups[order[o]], gc = gridCenter(order[o]);
      var ps = this.copies(gc.lon, gc.lat);
      for (var p = 0; p < ps.length; p++) this.drawPin(g, ps[p], list, sc);
    }
  };

  LiveMap.prototype.drawPin = function (g, pt, list, sc) {
    var grp = el('g', { transform: 'translate(' + pt.x.toFixed(1) + ',' + pt.y.toFixed(1) + ') scale(' + sc + ')' }, g);
    // the pin: its tip on the grid square's centre
    el('path', { d: 'M0,0 C-1.5,-5 -6,-8 -6,-13 A6,6 0 1 1 6,-13 C6,-8 1.5,-5 0,0 Z', fill: bandColor(list[0].band),
      stroke: THEME.pin, 'stroke-width': 1.5 }, grp);
    el('circle', { cx: 0, cy: -13, r: 2.3, fill: THEME.pin }, grp);
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
  };

  LiveMap.prototype.showPop = function (kind, idx, pin) {
    var now = Date.now(), p = this.pop;
    p.textContent = '';
    if (kind === 'st') {
      var it = this.items[idx];
      if (!it) return;
      h('div', 'lqmap-pop-call', it.call, p);
      var rows = [[str('LIVE_BAND', 'Band'), it.mode + ' ' + it.band], [str('LIVE_FREQ', 'Frequency'), freqText(it.dial_hz)],
        [str('LIVE_GRID', 'Grid'), it.grid], [str('LIVE_HEARD', 'Last active'), minutesAgo(it.tx_ms, now)]];
      for (var i = 0; i < rows.length; i++) {
        var r = h('div', 'lqmap-pop-row', null, p);
        h('span', 'k', rows[i][0], r); h('span', 'v', rows[i][1], r);
      }
      var gc = gridCenter(it.grid);
      this.popGeo = gc;
    } else {
      var d = this.dx[idx];
      if (!d) return;
      h('div', 'lqmap-pop-call dx', d.call, p);
      if (d.name) h('div', 'lqmap-pop-row', d.name, p);
      h('div', 'lqmap-pop-row', d.start + ' – ' + d.end, p);
      if (d.bands) h('div', 'lqmap-pop-row', d.bands, p);
      h('div', 'lqmap-pop-count', dxCounter(d.st), p);
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
      out.push({ call: c.call, grid: c.grid, band: c.band, mode: c.mode, dial_hz: c.dial_hz, tx_ms: c.tx_ms });
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
    // the band filter (full map only): every band the service accepts, all shown unless the viewer hid some
    this.bandBox = this.mini ? null : root.querySelector('.lqlive-bands');
    if (this.bandBox) {
      this.map.hiddenBands = readHiddenBands();
      if (window.matchMedia && window.matchMedia('(max-width: 760px)').matches) this.bandBox.open = false;
      var all = this.bandBox.querySelector('.lqlive-bands-all');
      if (all) all.addEventListener('click', function () {
        Object.keys(self.map.hiddenBands).forEach(function (b) { delete self.map.hiddenBands[b]; });
        saveHiddenBands(self.map.hiddenBands);
        self.update();
      });
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
    this.loadDx();
    this.poll();
    setInterval(function () { self.poll(); }, POLL_MS);
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
    fetch(FEED, { credentials: 'omit', cache: 'no-cache' }).then(function (r) {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      return r.json();
    }).then(function (j) {
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
      this.dxBox.hidden = !dx.length;
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
