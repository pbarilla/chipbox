/* The game boxes on the home page: one per sound chip, each in the box-art style of its own machine, plus the three brothers.
   Our own drawings, no logos. Input: the hidden table #chipdata (name, channels, docs). Output: #chips and #shelf. */
(function () {
  var W = 200, H = 270;
  var DID = "Didot,'Bodoni 72','Bodoni MT','Playfair Display',Georgia,serif";
  var SANS = "'Arial Black',Impact,'Helvetica Neue',sans-serif", SERIF = "Georgia,'Times New Roman',serif", PX = "PS2P,monospace", MONO = "'Courier New',monospace";
  var rect = function (x, y, w, h, f, o) { return '<rect x="' + x + '" y="' + y + '" width="' + w + '" height="' + h + '" fill="' + f + '" ' + (o || '') + '/>'; };
  var circ = function (x, y, r, f, o) { return '<circle cx="' + x + '" cy="' + y + '" r="' + r + '" fill="' + f + '" ' + (o || '') + '/>'; };
  var poly = function (p, f, o) { return '<polygon points="' + p + '" fill="' + f + '" ' + (o || '') + '/>'; };
  var path = function (d, f, s, sw, o) { return '<path d="' + d + '" fill="' + (f || 'none') + '"' + (s ? ' stroke="' + s + '" stroke-width="' + (sw || 1) + '"' : '') + ' ' + (o || '') + '/>'; };
  var line = function (x1, y1, x2, y2, s, sw, o) { return '<line x1="' + x1 + '" y1="' + y1 + '" x2="' + x2 + '" y2="' + y2 + '" stroke="' + s + '" stroke-width="' + (sw || 1) + '" ' + (o || '') + '/>'; };
  var txt = function (t, x, y, sz, f, fam, o) {
    o = o || {};
    return '<text x="' + x + '" y="' + y + '" text-anchor="' + (o.a || 'middle') + '" font-family="' + (fam || SANS) + '" font-size="' + sz + '" fill="' + f + '"' +
      (o.w ? ' font-weight="' + o.w + '"' : '') + (o.i ? ' font-style="italic"' : '') + (o.ls ? ' letter-spacing="' + o.ls + '"' : '') +
      (o.tl ? ' textLength="' + o.tl + '" lengthAdjust="spacingAndGlyphs"' : '') + (o.s ? ' stroke="' + o.s + '" stroke-width="' + (o.sw || 1) + '"' : '') +
      (o.tr ? ' transform="' + o.tr + '"' : '') + '>' + t + '</text>';
  };
  var band = function (y, h, f, t, c) { return rect(0, y, W, h, f) + txt(t, 100, y + h * 0.66, 7, c || '#fff', PX, { ls: 1 }); };
  var seedr = function (s) { return function () { s = (s * 1103515245 + 12345) & 0x7fffffff; return s / 0x7fffffff; }; };
  var stars = function (n, seed, col, y0, y1) {
    var r = seedr(seed), s = '';
    for (var i = 0; i < n; i++) { var x = r() * W, y = (y0 || 0) + r() * ((y1 || H) - (y0 || 0)); s += rect(x.toFixed(0), y.toFixed(0), 1 + (i % 3 === 0), 1 + (i % 3 === 0), col, 'opacity="' + (0.5 + (i % 4) * 0.15) + '"'); }
    return s;
  };
  var grid = function (step, col, w, h) {
    var s = '', i; w = w || W; h = h || H;
    for (i = 0; i <= w; i += step) s += line(i, 0, i, h, col, 1);
    for (i = 0; i <= h; i += step) s += line(0, i, w, i, col, 1);
    return s;
  };
  var pins = function (x, y, n, gap, len, col, vertical) {
    var s = '', i;
    for (i = 0; i < n; i++) s += vertical ? rect(x, y + i * gap, len, 3, col) : rect(x + i * gap, y, 3, len, col);
    return s;
  };

  /* ---- the art. Each returns [spine, side colour, top colour, inner svg] */
  var ART = {
    nes: function () { return ['NES', '#33333a', '#4a4a54',
      rect(0, 0, W, H, '#0d0d14') + stars(70, 7, '#fff') +
      rect(14, 14, 86, 34, '#ecece8', 'rx="17" stroke="#d8262c" stroke-width="4"') + rect(19, 19, 76, 24, 'none', 'rx="12" stroke="#b8b8b2" stroke-width="1.5"') + txt('NES', 57, 38, 17, '#c8202a', SANS, { w: 900, i: 1 }) +
      txt('SOUND', 112, 30, 12, '#f08a24', SANS, { i: 1, w: 900, a: 'start' }) + txt('SYSTEM', 112, 44, 12, '#f08a24', SANS, { i: 1, w: 900, a: 'start' }) +
      txt('PULSE · TRIANGLE', 100, 86, 8, '#fff', PX, { ls: 1 }) + txt('NOISE · DPCM', 100, 102, 8, '#fff', PX, { ls: 1 }) +
      rect(22, 130, 156, 62, '#c9c9c2', 'rx="3"') + rect(22, 130, 156, 14, '#b2b2aa') + rect(70, 148, 60, 34, '#16161a') + rect(146, 146, 22, 30, '#2a2a30') +
      rect(30, 204, 46, 18, '#c9c9c2', 'rx="3"') + rect(34, 208, 38, 10, '#222') + rect(124, 204, 46, 18, '#c9c9c2', 'rx="3"') + rect(128, 208, 38, 10, '#222') +
      band(246, 24, '#c33', '8-BIT · 1983')]; },
    gb: function () { return ['GAME BOY', '#6a1c6e', '#8a3a8e',
      '<defs><linearGradient id="gbg" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#3a1c8c"/><stop offset=".5" stop-color="#b3206a"/><stop offset="1" stop-color="#e83a2c"/></linearGradient><linearGradient id="gbs" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#dfe1e8"/><stop offset=".45" stop-color="#b4b8c4"/><stop offset="1" stop-color="#cdd0d8"/></linearGradient></defs>' +
      rect(0, 0, W, H, 'url(#gbg)') + (function () { var s = '', i; for (i = 0; i < 15; i++) s += line(140, 250, 62 + i * 14, 0, '#ffb320', 1.2, 'opacity=".8"'); for (i = 0; i < 9; i++) s += line(58, 30 + i * 26, 200, 14 + i * 26 - i * i * 0.4, '#ff7a1a', 1.2, 'opacity=".7"'); return s; })() +
      rect(74, 64, 72, 12, '#ffe9a0', 'rx="6" transform="rotate(35 110 70)" opacity=".95"') + rect(112, 170, 70, 11, '#ffe9a0', 'rx="5" transform="rotate(35 148 176)" opacity=".9"') +
      path('M82 160h16v-34h16v34h16v-34h16v34h16', 0, '#fff', 5, 'opacity=".95"') +
      txt('PULSE · WAVE', 130, 222, 8, '#ffe34d', PX) + txt('NOISE', 130, 238, 8, '#ffe34d', PX) +
      rect(0, 0, 58, H, 'url(#gbs)') + (function () { var s = '', i; for (i = 0; i < 90; i++) s += line(0, i * 3 + 1, 58, i * 3 + 1, i % 2 ? '#fff' : '#7a7e8c', 0.7, 'opacity=".28"'); return s; })() + rect(57, 0, 1.5, H, '#6a6e7c', 'opacity=".5"') +
      '<text transform="translate(47 258) rotate(-90)" font-family="' + SANS + '" font-weight="900" font-size="56" fill="#2d3a98" textLength="226" lengthAdjust="spacingAndGlyphs">GAME BOY</text>' +
      rect(5, 8, 48, 20, '#fff', 'rx="10" stroke="#d8262c" stroke-width="2.5"') + txt('FM-1', 29, 22, 9, '#d8262c', SANS, { w: 900 }) + txt('®', 50, 262, 7, '#2d3a98', SANS)]; },
    sms: function () { return ['MASTER SYSTEM', '#8a8980', '#a2a199',
      rect(0, 0, W, H, '#dedcd4') + grid(14, '#b4b3ab') +
      txt('The', 100, 40, 16, '#111', DID, { i: 1, w: 400 }) +
      txt('Master', 100, 84, 40, '#111', DID, { w: 400, ls: -1 }) + txt('System.', 100, 124, 40, '#111', DID, { w: 400, ls: -1 }) +
      line(14, 138, 186, 138, '#2a2f7a', 0.8) + txt('INTERACTIVE  SOUND  SYSTEM', 100, 150, 5, '#2a2f7a', PX, { ls: 2 }) +
      [[140, 164], [154, 178], [126, 178], [140, 192], [168, 192], [112, 192], [154, 206], [126, 206], [140, 220], [168, 220], [182, 206]].map(function (p) { return rect(p[0], p[1], 14, 14, '#1d2468'); }).join('') +
      rect(18, 176, 70, 30, '#25252c') + rect(24, 182, 58, 8, '#b0302a') + txt('Tone &amp; noise. No limits.', 20, 244, 10, '#111', DID, { i: 1, a: 'start', w: 400 })]; },
    pce: function () { return ['PC ENGINE', '#8a1c22', '#a8323a',
      rect(0, 0, W, H, '#f4f3ee') + rect(0, 0, W, 6, '#d8262c') +
      txt('PC Engine', 100, 62, 32, '#d8262c', SANS, { i: 1, w: 900, tl: 176 }) + path('M12 74h176', 0, '#1a3d9e', 3) + path('M12 80h120', 0, '#1a3d9e', 1.5) +
      poly('36,128 140,106 186,128 82,152', '#f6f6f2', 'stroke="#b8b8b2" stroke-width="1.5"') + poly('36,128 82,152 82,184 36,160', '#dcdcd6', 'stroke="#b8b8b2" stroke-width="1.5"') + poly('82,152 186,128 186,160 82,184', '#e8e8e2', 'stroke="#b8b8b2" stroke-width="1.5"') +
      poly('96,120 130,113 150,123 116,131', '#2a2a30') + poly('99,124 129,118 133,120 103,127', '#1a8a3a') +
      txt('PC Engine', 134, 160, 9, '#d8262c', SANS, { i: 1, w: 900, tr: 'rotate(-14 134 160)' }) + poly('40,146 46,149 46,160 40,157', '#22a04a') +
      rect(40, 196, 92, 38, '#e6e6e0', 'rx="12" stroke="#aaa" stroke-width="1.5"') + rect(48, 202, 76, 26, '#5a5a62', 'rx="6"') + circ(66, 215, 8, '#2a2a30') + path('M60 215h12M66 209v12', 0, '#8a8a92', 2) +
      circ(98, 215, 7, '#d8262c', 'stroke="#fff" stroke-width="1.5"') + circ(114, 215, 7, '#d8262c', 'stroke="#fff" stroke-width="1.5"') +
      rect(146, 204, 34, 22, '#2b2b32', 'rx="3"') + rect(150, 208, 26, 8, '#c9c9c2') + txt('HuCARD', 163, 223, 5, '#fff', PX) +
      band(248, 22, '#1a3d9e', 'WAVE · 6 CHANNELS')]; },
    a26: function () { return ['ATARI 2600', '#3a2a1c', '#5a4028',
      rect(0, 0, W, H, '#1b1410') + rect(0, 0, W, 62, '#7a4a24') + (function () { var s = '', i; for (i = 0; i < 8; i++) s += path('M0 ' + (8 + i * 7) + 'q50-6 100 0t100 0', 0, '#5a3418', 2); return s; })() +
      ['#f08a24', '#e0502a', '#c42a6a', '#6a3aa8'].map(function (c, i) { return rect(0, 88 + i * 14, W, 10, c); }).join('') +
      txt('2600', 100, 170, 54, '#fff', SANS, { w: 900, ls: -2 }) + txt('TWO VOICES. TIA.', 100, 206, 8, '#f08a24', PX, { ls: 1 }) + txt('VIDEO COMPUTER SOUND', 100, 250, 7, '#fff', PX)]; },
    vrc6: function () { return ['VRC6', '#2a1038', '#44205a',
      rect(0, 0, W, H, '#0e0714') + stars(40, 11, '#cdb6ff') + circ(140, 70, 40, '#f6efc8') + circ(128, 62, 40, '#0e0714', 'opacity=".25"') +
      path('M40 92q14-26 30-8q8-14 24 0q-4-24 -28 -22q-18 -2 -26 30z', '#000') + path('M60 78l10-14l8 12z', '#000') +
      rect(0, 168, W, 80, '#1a0c26') + poly('0,168 14,150 28,168 28,140 40,126 52,140 52,168 80,168 80,148 92,134 104,148 104,168 140,168 140,144 154,128 168,144 168,168 200,168 200,248 0,248', '#241236') +
      rect(60, 196, 8, 12, '#ffcc33') + rect(100, 190, 8, 12, '#ffcc33') + rect(140, 198, 8, 12, '#ffcc33') +
      txt('VRC6', 100, 130, 46, '#c3202a', SERIF, { w: 700, i: 1, s: '#000', sw: 2 }) + txt('PULSE · PULSE · SAW', 100, 232, 7, '#cdb6ff', PX) +
      band(250, 20, '#c3202a', 'FAMICOM · 1989')]; },
    ay: function () { return ['AY-3-8910', '#2a4a7a', '#4a6a9a',
      rect(0, 0, W, H, '#e8e0c8') + rect(0, 0, 18, H, '#2a4a9a') + rect(18, 0, 4, H, '#c8202a') +
      txt('DATA MANUAL', 112, 30, 9, '#2a2a30', MONO, { w: 700, ls: 2 }) + line(36, 38, 188, 38, '#2a2a30', 2) +
      rect(60, 70, 100, 150, '#16161a', 'rx="4"') + circ(110, 76, 7, '#e8e0c8') + pins(60, 78, 20, 7, 12, '#c9c9c2', 1).replace(/x="60"/g, 'x="48"') + pins(160, 78, 20, 7, 12, '#c9c9c2', 1).replace(/x="160"/g, 'x="160"') +
      txt('AY-3', 110, 132, 18, '#c9c9c2', MONO, { w: 700 }) + txt('8910', 110, 160, 22, '#e8c04a', MONO, { w: 700 }) + txt('PSG', 110, 190, 11, '#c9c9c2', MONO) +
      txt('3 TONES + NOISE', 112, 244, 8, '#2a2a30', MONO, { w: 700 }) + txt('PROGRAMMABLE SOUND GENERATOR', 112, 258, 6, '#5a5a60', MONO)]; },
    md: function () { return ['MEGA DRIVE', '#555', '#777',
      rect(0, 0, W, H, '#fafaf6') + poly('0,0 90,0 0,90', '#1f9a4a') + poly('120,0 200,0 200,100', '#2a3a9a') + poly('40,60 110,130 20,150', '#f2c500') + poly('150,110 200,170 150,200', '#d8262c') + poly('0,170 60,230 0,270', '#2a3a9a') +
      '<g transform="skewX(-10)">' + txt('MEGA', 134, 196, 34, '#111', SANS, { w: 900, tl: 120 }) + txt('DRIVE', 140, 232, 34, '#111', SANS, { w: 900, tl: 124 }) + '</g>' +
      path('M30 108q30-26 60 0t60 0', 0, '#111', 5) + path('M30 126q30-26 60 0t60 0', 0, '#d8262c', 5) + band(248, 22, '#111', 'FM · 6 VOICES')]; },
    vb: function () { return ['VIRTUAL BOY', '#7a0c12', '#a01418',
      rect(0, 0, W, H, '#140004') + (function () { var s = '', i; for (i = 0; i < 14; i++) s += line(0, 130 + i * i * 0.9 + i * 3, W, 130 + i * i * 0.9 + i * 3, '#e0102a', 1, 'opacity=".7"'); for (i = 0; i < 13; i++) s += line(100, 130, -120 + i * 35, 270, '#e0102a', 1, 'opacity=".7"'); return s; })() +
      rect(62, 36, 76, 76, 'none', 'stroke="#ff2a44" stroke-width="3"') + rect(78, 52, 76, 76, 'none', 'stroke="#ff2a44" stroke-width="3" opacity=".6"') + line(62, 36, 78, 52, '#ff2a44', 3) + line(138, 36, 154, 52, '#ff2a44', 3) +
      txt('VIRTUAL', 100, 176, 22, 'none', SANS, { w: 900, s: '#ff2a44', sw: 2 }) + txt('BOY', 100, 204, 22, 'none', SANS, { w: 900, s: '#ff2a44', sw: 2 }) +
      rect(0, 0, W, 16, '#c4161c') + txt('5 WAVES · 1 NOISE', 100, 250, 7, '#ff2a44', PX)]; },
    a8: function () { return ['ATARI 8-BIT', '#6a5a3a', '#8a7a5a',
      rect(0, 0, W, H, '#ebe4d2') + grid(20, '#d8d0bb') +
      '<g transform="rotate(-12 100 190)">' + ['#c8202a', '#e86a1c', '#f2c500', '#7a5230'].map(function (c, i) { return rect(-20, 150 + i * 16, 240, 12, c); }).join('') + '</g>' +
      txt('8-BIT', 100, 90, 52, '#1b1b1b', SANS, { w: 900, ls: -2 }) + txt('HOME COMPUTER', 100, 112, 9, '#1b1b1b', PX) +
      rect(36, 214, 128, 20, '#3a3a40', 'rx="3"') + [0, 1, 2, 3, 4, 5, 6, 7, 8, 9].map(function (i) { return rect(42 + i * 12, 219, 9, 4, '#8a8a90') + rect(42 + i * 12, 226, 9, 4, '#8a8a90'); }).join('') +
      txt('POKEY · 4 VOICES', 100, 256, 7, '#1b1b1b', PX)]; },
    pcspk: function () { return ['PC SPEAKER', '#5a5a52', '#7a7a72',
      rect(0, 0, W, H, '#d8d3c3') + rect(0, 0, W, 40, '#1a4bb0') + txt('PERSONAL COMPUTER', 100, 25, 8, '#fff', PX, { ls: 1 }) +
      circ(100, 120, 62, '#2a2a30') + circ(100, 120, 50, '#3c3c44') + circ(100, 120, 36, '#2a2a30') + circ(100, 120, 22, '#4a4a52') + circ(100, 120, 8, '#111') +
      path('M10 120h14', 0, '#1a4bb0', 4) + path('M176 120h14', 0, '#1a4bb0', 4) +
      txt('BEEP', 100, 224, 24, '#1a1a1a', PX) + txt('ONE VOICE · 1 BIT', 100, 250, 7, '#1a4bb0', PX)]; },
    adlib: function () { return ['ADLIB', '#2a0e5c', '#4a2a8c',
      '<defs><linearGradient id="adg" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#2a0e5c"/><stop offset="1" stop-color="#0f7a8a"/></linearGradient></defs>' + rect(0, 0, W, H, 'url(#adg)') +
      path('M0 80q25-40 50 0t50 0t50 0t50 0', 0, '#7af0ff', 3, 'opacity=".9"') + path('M0 100q25-30 50 0t50 0t50 0t50 0', 0, '#ff7ad8', 3, 'opacity=".8"') + path('M0 120q25-20 50 0t50 0t50 0t50 0', 0, '#ffe34d', 3, 'opacity=".7"') +
      txt('AdLib', 100, 190, 44, '#ffe9a0', SERIF, { w: 700, i: 1, s: '#2a0e5c', sw: 2 }) + txt('MUSIC SYNTHESIZER CARD', 100, 210, 7, '#fff', PX) +
      (function () { var s = '', i; for (i = 0; i < 12; i++) s += rect(i * 16.7, 222, 15.5, 48, '#f4f0e4'); for (i = 0; i < 11; i++) if (i % 7 !== 2 && i % 7 !== 6) s += rect(i * 16.7 + 11, 222, 11, 28, '#14101c'); return s; })()]; },
    wsg: function () { return ['NAMCO WSG', '#1a1a6a', '#2a2a9a',
      rect(0, 0, W, H, '#05050c') + path('M14 40h120v40h-40v20h70v40h-150z', 0, '#2121de', 4) + path('M14 140h60v50h-60z', 0, '#2121de', 4) + path('M110 140h76v50h-76z', 0, '#2121de', 4) + path('M14 210h172', 0, '#2121de', 4) +
      path('M66 118 L87.3 103.1 A26 26 0 1 0 87.3 132.9 Z', '#ffe000') + circ(66, 106, 3, '#05050c') +
      [0, 1, 2, 3, 4].map(function (i) { return circ(112 + i * 16, 118, 3, '#ffd9a0'); }).join('') +
      [['#ff4040', 30], ['#ff9ad8', 80], ['#40e0ff', 130]].map(function (g) { return path('M' + g[1] + ' 176v-18a14 14 0 0 1 28 0v18l-5-4-5 4-4-4-5 4-4-4-5 4z', g[0]) + circ(g[1] + 9, 160, 3, '#fff') + circ(g[1] + 19, 160, 3, '#fff') + circ(g[1] + 10, 161, 1.5, '#222') + circ(g[1] + 20, 161, 1.5, '#222'); }).join('') +
      txt('WSG', 100, 238, 20, '#ffe000', SANS, { w: 900 }) + txt('WAVE SOUND GENERATOR', 100, 256, 6, '#fff', PX)]; },
    n163: function () { return ['NAMCO 163', '#8a1a1a', '#b02a2a',
      rect(0, 0, W, H, '#d22a2a') + (function () { var s = '', i; for (i = 0; i < 6; i++) s += poly((i * 40 - 30) + ',270 ' + (i * 40) + ',270 ' + (i * 40 + 90) + ',0 ' + (i * 40 + 60) + ',0', '#fff', 'opacity=".14"'); return s; })() +
      (function () { var s = '', r, c; for (r = 0; r < 2; r++) for (c = 0; c < 10; c++) s += rect(c * 20, r * 20, 20, 20, (r + c) % 2 ? '#111' : '#fff'); return s; })() +
      '<g transform="skewX(-10)">' + txt('163', 120, 170, 78, '#fff', SANS, { w: 900, s: '#111', sw: 3 }) + '</g>' + txt('8 WAVE CHANNELS', 100, 206, 8, '#fff', PX) +
      rect(0, 226, W, 44, '#111') + txt('NAMCO · FAMICOM', 100, 252, 8, '#ffd34d', PX)]; },
    s5b: function () { return ['SUNSOFT 5B', '#1a6a2a', '#2a9a3a',
      rect(0, 0, W, H, '#38b449') + stars(30, 5, '#fff').replace(/<rect/g, '<rect') + circ(40, 60, 22, '#8be07a', 'opacity=".7"') + circ(160, 120, 30, '#8be07a', 'opacity=".6"') + circ(60, 190, 18, '#8be07a', 'opacity=".6"') +
      poly('100,24 118,66 164,70 129,100 140,146 100,122 60,146 71,100 36,70 82,66', '#ffe34d', 'stroke="#111" stroke-width="4"') +
      circ(100, 92, 4, '#111') + path('M88 108q12 10 24 0', 0, '#111', 3) +
      txt('5B', 100, 214, 64, '#fff', SANS, { w: 900, s: '#111', sw: 3 }) + band(236, 34, '#111', 'SUNSOFT · 3 TONES', '#ffe34d')]; },
    mmc5: function () { return ['NES MMC5', '#14307a', '#2a4aa0',
      rect(0, 0, W, H, '#1c3fa8') + poly('0,200 200,100 200,150 0,250', '#fff', 'opacity=".9"') + poly('0,100 200,0 200,40 0,140', '#6a8af0', 'opacity=".7"') +
      '<g transform="skewX(-8)">' + txt('MMC5', 100, 100, 52, '#fff', SANS, { w: 900, s: '#0a1a50', sw: 3, tl: 170 }) + '</g>' + txt('+ 2 PULSE CHANNELS', 100, 128, 8, '#ffe34d', PX) +
      rect(66, 170, 68, 54, '#8a8a90', 'rx="3"') + rect(72, 176, 56, 30, '#c9c9c2') + rect(84, 214, 32, 8, '#222') + txt('MMC5', 100, 196, 10, '#1c3fa8', PX) +
      circ(168, 232, 20, '#d8b030', 'stroke="#fff6" stroke-width="2"') + txt('OK', 168, 236, 9, '#5a3a00', PX)]; },
    fds: function () { return ['FAMICOM DISK', '#a86a00', '#d08a10',
      rect(0, 0, W, H, '#2a2a34') + rect(0, 0, W, 8, '#d8262c') + stars(24, 3, '#fff', 8, 120) +
      '<g transform="rotate(-7 100 100)">' +
      path('M34 30H46V25H68V30H88V22H112V30H134V25H154V30H166V172H34Z', '#f2b700', '#b88400', 2) +
      rect(37, 34, 7, 9, '#d89c00') + rect(156, 34, 7, 9, '#d89c00') +
      rect(92, 34, 16, 46, '#4a4430') + rect(94, 36, 12, 42, '#5a5440') + path('M66 92q0-30 34-30t34 30v14h-68z', 'none', '#d89c00', 1.5) +
      circ(100, 98, 15, '#2a2a30') + circ(100, 98, 13, '#e8e8e0') + circ(100, 98, 5, '#2a2a30') + circ(100, 98, 2.5, '#e8e8e0') +
      rect(46, 120, 108, 30, '#4ea468', 'rx="2"') + rect(46, 120, 108, 8, '#1a1a20') + txt('SIDE A', 62, 126.5, 5, '#fff', PX) + txt('SOUND', 100, 145, 14, '#d8262c', SANS, { w: 900, i: 1 }) +
      rect(38, 156, 124, 16, '#e8a800', 'rx="3"') + txt('SOUND', 100, 168.5, 12, '#c88a00', SANS, { w: 900, ls: 4 }) + '</g>' +
      rect(0, 212, W, 58, '#14131a') + txt('FAMICOM DISK', 100, 234, 10, '#ffcf30', PX) + txt('1 WAVE · FM', 100, 252, 7, '#fff', PX)]; },
    scc: function () { return ['KONAMI SCC', '#1a1a4a', '#2a2a7a',
      rect(0, 0, W, H, '#060a1c') + stars(60, 21, '#fff') +
      poly('40,120 150,96 170,120 150,144', '#c9cfdc', 'stroke="#fff" stroke-width="2"') + poly('40,120 110,108 110,132', '#8a92a8') + line(170, 120, 200, 120, '#ff4060', 3) + line(170, 114, 200, 108, '#ff4060', 2) + line(170, 126, 200, 132, '#ff4060', 2) +
      path('M10 190l10-18 10 36 10-36 10 36 10-36 10 36 10-36 10 36 10-36 10 36 10-36', 0, '#40e0ff', 3) + path('M10 214l10-12 10 24 10-24 10 24 10-24 10 24 10-24 10 24 10-24 10 24 10-24', 0, '#ff4090', 2) +
      '<g transform="skewX(-10)">' + txt('SCC', 120, 66, 44, '#d4dae8', SANS, { w: 900, s: '#1a2a6a', sw: 2 }) + '</g>' + txt('5 WAVE CHANNELS', 100, 250, 8, '#40e0ff', PX)]; },
    msxm: function () { return ['MSX MUSIC', '#555', '#888',
      rect(0, 0, W, H, '#f7f7f2') + poly('0,270 60,270 200,60 200,0', '#17b6e8', 'opacity=".9"') + poly('60,270 120,270 200,150 200,100', '#e8329a', 'opacity=".9"') + poly('120,270 170,270 200,230 200,200', '#f6d400', 'opacity=".95"') +
      txt('MSX', 14, 84, 52, '#111', SANS, { w: 900, a: 'start', ls: -1 }) + txt('MUSIC', 14, 130, 40, '#111', SANS, { w: 900, a: 'start' }) +
      txt('FM SOUND · YM2413', 14, 158, 7, '#111', PX, { a: 'start' }) + rect(14, 14, 38, 20, '#111') + txt('M', 33, 30, 12, '#fff', SANS, { w: 900 })]; },
    vrc7: function () { return ['KONAMI VRC7', '#0a3a4a', '#1a6a7a',
      rect(0, 0, W, H, '#03070f') + stars(80, 33, '#fff') + circ(100, 120, 56, '#1f9aa8') + circ(86, 104, 56, '#03070f', 'opacity=".28"') +
      '<ellipse cx="100" cy="120" rx="96" ry="22" fill="none" stroke="#9ff0f0" stroke-width="3" transform="rotate(-20 100 120)"/>' + circ(190, 92, 4, '#fff') +
      '<g transform="skewX(-8)">' + txt('VRC7', 112, 224, 40, '#b8f6f6', SANS, { w: 900, s: '#0a3a4a', sw: 2 }) + '</g>' + txt('6 FM VOICES', 100, 252, 8, '#9ff0f0', PX)]; },
    arcfm: function () { return ['ARCADE FM', '#3a0a4a', '#6a1a8a',
      rect(0, 0, W, H, '#0a0414') + (function () { var s = '', i; for (i = 0; i < 12; i++) s += line(100, 150, -200 + i * 40, 270, '#ff2ad4', 1, 'opacity=".7"'); for (i = 0; i < 8; i++) s += line(0, 150 + i * i * 1.8 + 4, W, 150 + i * i * 1.8 + 4, '#ff2ad4', 1, 'opacity=".7"'); return s; })() +
      rect(60, 36, 80, 96, '#1a1a2a', 'rx="4" stroke="#27d3ff" stroke-width="2"') + rect(68, 44, 64, 50, '#071a2a', 'stroke="#27d3ff"') + circ(86, 112, 6, '#ff2ad4') + circ(104, 112, 6, '#ffe34d') + rect(114, 108, 12, 8, '#27d3ff') +
      path('M72 80l8-12 8 12 8-12 8 12 8-12 8 12', 0, '#27ff9a', 2) +
      txt('ARCADE', 100, 170, 26, '#ffe34d', SANS, { w: 900, s: '#ff2ad4', sw: 1.5, tl: 150 }) + txt('FM', 100, 204, 34, '#27d3ff', SANS, { w: 900, s: '#0a0414', sw: 1 }) +
      txt('INSERT COIN', 100, 240, 9, '#ff2ad4', PX) + txt('YM2151 · 8 VOICES', 100, 258, 6, '#fff', PX)]; },
    sb: function () { return ['SOUND BLASTER', '#14508a', '#2a78b8',
      rect(0, 0, W, H, '#fbfbf8') + '<defs><linearGradient id="sbg" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#0a5ac8"/><stop offset="1" stop-color="#14c8a0"/></linearGradient></defs>' +
      path('M0 200q40-60 100-20t100-40v130H0z', 'url(#sbg)') + path('M0 224q50-50 100-10t100-30v86H0z', '#0a3a8a', 0, 0, 'opacity=".7"') +
      txt('Sound', 100, 70, 38, '#0a3a8a', SANS, { w: 900, i: 1, tl: 160 }) + txt('Blaster', 100, 112, 38, '#0a3a8a', SANS, { w: 900, i: 1, tl: 170 }) +
      circ(100, 164, 20, '#0a5ac8') + circ(100, 164, 12, '#fff') + circ(100, 164, 4, '#0a5ac8') + txt('FM · YM3812', 100, 258, 8, '#fff', PX)]; },
    ws: function () { return ['WONDERSWAN', '#6a88b8', '#9ab4d8',
      '<defs><linearGradient id="wsg" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#ffffff"/><stop offset=".5" stop-color="#d8e8ff"/><stop offset="1" stop-color="#ffdce8"/></linearGradient></defs>' + rect(0, 0, W, H, 'url(#wsg)') +
      circ(160, 60, 34, '#ffb8d0', 'opacity=".5"') + circ(34, 210, 40, '#b8d8ff', 'opacity=".5"') +
      path('M22 70q-12-18 6-26t16-22q4-14 16-10q-6 8-2 14q-8 2-6 12q4 16-12 28z', '#fff', '#3a5a9a', 2) + circ(60, 26, 2, '#3a5a9a') + path('M66 24l10 1-9 4z', '#f08a24') +
      txt('WonderSwan', 138, 50, 20, '#2a3a6a', SANS, { w: 900, i: 1, tl: 108 }) + txt('4 WAVE CHANNELS', 138, 66, 6, '#2a3a6a', PX) +
      '<g transform="translate(100 156) rotate(-12)">' +
      rect(-82, -52, 164, 104, '#26262c', 'rx="16"') + rect(-80, -50, 160, 100, 'none', 'rx="15" stroke="#5a5a64" stroke-width="2"') +
      rect(-46, -42, 100, 84, '#c9ccd4', 'rx="6"') + rect(-38, -34, 84, 66, '#9aa08a', 'rx="2"') + rect(-38, -34, 84, 10, '#8a9078', 'opacity=".6"') +
      txt('WonderSwan', 12, 42, 6, '#555', SANS, { i: 1, w: 900 }) + rect(-44, 34, 14, 6, '#8a8e98', 'rx="3"') + rect(-26, 34, 14, 6, '#8a8e98', 'rx="3"') +
      [[-64, -34], [-56, -26], [-72, -26], [-64, -18]].map(function (p) { return rect(p[0] - 5, p[1] - 5, 10, 10, '#d8d8dc', 'rx="3" transform="rotate(45 ' + p[0] + ' ' + p[1] + ')"'); }).join('') +
      [[-64, 18], [-56, 26], [-72, 26], [-64, 34]].map(function (p) { return rect(p[0] - 5, p[1] - 5, 10, 10, '#d8d8dc', 'rx="3" transform="rotate(45 ' + p[0] + ' ' + p[1] + ')"'); }).join('') +
      circ(68, -30, 11, '#18181c') + path('M62 -36h12M62 -32h12M62 -28h12M62 -24h12', 0, '#3a3a44', 1.5) + circ(70, 30, 7, '#d8d8dc') + circ(58, 38, 7, '#d8d8dc') + '</g>' +
      rect(0, 236, W, 34, '#2a3a6a') + txt('BANDAI · 1999', 100, 258, 8, '#fff', PX)]; },
    sam: function () { return ['SAM COUPÉ', '#2a2a30', '#44444c',
      rect(0, 0, W, H, '#08080c') + ['#e8262a', '#f2a300', '#f5e100', '#22b34a', '#17b6e8', '#8a3ad8'].map(function (c, i) { return poly((10 + i * 12) + ',0 ' + (28 + i * 12) + ',0 ' + (130 + i * 12) + ',270 ' + (112 + i * 12) + ',270', c); }).join('') +
      txt('SAM', 134, 100, 40, '#fff', SANS, { w: 900, s: '#08080c', sw: 3, tl: 90 }) + txt('COUPÉ', 134, 140, 30, '#fff', SANS, { w: 900, s: '#08080c', sw: 3, tl: 90 }) +
      txt('6 VOICES', 134, 168, 7, '#ffe34d', PX) + rect(0, 244, W, 26, '#08080c', 'opacity=".85"') + txt('SAA1099 · STEREO', 100, 261, 7, '#fff', PX)]; },
    vic: function () { return ['VIC-20', '#6a4a2a', '#8a6a4a',
      rect(0, 0, W, H, '#f2e8d0') + ['#7a4a22', '#c8741c', '#e8a63a'].map(function (c, i) { return rect(0, i * 10, W, 10, c); }).join('') +
      txt('VIC-20', 100, 92, 40, '#5a3216', SANS, { w: 900 }) + txt('THE FRIENDLY COMPUTER', 100, 112, 7, '#5a3216', PX) +
      rect(20, 138, 160, 70, '#cdb990', 'rx="4"') + (function () { var s = '', r, c; for (r = 0; r < 4; r++) for (c = 0; c < 11; c++) s += rect(26 + c * 14, 144 + r * 15, 11, 11, r === 3 ? '#8a6a3a' : '#f2e8d0', 'rx="1.5"'); return s; })() +
      rect(0, 226, W, 44, '#7a4a22') + txt('4 VOICES · 6560', 100, 254, 8, '#ffe9a0', PX)]; },
    plus4: function () { return ['PLUS/4', '#6a6a66', '#8a8a86',
      rect(0, 0, W, H, '#dcdcd6') + rect(0, 0, W, 8, '#444') +
      rect(40, 40, 56, 56, '#d8262c', 'rx="6"') + rect(104, 40, 56, 56, '#2a4ab0', 'rx="6"') + rect(40, 104, 56, 56, '#22a04a', 'rx="6"') + rect(104, 104, 56, 56, '#f2c500', 'rx="6"') +
      txt('W', 68, 80, 26, '#fff', SANS, { w: 900 }) + txt('S', 132, 80, 26, '#fff', SANS, { w: 900 }) + txt('D', 68, 144, 26, '#fff', SANS, { w: 900 }) + txt('G', 132, 144, 26, '#111', SANS, { w: 900 }) +
      txt('plus/4', 100, 204, 34, '#1a1a1a', 'Helvetica,Arial,sans-serif', { w: 300, tl: 150 }) + txt('INTEGRATED SOFTWARE', 100, 226, 7, '#444', PX) + band(246, 24, '#444', 'TED · 2 VOICES')]; },
    lynx: function () { return ['ATARI LYNX', '#0a3a30', '#0a6a58',
      rect(0, 0, W, H, '#05080a') + (function () { var s = '', i; for (i = 0; i < 9; i++) s += line(0, i * 30, W, i * 30 - 40, '#14ffb4', 1, 'opacity=".25"'); return s; })() +
      path('M10 110q90-70 180 0q-90 70-180 0z', '#b8ff3a', '#14ffb4', 3) + '<ellipse cx="100" cy="110" rx="9" ry="40" fill="#05080a"/>' + circ(80, 96, 5, '#fff', 'opacity=".7"') +
      txt('LYNX', 100, 210, 54, '#fff', SANS, { w: 900, s: '#ff2a8a', sw: 3, tl: 170 }) + txt('HANDHELD · 4 CHANNELS', 100, 240, 7, '#14ffb4', PX)]; },
    pc98: function () { return ['PC-98', '#4a6a9a', '#7a9aca',
      '<defs><linearGradient id="p98" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#ffd0e8"/><stop offset="1" stop-color="#a0e8ff"/></linearGradient></defs>' + rect(0, 0, W, H, '#fbfbff') + grid(10, '#c8d4ee') + rect(0, 190, W, 80, 'url(#p98)') + rect(0, 0, W, 6, '#d8262c') +
      txt('PC-9801', 100, 90, 38, '#1a3a9a', SANS, { w: 900, tl: 170 }) + txt('音源', 100, 150, 46, '#d8262c', "'Hiragino Sans','Yu Gothic',sans-serif", { w: 700 }) +
      txt('FM · SSG · 6+3 VOICES', 100, 222, 8, '#1a3a9a', PX) + txt('NEC', 100, 252, 12, '#1a3a9a', SANS, { w: 900, ls: 4 })]; },
    neo: function () { return ['NEO·GEO', '#6a5010', '#a08020',
      '<defs><linearGradient id="gold" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#fff2a8"/><stop offset=".45" stop-color="#d8a820"/><stop offset=".6" stop-color="#8a6410"/><stop offset="1" stop-color="#f6d460"/></linearGradient></defs>' +
      rect(0, 0, W, H, '#0a0a0a') + rect(8, 8, 184, 254, 'none', 'stroke="url(#gold)" stroke-width="3"') + rect(14, 14, 172, 242, 'none', 'stroke="url(#gold)" stroke-width="1"') +
      txt('NEO', 100, 100, 56, 'url(#gold)', SANS, { w: 900, tl: 152 }) + txt('GEO', 100, 156, 56, 'url(#gold)', SANS, { w: 900, tl: 152 }) +
      line(34, 170, 166, 170, '#d8a820', 1.5) + rect(34, 182, 132, 24, '#c8202a') + txt('MEGA SOUND', 100, 199, 10, '#fff', PX, { ls: 1 }) +
      line(34, 218, 166, 218, '#d8a820', 1.5) + txt('FM · SSG', 100, 236, 9, '#d8a820', PX) + txt('MVS · AES', 100, 250, 6, '#fff', PX)]; },
    ngp: function () { return ['NEO GEO POCKET', '#14408a', '#2a68c0',
      rect(0, 0, W, H, '#1f5ea8') + circ(50, 60, 40, '#ff8ac0', 'opacity=".8"') + circ(150, 50, 28, '#ffe34d', 'opacity=".85"') + circ(120, 120, 50, '#7af0ff', 'opacity=".7"') + circ(40, 160, 22, '#fff', 'opacity=".5"') + circ(170, 170, 18, '#ff8ac0', 'opacity=".7"') +
      rect(0, 190, W, 80, '#fff') + txt('POCKET', 100, 226, 34, '#1f5ea8', SANS, { w: 900, tl: 170 }) + txt('TONE · NOISE · STEREO', 100, 250, 7, '#1f5ea8', PX)]; },
    msxa: function () { return ['MSX-AUDIO', '#2a2a40', '#44446a',
      rect(0, 0, W, H, '#1a1a24') + rect(14, 20, 172, 120, '#d8d0b8', 'rx="4"') +
      [0, 1, 2].map(function (i) { var x = 22 + i * 54; return rect(x, 30, 46, 50, '#f6f0dc', 'stroke="#555"') + path('M' + (x + 6) + ' 72a20 20 0 0 1 34 0', 0, '#c8202a', 1) + line(x + 23, 72, x + 12 + i * 10, 44, '#111', 2); }).join('') +
      [0, 1, 2, 3, 4, 5, 6, 7].map(function (i) { return circ(30 + i * 20, 104, 5, i < 5 ? '#27ff6a' : i < 7 ? '#ffe34d' : '#ff3a3a'); }).join('') + rect(22, 120, 156, 6, '#8a8268') +
      txt('MSX', 100, 190, 26, '#fff', SANS, { w: 900, ls: 4 }) + txt('AUDIO', 100, 222, 30, '#ffe34d', SANS, { w: 900 }) + txt('Y8950 · FM + ADPCM', 100, 250, 7, '#8af', PX)]; },
    tx81z: function () { return ['YAMAHA TX81Z', '#3a3a44', '#5a5a66',
      rect(0, 0, W, H, '#25252c') + rect(0, 0, W, 6, '#6a3aa8') +
      rect(24, 34, 152, 54, '#a8c070', 'rx="3" stroke="#111" stroke-width="3"') + txt('FM 4-OP', 100, 58, 13, '#2a3a10', MONO, { w: 700 }) + txt('VOICE 01  ALGO 5', 100, 78, 9, '#2a3a10', MONO) +
      [0, 1, 2, 3, 4, 5, 6, 7].map(function (i) { return rect(24 + i * 19.5, 100, 15, 14, '#3a3a44', 'rx="2"'); }).join('') + circ(160, 150, 14, '#8a8a94') + line(160, 150, 168, 138, '#111', 3) + rect(24, 140, 80, 6, '#6a3aa8') +
      '<g transform="skewX(-8)">' + txt('TX81Z', 106, 214, 42, '#d4d6e0', SANS, { w: 900, s: '#111', sw: 2, tl: 170 }) + '</g>' + txt('4-OPERATOR FM', 100, 244, 8, '#c8a8ff', PX)]; },
    sn77: function () { return ['SN76477', '#5a0a0a', '#8a1a1a',
      rect(0, 0, W, H, '#080408') + (function () { var s = '', i; for (i = 0; i < 18; i++) s += poly('100,190 ' + (-60 + i * 18) + ',0 ' + (-48 + i * 18) + ',0', i % 2 ? '#d8262c' : '#f08a24', 'opacity=".55"'); return s; })() + circ(100, 190, 28, '#ffe34d') +
      (function () { var P = ['00100000100', '00010001000', '00111111100', '01101110110', '11111111111', '10111111101', '10100000101', '00011011000'], s = '', r, c; for (r = 0; r < 8; r++) for (c = 0; c < 11; c++) if (P[r][c] === '1') s += rect(34 + c * 12, 40 + r * 10, 11, 9, '#3aff6a'); return s; })() +
      txt('SN76477', 100, 244, 26, '#fff', SANS, { w: 900, tl: 170 }) + txt('COMPLEX SOUND GENERATOR', 100, 260, 6, '#ffe34d', PX)]; },
    sys1: function () { return ['NAMCO SYSTEM 1', '#0a4a22', '#1a7a3a',
      rect(0, 0, W, H, '#0f5c2e') + (function () { var r = seedr(5), s = '', i; for (i = 0; i < 26; i++) { var x = r() * W, y = r() * 200; s += path('M' + x.toFixed(0) + ' ' + y.toFixed(0) + 'h' + (10 + r() * 40).toFixed(0) + 'v' + (10 + r() * 30).toFixed(0), 0, '#2a8a4a', 2); s += circ(x.toFixed(0), y.toFixed(0), 2.5, '#c8a040'); } return s; })() +
      rect(20, 40, 70, 40, '#111', 'rx="2"') + pins(24, 36, 8, 8, 5, '#c9c9c2') + pins(24, 80, 8, 8, 5, '#c9c9c2') + rect(110, 40, 60, 60, '#111', 'rx="2"') + pins(114, 36, 7, 8, 5, '#c9c9c2') + pins(114, 100, 7, 8, 5, '#c9c9c2') + txt('C30', 55, 66, 10, '#c9c9c2', MONO, { w: 700 }) + txt('WSG', 140, 74, 10, '#c9c9c2', MONO, { w: 700 }) +
      rect(20, 110, 150, 6, '#c8a040', 'opacity=".0"') + txt('SYSTEM 1', 100, 156, 20, '#fff', PX, { s: '#0a3a1a', sw: 1 }) + txt('NAMCO · ARCADE BOARD', 100, 178, 7, '#c8f0c8', PX) +
      (function () { var s = '', i; for (i = 0; i < 20; i++) s += rect(4 + i * 9.6, 240, 7, 30, '#d8b040'); return s; })() + txt('8 WAVE CHANNELS', 100, 226, 8, '#fff', PX)]; },
    poly: function () { return ['KORG POLY-800', '#1a3a5a', '#2a5a8a',
      rect(0, 0, W, H, '#2f4a6b') + poly('0,0 60,0 0,60', '#f6d400') + circ(170, 40, 22, '#ff5aa0') + path('M20 120q16-24 32 0t32 0t32 0t32 0', 0, '#27d3ff', 4) + poly('150,100 190,100 170,140', '#8a3ad8') +
      '<g transform="skewX(-8)">' + txt('POLY', 100, 196, 38, '#fff', SANS, { w: 900, s: '#0a1a30', sw: 2, tl: 140 }) + txt('-800', 100, 230, 28, '#ffe34d', SANS, { w: 900, tl: 100 }) + '</g>' +
      (function () { var s = '', i; for (i = 0; i < 12; i++) s += rect(i * 16.7, 240, 15.5, 30, '#f4f0e4'); for (i = 0; i < 11; i++) if (i % 7 !== 2 && i % 7 !== 6) s += rect(i * 16.7 + 11, 240, 11, 18, '#111'); return s; })() + txt('8 VOICES · ORGAN', 100, 160, 8, '#fff', PX)]; },
    zx: function () { return ['ZX SPECTRUM', '#222', '#3a3a3a',
      rect(0, 0, W, H, '#0a0a0a') + ['#e8262a', '#f5d300', '#1fb24a', '#16b4d8'].map(function (c, i) { return poly((200 - i * 22 - 60) + ',270 ' + (200 - i * 22 - 38) + ',270 ' + (200 - i * 22 + 40) + ',0 ' + (200 - i * 22 + 18) + ',0', c); }).join('') +
      txt('ZX', 14, 190, 30, '#fff', SANS, { w: 900, a: 'start', tl: 150 }) + txt('SPECTRUM', 14, 224, 26, '#fff', SANS, { w: 900, a: 'start', tl: 150 }) + txt('ONE BEEPER', 14, 246, 7, '#f5d300', PX, { a: 'start', ls: 1 })]; },
    pc88: function () { return ['NEC PC-88', '#6a5a3a', '#8a7a5a',
      rect(0, 0, W, H, '#e8dec6') + [['#f08a24', 0], ['#17b6e8', 14], ['#f08a24', 28]].map(function (c) { return poly('0,' + (150 + c[1]) + ' 200,' + (90 + c[1]) + ' 200,' + (104 + c[1]) + ' 0,' + (164 + c[1]), c[0]); }).join('') +
      txt('PC-8801', 100, 80, 38, '#4a3216', SANS, { w: 900, tl: 170 }) + txt('mkII SR', 100, 108, 16, '#4a3216', SERIF, { i: 1, w: 700 }) +
      rect(54, 196, 92, 54, '#3a3a40', 'rx="4"') + rect(62, 202, 76, 34, '#2a5a3a') + path('M66 224l8-14 8 14 8-14 8 14 8-14 8 14 8-14', 0, '#8aff9a', 2) + txt('FM · SSG · 3+3 VOICES', 100, 262, 6, '#4a3216', PX)]; },
    tetris: function () { return ['TUTORIAL 1', '#14307a', '#2a4aa0',
      '<defs><linearGradient id="ttg" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#0c1a5a"/><stop offset="1" stop-color="#2a4ac0"/></linearGradient></defs>' + rect(0, 0, W, H, 'url(#ttg)') + stars(40, 17, '#fff', 0, 150) +
      rect(46, 34, 108, 150, '#050a24', 'stroke="#9fb4ff" stroke-width="3"') +
      [['#00d8e8', 0, 6, 4, 1], ['#f6d400', 3, 5, 2, 2], ['#a020d8', 5, 4, 3, 1], ['#22c04a', 1, 4, 3, 1], ['#e8262a', 4, 3, 3, 1], ['#f08a24', 0, 3, 3, 2], ['#2a4ae8', 6, 2, 3, 2]].map(function (b) { return rect(52 + b[1] * 14, 40 + b[2] * 18, b[3] * 14 - 2, b[4] * 18 - 2, b[0]); }).join('') +
      rect(66, 40, 12, 16, '#f6d400') + rect(80, 40, 12, 16, '#f6d400') + rect(66, 58, 12, 16, '#f6d400') + rect(80, 58, 12, 16, '#f6d400') +
      txt('TETRIS', 100, 214, 28, '#fff', SANS, { w: 900, s: '#0a1a50', sw: 3, tl: 168 }) + txt('KOROBEINIKI · 4 CHANNELS', 100, 236, 6, '#ffe34d', PX) + band(250, 20, '#e8262a', 'START HERE')]; },
    amen: function () { return ['TUTORIAL 2', '#6a2a08', '#a04a14',
      rect(0, 0, W, H, '#f08a24') + (function () { var s = '', i; for (i = 0; i < 12; i++) s += rect(0, i * 24, W, 12, '#e07814', 'opacity=".5"'); return s; })() +
      circ(100, 104, 78, '#16161a') + (function () { var s = '', i; for (i = 1; i < 6; i++) s += circ(100, 104, 78 - i * 9, 'none', 'stroke="#2e2e36" stroke-width="1.5"'); return s; })() + circ(100, 104, 26, '#d8262c') + circ(100, 104, 3, '#f4f0e4') + txt('AMEN', 100, 100, 9, '#fff', SANS, { w: 900 }) + path('M40 60A68 68 0 0 1 100 36', 0, '#fff', 3, 'opacity=".25" stroke-linecap="round"') +
      txt('AMEN BREAK', 100, 214, 26, '#16161a', SANS, { w: 900, tl: 170 }) + txt('4 BARS · 137 BPM · DRUM LANES', 100, 236, 6, '#16161a', PX) + band(250, 20, '#16161a', 'THE FAMOUS BREAK', '#f08a24')]; },
    c64: function () { return ['COMMODORE 64', '#9a9a92', '#b8b8b0',
      rect(0, 0, W, H, '#f4f2ea') + rect(0, 0, W, 12, '#c8202a') + rect(0, 12, W, 12, '#1f4fa8') +
      txt('COMMODORE', 14, 70, 32, '#111', SANS, { w: 900, a: 'start', tl: 172 }) + txt('64', 14, 112, 46, '#111', SANS, { w: 900, a: 'start' }) + txt('Personal Computer', 14, 132, 14, '#111', SERIF, { w: 700, a: 'start' }) +
      circ(156, 186, 26, '#fff', 'stroke="#1f4fa8" stroke-width="4"') + circ(156, 186, 19, 'none', 'stroke="#c8202a" stroke-width="3"') + txt('SID', 156, 191, 12, '#1f4fa8', SANS, { w: 900 }) +
      rect(16, 150, 86, 34, '#6b5a44', 'rx="3"') + rect(20, 168, 78, 12, '#bfae92') + rect(0, 236, 14, 34, '#c8202a') + rect(14, 236, 14, 34, '#1f4fa8') + txt('Voice, filter &amp; more.', 40, 258, 14, '#111', SERIF, { i: 1, a: 'start' })]; }
  };
  var ID = { 'NES': 'nes', 'Game Boy': 'gb', 'Master System': 'sms', 'PC Engine': 'pce', 'Atari 2600': 'a26', 'Konami VRC6': 'vrc6', 'AY-3-8910': 'ay', 'Mega Drive': 'md', 'Virtual Boy': 'vb', 'Atari 8-bit': 'a8',
    'PC Speaker': 'pcspk', 'AdLib': 'adlib', 'Namco WSG': 'wsg', 'Namco 163': 'n163', 'Sunsoft 5B': 's5b', 'NES MMC5': 'mmc5', 'Famicom Disk': 'fds', 'Konami SCC': 'scc', 'MSX Music': 'msxm', 'Konami VRC7': 'vrc7',
    'Arcade FM': 'arcfm', 'Sound Blaster': 'sb', 'WonderSwan': 'ws', 'Sam Coupe': 'sam', 'VIC-20': 'vic', 'Plus/4': 'plus4', 'Atari Lynx': 'lynx', 'PC-98': 'pc98', 'Neo Geo': 'neo', 'Neo Geo Pocket': 'ngp',
    'MSX-Audio': 'msxa', 'Yamaha TX81Z': 'tx81z', 'SN76477': 'sn77', 'Namco System 1': 'sys1', 'Korg Poly-800': 'poly', 'ZX Spectrum': 'zx', 'NEC PC-88': 'pc88', 'Commodore 64': 'c64' };

  /* ---- wear: grain, halftone, creases, scuffed edges, gloss */
  var PROF = { nes: 'yellow crease', gb: 'fade tape', sms: 'yellow stain', pce: 'sticker scratch', a26: 'stain crush', vrc6: 'dent scratch', ay: 'yellow fold', md: 'fade fold', vb: 'crush scratch', a8: 'yellow tape',
    pcspk: 'yellow stain', adlib: 'fade', wsg: 'sticker scratch', n163: 'fade tape', s5b: 'dent', mmc5: 'sticker', fds: 'yellow stain', scc: 'scratch fade', msxm: 'fade fold', vrc7: 'dent', arcfm: 'scratch', sb: 'stain sticker',
    ws: 'yellow', sam: 'crush', vic: 'yellow fold stain', plus4: 'tape scratch', lynx: 'scratch', pc98: 'fade', neo: 'scratch dent', ngp: 'sticker', msxa: 'yellow', tx81z: 'scratch fold', sn77: 'fade stain', sys1: 'dent',
    poly: 'fade tape', zx: 'crush scratch', pc88: 'yellow stain', c64: 'fade sticker', brochipbox: 'fold scratch', brosmileyface: 'sticker', brodrumlab: 'stain' };
  var PRICE = ['$4.99', '¥1,980', '$9.99', '£3.50', 'SALE', '$2.00', '¥980'];
  function wear(id, w, h) {
    var r = seedr(id.length * 977 + id.charCodeAt(0) * 31 + id.charCodeAt(id.length - 1) * 7 + id.charCodeAt(id.length >> 1) * 13), cr = '', sc = '', i, fx = '', P = ' ' + (PROF[id] || 'crease') + ' ';
    for (i = 0; i < 2; i++) { var y = 30 + r() * (h - 60), d = (r() - 0.5) * 30; cr += '<path d="M0 ' + y + ' L' + w + ' ' + (y + d) + '" stroke="#fff" stroke-opacity=".22"/><path d="M0 ' + (y + 1.4) + ' L' + w + ' ' + (y + d + 1.4) + '" stroke="#000" stroke-opacity=".18"/>'; }
    for (i = 0; i < 9; i++) { var e = r() * 4, t = r() * 200, l = 4 + r() * 16;
      sc += e < 1 ? rect(t * w / 200, 0, l, 1.6, '#f3efe4', 'opacity=".75"') : e < 2 ? rect(t * w / 200, h - 1.6, l, 1.6, '#f3efe4', 'opacity=".75"') : e < 3 ? rect(0, t * h / 200, 1.6, l, '#f3efe4', 'opacity=".75"') : rect(w - 1.6, t * h / 200, 1.6, l, '#f3efe4', 'opacity=".75"'); }
    var has = function (k) { return P.indexOf(' ' + k + ' ') >= 0; };
    if (has('fade')) fx += rect(0, 0, w, h, 'url(#gbVeil)');
    if (has('yellow')) fx += rect(0, 0, w, h, '#c9a13a', 'style="mix-blend-mode:multiply" opacity=".34"') + rect(0, 0, w, h, 'url(#gbAge)');
    if (has('stain')) {
      var sx = 30 + r() * (w - 60), sy = 50 + r() * (h - 100), sr = 16 + r() * 10;
      fx += '<g filter="url(#gbWob)"><circle cx="' + sx + '" cy="' + sy + '" r="' + sr + '" fill="#7a5230" fill-opacity=".14" stroke="#4a2a10" stroke-opacity=".55" stroke-width="3.5"/>' +
        '<circle cx="' + (sx + sr * 0.55) + '" cy="' + (sy + sr * 0.35) + '" r="' + (sr * 0.7) + '" fill="none" stroke="#4a2a10" stroke-opacity=".3" stroke-width="2"/><circle cx="' + (sx - sr * 0.9) + '" cy="' + (sy + sr * 1.15) + '" r="2.6" fill="#4a2a10" fill-opacity=".45"/></g>';
    }
    if (has('tape')) { var tx = r() < 0.5 ? 6 : w - 54, ty = r() < 0.5 ? 6 : h - 30;
      fx += '<g transform="rotate(' + (-16 + r() * 32).toFixed(0) + ' ' + (tx + 24) + ' ' + (ty + 8) + ')"><rect x="' + tx + '" y="' + ty + '" width="48" height="16" fill="#f0e2a8" fill-opacity=".62" stroke="#fff" stroke-opacity=".35"/>' +
        '<path d="M' + tx + ' ' + ty + 'l3 4-3 4 3 4-3 4" stroke="#b8a458" stroke-opacity=".6" fill="none"/><path d="M' + (tx + 48) + ' ' + ty + 'l-3 4 3 4-3 4 3 4" stroke="#b8a458" stroke-opacity=".6" fill="none"/></g>'; }
    if (has('sticker')) { var px = 12 + r() * (w - 70), py = h - 70 - r() * 30, pr = (r() - 0.5) * 24;
      fx += '<g transform="rotate(' + pr.toFixed(0) + ' ' + (px + 24) + ' ' + (py + 10) + ')"><rect x="' + px + '" y="' + py + '" width="50" height="22" rx="2" fill="#ffe34d" stroke="#d8262c" stroke-width="1.5"/>' +
        txt(PRICE[Math.floor(r() * PRICE.length)], px + 25, py + 15, 8, '#d8262c', PX) + '<path d="M' + px + ' ' + py + 'l9 0-9 9z" fill="#fff" fill-opacity=".4"/></g>'; }
    if (has('crush')) { fx += poly(w + ',' + (h - 54) + ' ' + (w - 38) + ',' + h + ' ' + w + ',' + h, '#000', 'opacity=".28"') + path('M' + w + ' ' + (h - 54) + 'L' + (w - 38) + ' ' + h, 0, '#fff', 1.6, 'opacity=".5"') +
        path('M' + (w - 8) + ' ' + (h - 30) + 'l-14 8M' + (w - 4) + ' ' + (h - 14) + 'l-18 3', 0, '#000', 1, 'opacity=".4"') + poly('0,' + (h - 30) + ' 24,' + h + ' 0,' + h, '#000', 'opacity=".2"'); }
    if (has('scratch')) { for (i = 0; i < 6; i++) { var x1 = r() * w, y1 = r() * h, a2 = r() * 6.28, ln = 12 + r() * 38;
        fx += line(x1.toFixed(1), y1.toFixed(1), (x1 + Math.cos(a2) * ln).toFixed(1), (y1 + Math.sin(a2) * ln).toFixed(1), i % 3 ? '#fff' : '#000', 0.9, 'opacity="' + (i % 3 ? 0.45 : 0.3) + '"'); } }
    if (has('dent')) { var dx = r() < 0.5 ? 0 : w, dy = r() < 0.5 ? 0 : h; fx += '<circle cx="' + dx + '" cy="' + dy + '" r="64" fill="url(#gbDent)"/>' + '<circle cx="' + (dx ? w - 14 : 14) + '" cy="' + (dy ? h - 14 : 14) + '" r="5" fill="#fff" fill-opacity=".15"/>'; }
    if (has('fold')) { var fxp = w * (0.42 + r() * 0.16); fx += line(fxp.toFixed(1), 0, (fxp + 6).toFixed(1), h, '#fff', 1.4, 'opacity=".4"') + line((fxp + 1.6).toFixed(1), 0, (fxp + 7.6).toFixed(1), h, '#000', 1, 'opacity=".28"') + rect(fxp - 3, 0, 8, h, '#fff', 'opacity=".05" transform="skewX(-1)"'); }
    return rect(0, 0, w, h, 'url(#gbHalf)') + rect(0, 0, w, h, '#fff', 'filter="url(#gbMottle)" opacity=".12"') + rect(0, 0, w, h, '#000', 'filter="url(#gbGrain)" opacity=".34"') + cr + fx +
      rect(0, 0, w, h, 'url(#gbVig)') + rect(0, 0, w, h, 'url(#gbGloss)') + poly(w + ',0 ' + (w - 24) + ',0 ' + w + ',22', '#e9e5d8', 'opacity=".55"') + sc +
      rect(0.8, 0.8, w - 1.6, h - 1.6, 'none', 'stroke="#fff" stroke-opacity=".28" stroke-width="1.6"');
  }
  var DEFS = '<svg width="0" height="0" style="position:absolute" aria-hidden="true"><defs>' +
    '<filter id="gbGrain" x="0" y="0" width="100%" height="100%"><feTurbulence type="fractalNoise" baseFrequency=".85" numOctaves="2" seed="4"/><feColorMatrix type="matrix" values="0 0 0 0 .1  0 0 0 0 .08  0 0 0 0 .1  0 0 0 1.5 -.62"/></filter>' +
    '<filter id="gbMottle" x="0" y="0" width="100%" height="100%"><feTurbulence type="fractalNoise" baseFrequency=".012 .03" numOctaves="3" seed="9"/><feColorMatrix type="matrix" values="0 0 0 0 1  0 0 0 0 1  0 0 0 0 .95  0 0 0 .9 -.4"/></filter>' +
    '<linearGradient id="gbGloss" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#fff" stop-opacity=".34"/><stop offset=".32" stop-color="#fff" stop-opacity=".06"/><stop offset=".5" stop-color="#fff" stop-opacity="0"/><stop offset=".82" stop-color="#000" stop-opacity=".12"/><stop offset="1" stop-color="#000" stop-opacity=".3"/></linearGradient>' +
    '<radialGradient id="gbVig" cx=".5" cy=".5" r=".75"><stop offset=".6" stop-color="#000" stop-opacity="0"/><stop offset="1" stop-color="#000" stop-opacity=".38"/></radialGradient>' +
    '<filter id="gbFade" x="0" y="0" width="100%" height="100%"><feColorMatrix type="saturate" values=".68"/><feComponentTransfer><feFuncR type="linear" slope=".9" intercept=".1"/><feFuncG type="linear" slope=".9" intercept=".1"/><feFuncB type="linear" slope=".9" intercept=".1"/></feComponentTransfer></filter>' +
    '<filter id="gbWob" x="-30%" y="-30%" width="160%" height="160%"><feTurbulence type="fractalNoise" baseFrequency=".035" numOctaves="2" seed="3" result="n"/><feDisplacementMap in="SourceGraphic" in2="n" scale="9"/></filter>' +
    '<linearGradient id="gbVeil" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#fff" stop-opacity=".26"/><stop offset=".6" stop-color="#fff" stop-opacity=".08"/><stop offset="1" stop-color="#fff" stop-opacity=".04"/></linearGradient>' +
    '<radialGradient id="gbAge" cx=".5" cy=".5" r=".8"><stop offset=".5" stop-color="#8a5a1a" stop-opacity="0"/><stop offset="1" stop-color="#8a5a1a" stop-opacity=".4"/></radialGradient>' +
    '<radialGradient id="gbDent"><stop offset="0" stop-color="#000" stop-opacity=".42"/><stop offset=".5" stop-color="#000" stop-opacity=".14"/><stop offset="1" stop-color="#000" stop-opacity="0"/></radialGradient>' +
    '<pattern id="gbHalf" width="3" height="3" patternUnits="userSpaceOnUse" patternTransform="rotate(30)"><circle cx="1.5" cy="1.5" r=".7" fill="#000" opacity=".16"/></pattern></defs></svg>';

  /* ---- a chip's box: front, a spine on the left, a lid on top */
  function chipBox(k) {
    var a = ART[k](), n = 'c' + k;
    return '<svg viewBox="0 0 240 312" role="img" aria-label="' + k + '"><defs><clipPath id="' + n + '"><rect x="30" y="26" width="200" height="270"/></clipPath></defs>' +
      '<polygon points="30,26 8,12 8,282 30,296" fill="' + a[1] + '"/><polygon points="30,26 8,12 208,12 230,26" fill="' + a[2] + '"/>' +
      '<g clip-path="url(#' + n + ')"><g transform="translate(30 26)">' + (((PROF[k] || '').indexOf('fade') >= 0) ? '<g filter="url(#gbFade)">' + a[3] + '</g>' : a[3]) + wear(k, W, H) + '</g></g>' +
      '<g transform="translate(19 154) rotate(-90)"><text text-anchor="middle" font-family="' + PX + '" font-size="7" fill="#fff" opacity=".85" letter-spacing="1">' + a[0] + '</text></g>' +
      '<polygon points="30,26 8,12 8,282 30,296" fill="#000" opacity=".18"/><rect x="30" y="26" width="200" height="270" fill="none" stroke="#000" stroke-opacity=".35" stroke-width="1.5"/></svg>';
  }

  /* ---- the three brothers */
  var BRO = [
    { k: 'chipbox', name: 'CHIPBOX', tag: 'CHIPTUNE TRACKER', c1: '#2b3aa0', c2: '#a8cd1c', sz: 18, shot: 'img/bros/chipbox.png', href: '../chipbox/',
      art: rect(0, 62, 240, 206, '#2b3aa0') + rect(0, 62, 240, 30, '#3e50c8') + rect(0, 122, 240, 30, '#3e50c8', 'opacity=".6"') +
        [[30, 80], [190, 74], [214, 110], [40, 120], [170, 140]].map(function (p) { return rect(p[0], p[1], 4, 4, '#fff', 'opacity=".9"'); }).join('') +
        '<g transform="translate(70 76)">' + [10, 26, 42, 58].map(function (y) { return rect(-8, y, 12, 6, '#c9c7bf') + rect(96, y, 12, 6, '#c9c7bf'); }).join('') +
        rect(0, 0, 100, 74, '#1c1c24', 'rx="4"') + rect(3, 3, 94, 68, 'none', 'rx="2" stroke="#3a3a48" stroke-width="2"') + circ(14, 14, 4, '#3a3a48') +
        txt('CHIP', 50, 40, 11, '#a8cd1c', PX) + txt('35+ SOUNDS', 50, 58, 7, '#8a8a9a', PX) + '</g>' +
        ['#eb4034', '#eb4034', '#7a2018', '#eb4034', '#7a2018', '#a8cd1c', '#3b5410', '#a8cd1c', '#46aaff', '#1c4a7a'].map(function (c, i) { return rect(34 + i * 18, 160, 14, 14, c); }).join('') },
    { k: 'smileyface', name: 'SMILEYFACE', tag: 'ACID BASS LINE', c1: '#ffec00', c2: '#ff1fa6', sz: 13, shot: 'img/bros/smileyface.png', href: '../smileyface/',
      art: rect(0, 62, 240, 206, '#ffec00') + [[0, 62], [80, 62], [160, 62], [40, 102], [120, 102], [200, 102]].map(function (p) { return rect(p[0], p[1], 40, 40, '#e0cf00'); }).join('') +
        circ(94, 140, 52, '#ffec00', 'stroke="#111" stroke-width="8"') + '<ellipse cx="76" cy="124" rx="7" ry="13" fill="#111"/><ellipse cx="112" cy="124" rx="7" ry="13" fill="#111"/>' + path('M64 154q30 32 60 0', 0, '#111', 8, 'stroke-linecap="round"') +
        txt('303', 94, 252, 30, '#ff1fa6', SANS, { w: 900, s: '#111', sw: 1.5 }) },
    { k: 'drumlab', name: 'DRUMLAB', tag: 'DRUMS + PO-32', c1: '#0f6f73', c2: '#ef6a3c', sz: 18, shot: 'img/bros/drumlab.png', href: '../drumlab/',
      art: rect(0, 62, 240, 206, '#0f6f73') + rect(0, 62, 240, 26, '#16878c') + rect(0, 114, 240, 26, '#16878c', 'opacity=".7"') +
        '<g transform="translate(88 128)" fill="none" stroke="#f6e7c1" stroke-width="3"><ellipse rx="56" ry="20"/><ellipse rx="56" ry="20" transform="rotate(60)"/><ellipse rx="56" ry="20" transform="rotate(120)"/></g>' +
        circ(88, 128, 15, '#f6e7c1') + circ(88, 128, 12, '#ef6a3c') + circ(144, 128, 5, '#f6e7c1') + circ(60, 80, 5, '#f6e7c1') + circ(60, 176, 5, '#f6e7c1') +
        ['#ef6a3c', '#f6e7c1', '#ef6a3c', '#f6e7c1'].map(function (c, i) { return rect(22 + i * 30, 240, 24, 14, c, 'rx="3"'); }).join('') }
  ];
  function broBox(b, me) {
    var inner = rect(0, 0, 240, 340, b.c1) + rect(0, 0, 240, 58, '#14131a') + rect(0, 58, 240, 4, b.c2) + txt(b.name, 120, 38, b.sz, '#fff', PX, { ls: 1 }) + b.art +
      rect(0, 268, 240, 72, '#14131a') + rect(0, 268, 240, 4, b.c2) + txt(b.tag, 120, 298, 9, '#fff', PX, { ls: 1 }) + txt('FOR THE FM-1', 120, 320, 7, b.c2, PX, { ls: 1 }) +
      rect(0, 0, 14, 340, '#000', 'opacity=".28"') + wear('bro' + b.k, 240, 340);
    var svg = '<svg viewBox="0 0 240 340" width="240" height="340" role="img" aria-label="' + b.name + '"><defs><clipPath id="bc' + b.k + '"><rect width="240" height="340" rx="6"/></clipPath></defs><g clip-path="url(#bc' + b.k + ')">' + inner + '</g><rect width="240" height="340" fill="none" stroke="#000" stroke-opacity=".5" stroke-width="2" rx="6"/></svg>';
    return '<a class="box' + (me ? ' me' : '') + '" ' + (!me && b.href ? 'href="' + b.href + '"' : 'href="#" aria-current="page"') + '>' + svg + '<span class="shot"><img src="' + b.shot + '" alt=""></span><span class="badge">FM-1</span></a>';
  }

  function run() {
    var holder = document.createElement('div');
    holder.innerHTML = DEFS;
    document.body.insertBefore(holder.firstChild, document.body.firstChild);
    var shelf = document.getElementById('shelf');
    var who = window.BRO_ME || 'chipbox';
    if (shelf) shelf.innerHTML = BRO.map(function (b) { return broBox(b, b.k === who); }).join('');
    [].forEach.call(document.querySelectorAll('[data-box]'), function (el) { var k = el.getAttribute('data-box'); if (ART[k]) el.innerHTML = chipBox(k); });
    var data = document.getElementById('chipholder'), out = document.getElementById('chips');
    if (!data || !out) return;
    var html = '';
    [].forEach.call(data.querySelectorAll('tr'), function (tr, i) {
      var td = tr.querySelectorAll('td');
      if (!td.length) return;
      var name = td[0].textContent.trim(), k = ID[name];
      if (!k || !ART[k]) return;
      html += '<div class="chip">' + chipBox(k) + '<b>' + name + '</b><span class="ch">' + td[1].textContent.trim() + '</span><span class="docs">' + td[2].innerHTML + '</span></div>';
    });
    out.innerHTML = html;
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', run); else run();
})();
