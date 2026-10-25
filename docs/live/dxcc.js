// DXCC entity of a callsign for the live map popup, resolved with the same rules as qFT8's own resolver
// (cty.dat prefixes and exact calls, longest prefix, compound calls such as EA/HB9AAA, HB9AAA/P, HB9AAA/3).
// The tables (dxcc_data.js, ~44 KB) are loaded on demand: LqDxcc.load(data) once, then LqDxcc.resolve(call).
(function (root) {
  'use strict';
  var prefix = null, exact = null, maxLen = 0;
  var UNKNOWN = { name: '', prefix: '', dxcc: 0 };
  var WS = /[ \t\n\x0B\f\r]+/g;
  var has = Object.prototype.hasOwnProperty;

  function load(data) {
    if (prefix) return;
    var p = {}, x = {};
    for (var i = 0; i < data.rows.length; i++) {
      var f = data.rows[i].split('|');
      var info = { name: f[0], prefix: f[1], dxcc: +f[2] };
      if (f[3]) f[3].split(' ').forEach(function (k) { p[k] = info; });
      if (f[4]) f[4].split(' ').forEach(function (k) { x[k] = info; });
    }
    maxLen = data.max;
    exact = x;
    prefix = p;
  }
  function get(m, k) { return has.call(m, k) ? m[k] : null; }
  function isDigit(c) { return c >= '0' && c <= '9'; }
  function isSingleDigit(t) { return t.length === 1 && isDigit(t); }
  function isModifierOnly(t) {
    return t === 'P' || t === 'M' || t === 'MM' || t === 'AM' || t === 'QRP' || t === 'QRPP' || t === 'A' ||
      t === 'R' || t === 'B' || t === 'LH' || isSingleDigit(t);
  }
  // a whole callsign (letters after a digit that is not its first character), as opposed to a bare prefix
  function isFullCallsignShape(t) {
    var last = -1;
    for (var i = 0; i < t.length; i++) if (isDigit(t.charAt(i))) last = i;
    return last >= 1 && last < t.length - 1;
  }
  function locationRank(t) { return has.call(prefix, t) ? 0 : (isFullCallsignShape(t) ? 2 : 1); }

  function single(call) {
    if (!call) return UNKNOWN;
    var e = get(exact, call);
    if (e) return e;
    for (var len = Math.min(call.length, maxLen); len >= 1; len--) {
      // Guantanamo Bay is KG4 with a two-letter suffix; KG4A and KG4ABC are ordinary US calls
      if (len === 3 && call.length !== 3 && call.length !== 5 && call.lastIndexOf('KG4', 0) === 0) continue;
      var info = get(prefix, call.substring(0, len));
      if (info) return info;
    }
    return UNKNOWN;
  }

  function compound(call) {
    var tokens = [], i, t;
    var raw = call.split('/');
    for (i = 0; i < raw.length; i++) {
      t = raw[i];
      if (!t) continue;
      if (t === 'MM' || t === 'AM') return UNKNOWN;   // maritime / aeronautical mobile
      tokens.push(t);
    }
    // drop the modifiers behind the call (never the first part); a call listed with its digit keeps its entity
    var withDigits = [];
    for (i = 0; i < tokens.length; i++) {
      t = tokens[i];
      if (i > 0 && isModifierOnly(t) && !isSingleDigit(t)) continue;
      withDigits.push(t);
    }
    if (withDigits.length < tokens.length && withDigits.length > 1) {
      var ex = get(exact, withDigits.join('/'));
      if (ex) return ex;
    }
    var parts = [];
    for (i = 0; i < withDigits.length; i++) {
      t = withDigits[i];
      if (i > 0 && isSingleDigit(t)) continue;
      parts.push(t);
    }
    if (!parts.length) return UNKNOWN;
    if (parts.length === 1) return single(parts[0]);
    // the part that is a location prefix decides: a listed prefix, then a bare prefix shape, then the shorter call
    var best = null, bestInfo = UNKNOWN;
    for (i = 0; i < parts.length; i++) {
      t = parts[i];
      var info = single(t);
      if (info === UNKNOWN) continue;
      if (best === null || locationRank(t) < locationRank(best) ||
          (locationRank(t) === locationRank(best) && t.length < best.length)) {
        best = t;
        bestInfo = info;
      }
    }
    return bestInfo;
  }

  // {name, prefix, dxcc}; name '' when the call has no entity (or the tables are not loaded yet)
  function resolve(callsign) {
    if (!callsign || !prefix) return UNKNOWN;
    var call = String(callsign).toUpperCase().replace(WS, '');
    if (!call) return UNKNOWN;
    var e = get(exact, call);
    if (e) return e;
    var d = get(prefix, call);
    if (d) return d;
    return call.indexOf('/') < 0 ? single(call) : compound(call);
  }

  var api = { load: load, resolve: resolve, loaded: function () { return !!prefix; } };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.LqDxcc = api;
})(typeof window !== 'undefined' ? window : this);
