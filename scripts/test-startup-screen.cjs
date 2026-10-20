// The loading screen the browser draws before the game: lettered in the menus' Barlow, which the build ships with its
// licence, in the main menu's colours (ui_wwii_menu.c) and under its subtitle.
const assert = require('node:assert/strict');
const fs = require('node:fs');

const css = fs.readFileSync('site/startup.css', 'utf8');
const build = fs.readFileSync('scripts/build-web.sh', 'utf8');
const menu = fs.readFileSync('src/PC/ui_mp/ui_wwii_menu.c', 'utf8');

// Barlow Regular and Medium from the site's fonts folder, which the build fills from third_party with the OFL.
const faces = [...css.matchAll(/@font-face\s*\{([^}]*)\}/g)].map(match => match[1]);
assert.equal(faces.length, 2);
for (const [weight, file] of [[400, 'Regular'], [500, 'Medium']]) {
  const face = faces.find(text => new RegExp(`font-weight:\\s*${weight};`).test(text));
  assert.ok(face, file);
  assert.match(face, /font-family:\s*Barlow;/);
  assert.match(face, new RegExp(`src:\\s*url\\('/fonts/Barlow-${file}\\.ttf'\\) format\\('truetype'\\);`));
  assert.match(face, /font-display:\s*block;/);
  assert.ok(build.includes(`"\${repo_root}/third_party/barlow/Barlow-${file}.ttf"`), file);
  assert.equal(fs.readFileSync(`third_party/barlow/Barlow-${file}.ttf`).readUInt32BE(0), 0x00010000, file);
}
assert.ok(build.includes('"${repo_root}/third_party/barlow/OFL.txt"'));
assert.ok(build.includes('install -d "${site_dir}/fonts"') && build.includes('"${site_dir}/fonts/"'));
assert.match(fs.readFileSync('third_party/barlow/OFL.txt', 'utf8'), /SIL OPEN FONT LICENSE/i);

// The main menu's idle items, subtitle, focused gold and version, from its own constants.
function colour(name) {
  const match = menu.match(new RegExp(`static const vec4_t ${name} = \\{ ([\\d.]+)f, ([\\d.]+)f, ([\\d.]+)f, ([\\d.]+)f \\};`));
  assert.ok(match, name);
  return match.slice(1).map(Number);
}
const hex = rgb => '#' + rgb.slice(0, 3).map(value => Math.round(value * 255).toString(16).padStart(2, '0')).join('');
const token = name => (css.match(new RegExp(`--cod2-${name}:\\s*([^;]+);`)) || [])[1];
assert.equal(token('idle'), hex(colour('s_wwiiIdle')));
assert.equal(token('title'), hex(colour('s_wwiiTitle')));
assert.equal(token('gold'), hex(colour('s_wwiiGold')));
const version = colour('s_wwiiVersion');
assert.equal(token('version'), `rgb(${version.slice(0, 3).map(value => Math.round(value * 255)).join(' ')} / ` +
  `${Math.round(version[3] * 100)}%)`);
for (const name of ['idle', 'title', 'gold', 'version']) assert.ok(css.includes(`var(--cod2-${name})`), name);

// The subtitle is the main menu's, in Medium.
const title = menu.match(/#define WWII_TITLE "([^"]+)"/)[1];
const subtitle = css.slice(css.indexOf('#loading-title::after'));
assert.ok(subtitle.includes(`content: '${title}';`));
assert.match(subtitle.slice(0, subtitle.indexOf('}')), /font: 500 /);

// Every font the loading screen and the launcher's errors set is Barlow; no serif or system face is left.
assert.match(token('font'), /^Barlow, /);
const rules = css.slice(css.indexOf('#launcher-form {'));
const fonts = [...rules.matchAll(/^\s*font(?:-family)?:\s*([^;]+);/gm)].map(match => match[1]);
assert.ok(fonts.length >= 5);
for (const font of fonts) assert.match(font, /var\(--cod2-font\)$|^Barlow(, |$)/, font);
assert.doesNotMatch(css, /Georgia|Times New Roman|system-ui|[^-]serif\b/);
console.log('PASS: the loading screen in Barlow Regular and Medium, shipped from third_party with its OFL, in the main ' +
  'menu\'s idle, subtitle, gold and version colours, under its subtitle, and no other face left');
